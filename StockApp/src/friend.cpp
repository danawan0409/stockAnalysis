#include <iostream>
#include <pqxx/pqxx>
#include "user.h"

std::string connect_info = "dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432"; 

void sendFriendRequest(int userID) {
    std::string receiverUsername;
    std::cout << "Enter the username of the user you want to request: ";
    std::cin >> receiverUsername;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Step 1: Get the receiver's ID from their username
        pqxx::result res = W.exec(
            "SELECT id FROM \"User\" WHERE username = " + W.quote(receiverUsername) + ";"
        );

        if (res.empty()) {
            std::cout << "User '" << receiverUsername << "' not found.\n";
            return;
        }

        int receiverID = res[0]["id"].as<int>();

        // Step 2: Check if the two users are already friends or have already sent a request
        pqxx::result timeCheck = W.exec(
            "SELECT EXTRACT(EPOCH FROM CURRENT_TIMESTAMP - updatedTime) FROM Friends "
            "WHERE (senderID = " + W.quote(userID) + " AND receiverID = " + W.quote(receiverID) + ") "
            "OR (senderID = " + W.quote(receiverID) + " AND receiverID = " + W.quote(userID) + ") "
            "AND state IN ('rejected', 'deleted');"
        );
        if (timeCheck[0][0].as<double>() > 300) {  // 300 seconds = 5 minutes
            std::string lastUpdated = timeCheck[0]["updatedTime"].as<std::string>();
            // Compare the time difference to 5 minutes (assuming 'updatedTime' is in timestamp format)
            pqxx::result timeDiffCheck = W.exec(
                "SELECT CURRENT_TIMESTAMP - TIMESTAMP '" + W.quote(lastUpdated) + "' AS diff;"
            );
            if (!timeDiffCheck.empty()) {
                std::string query = "UPDATE Friends "
                                    "SET state = 'pending', "
                                    "senderID = " + W.quote(userID) + ", "
                                    "receiverID = " + W.quote(receiverID) + ", "
                                    "requestTime = CURRENT_TIMESTAMP, "
                                    "updatedTime = CURRENT_TIMESTAMP "
                                    "WHERE ((senderID = " + W.quote(userID) + " AND receiverID = " + W.quote(receiverID) + ") "
                                    "OR (senderID = " + W.quote(receiverID) + " AND receiverID = " + W.quote(userID) + ")) "
                                    "AND state IN ('rejected', 'deleted') "
                                    "AND updatedTime <= CURRENT_TIMESTAMP - interval '5 minutes';";
                W.exec(query);
                W.commit();
                std::cout << "Friend request re-sent to '" << receiverUsername << "'.\n";
            } else {
                std::cout << "You can only send a new friend request after 5 minutes from rejection or deletion.\n";
                return;
            }
        } else {
            // Step 3: If no previous request exists, insert a new one
            W.exec(
                "INSERT INTO Friends (senderID, receiverID, state, requestTime, updatedTime) "
                "VALUES (" + W.quote(userID) + ", " + W.quote(receiverID) + ", 'pending', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);"
            );
            W.commit();
            std::cout << "Friend request sent to '" << receiverUsername << "'.\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void viewIncomingFriendRequests(int userID){
    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);
        
        pqxx::result R = W.exec(
            "SELECT senderID, requestTime "
            "FROM Friends "
            "WHERE receiverID = " + W.quote(userID) + " AND state = 'pending';"
        );

        std::cout << "Incoming Friend Requests:\n";
        for (const auto& row : R) {
            int senderID = row["senderID"].as<int>();
            std::string requestTime = row["requestTime"].c_str();
            std::cout << "From User ID: " << senderID << " at " << requestTime << "\n";
        }

        W.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void viewOutgoingFriendRequests(int userID){
    try {
        pqxx::connection C("your_connection_string_here");
        pqxx::work W(C);

        pqxx::result R = W.exec(
            "SELECT receiverID, requestTime "
            "FROM Friends "
            "WHERE senderID = " + W.quote(userID) + " AND state = 'pending';"
        );

        std::cout << "Outgoing Friend Requests:\n";
        for (const auto& row : R) {
            int receiverID = row["receiverID"].as<int>();
            std::string requestTime = row["requestTime"].c_str();
            std::cout << "To User ID: " << receiverID << " at " << requestTime << "\n";
        }

        W.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void viewFriends(int userID){
    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);
        
        pqxx::result R = W.exec(
            "SELECT CASE "
            "         WHEN senderID = " + W.quote(userID) + " THEN receiverID "
            "         ELSE senderID "
            "       END AS friendID, requestTime, updatedTime "
            "FROM Friends "
            "WHERE (senderID = " + W.quote(userID) + " OR receiverID = " + W.quote(userID) + ") "
            "AND state = 'accepted';"
        );

        std::cout << "Friends:\n";
        for (const auto& row : R) {
            int friendID = row["friendID"].as<int>();
            std::string requestTime = row["requestTime"].c_str();
            std::string updatedTime = row["updatedTime"].c_str();
            std::cout << "Friend ID: " << friendID 
                      << " | Requested: " << requestTime 
                      << " | Updated: " << updatedTime << "\n";
        }

        W.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void acceptFriendRequest(int userID){
    std::string senderUsername;
    std::cout << "Enter the username of the user you want to accept: ";
    std::cin >> senderUsername;

    try {
        pqxx::connection C("your_connection_string_here");
        pqxx::work W(C);

        // Step 1: Get senderID from username
        pqxx::result res = W.exec(
            "SELECT id FROM \"User\" WHERE username = " + W.quote(senderUsername) + ";"
        );

        if (res.empty()) {
            std::cout << "User '" << senderUsername << "' not found.\n";
            return;
        }

        int senderID = res[0]["id"].as<int>();

        // Step 2: Check if there is a pending request from sender to receiver (userID)
        pqxx::result req = W.exec(
            "SELECT * FROM Friends "
            "WHERE senderID = " + W.quote(senderID) + " AND receiverID = " + W.quote(userID) + " AND state = 'pending';"
        );

        if (req.empty()) {
            std::cout << "No pending friend request from user '" << senderUsername << "'.\n";
            return;
        }

        // Step 3: Accept the request
        W.exec0(
            "UPDATE Friends "
            "SET state = 'accepted', updatedTime = CURRENT_TIMESTAMP "
            "WHERE senderID = " + W.quote(senderID) + " AND receiverID = " + W.quote(userID) + " AND state = 'pending';"
        );

        W.commit();
        std::cout << "Friend request accepted!\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void rejectFriendRequest(int userID) {
    std::string senderUsername;
    std::cout << "Enter the username of the person who sent the friend request: ";
    std::cin >> senderUsername;

    try {
        pqxx::connection C("your_connection_string_here");
        pqxx::work W(C);

        // Step 1: Get senderID from username
        pqxx::result res = W.exec(
            "SELECT id FROM \"User\" WHERE username = " + W.quote(senderUsername) + ";"
        );

        if (res.empty()) {
            std::cout << "User '" << senderUsername << "' not found.\n";
            return;
        }

        int senderID = res[0]["id"].as<int>();

        // Step 2: Check for a pending friend request
        pqxx::result check = W.exec(
            "SELECT 1 FROM Friends "
            "WHERE senderID = " + W.quote(senderID) +
            " AND receiverID = " + W.quote(userID) +
            " AND state = 'pending';"
        );

        if (check.empty()) {
            std::cout << "No pending friend request from user '" << senderUsername << "'.\n";
        } else {
            // Step 3: Reject the request
            W.exec0(
                "UPDATE Friends "
                "SET state = 'rejected', updatedTime = CURRENT_TIMESTAMP "
                "WHERE senderID = " + W.quote(senderID) +
                " AND receiverID = " + W.quote(userID) +
                " AND state = 'pending';"
            );
            W.commit();
            std::cout << "Friend request from '" << senderUsername << "' rejected.\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void deleteFriend(int userID) {
    std::string friendUsername;
    std::cout << "Enter the username of the friend you want to delete: ";
    std::cin >> friendUsername;

    try {
        pqxx::connection C("your_connection_string_here");
        pqxx::work W(C);

        // Step 1: Get the friend's user ID from their username
        pqxx::result res = W.exec(
            "SELECT id FROM \"User\" WHERE username = " + W.quote(friendUsername) + ";"
        );

        if (res.empty()) {
            std::cout << "User '" << friendUsername << "' not found.\n";
            return;
        }

        int friendID = res[0]["id"].as<int>();

        // Step 2: Check if there is an existing 'accepted' friendship
        pqxx::result check = W.exec(
            "SELECT 1 FROM Friends "
            "WHERE ((senderID = " + W.quote(userID) + " AND receiverID = " + W.quote(friendID) + ") "
            "OR (senderID = " + W.quote(friendID) + " AND receiverID = " + W.quote(userID) + ")) "
            "AND state = 'accepted';"
        );

        if (check.empty()) {
            std::cout << "No accepted friendship between you and '" << friendUsername << "'.\n";
        } else {
            // Step 3: Mark the friendship as 'deleted'
            W.exec0(
                "UPDATE Friends "
                "SET state = 'deleted', updatedTime = CURRENT_TIMESTAMP "
                "WHERE (senderID = " + W.quote(userID) + " AND receiverID = " + W.quote(friendID) + " AND state = 'accepted') "
                "OR (senderID = " + W.quote(friendID) + " AND receiverID = " + W.quote(userID) + " AND state = 'accepted');"
            );
            W.commit();
            std::cout << "Friendship with '" << friendUsername << "' has been deleted.\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}