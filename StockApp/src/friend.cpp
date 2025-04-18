#include <iostream>
#include <pqxx/pqxx>
#include "user.h"
#include "global.h"

void sendFriendRequest(const std::string& senderUsername) {
    std::string receiverUsername;
    std::cout << "Enter the username of the user you want to request: ";
    std::cin >> receiverUsername;

    if (senderUsername == receiverUsername) {
        std::cout << "You can't send a friend request to yourself.\n";
        return;
    }

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string query = "SELECT username FROM \"User\" WHERE username = " + W.quote(receiverUsername) + ";";
        pqxx::result res = W.exec(query);

        if (res.empty()) {
            std::cout << "User '" << receiverUsername << "' not found.\n";
            return;
        }

        // Check if a previous request was rejected or deleted recently
        query =
            "SELECT EXTRACT(EPOCH FROM CURRENT_TIMESTAMP - updatedTime) AS seconds "
            "FROM Friends "
            "WHERE ((senderUsername = " + W.quote(senderUsername) + " AND receiverUsername = " + W.quote(receiverUsername) + ") "
            "   OR (senderUsername = " + W.quote(receiverUsername) + " AND receiverUsername = " + W.quote(senderUsername) + ")) "
            "AND state IN ('rejected', 'deleted');";
        pqxx::result timeCheck = W.exec(query);

        if (!timeCheck.empty()) {
            double secondsSinceLast = timeCheck[0]["seconds"].as<double>();
            if (secondsSinceLast < 300) {
                std::cout << "Please wait 5 minutes after a friend request has been rejected or a friend has been deleted to request again.\n";
                return;
            } else {
                // Resend request
                query =
                    "UPDATE Friends "
                    "SET state = 'pending', senderUsername = " + W.quote(senderUsername) +
                    ", receiverUsername = " + W.quote(receiverUsername) +
                    ", requestTime = CURRENT_TIMESTAMP, updatedTime = CURRENT_TIMESTAMP "
                    "WHERE ((senderUsername = " + W.quote(senderUsername) + " AND receiverUsername = " + W.quote(receiverUsername) + ") "
                    "   OR (senderUsername = " + W.quote(receiverUsername) + " AND receiverUsername = " + W.quote(senderUsername) + ")) "
                    "AND state IN ('rejected', 'deleted');";
                W.exec(query);
                W.commit();
                std::cout << "Friend request re-sent to '" << receiverUsername << "'.\n";
                return;
            }
        }

        // Check if any friend request or friendship already exists
        query =
            "SELECT 1 FROM Friends "
            "WHERE (senderUsername = " + W.quote(senderUsername) + " AND receiverUsername = " + W.quote(receiverUsername) + ") "
            "OR (senderUsername = " + W.quote(receiverUsername) + " AND receiverUsername = " + W.quote(senderUsername) + ");";
        pqxx::result existing = W.exec(query);

        if (!existing.empty()) {
            std::cout << "A friend request already exists or you're already friends.\n";
            return;
        }

        // Send new request
        query =
            "INSERT INTO Friends (senderUsername, receiverUsername, state, requestTime, updatedTime) "
            "VALUES (" + W.quote(senderUsername) + ", " + W.quote(receiverUsername) + ", 'pending', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";
        W.exec(query);
        W.commit();
        std::cout << "Friend request sent to '" << receiverUsername << "'.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void viewIncomingFriendRequests(const std::string& username) {
    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string query =
            "SELECT senderUsername, requestTime "
            "FROM Friends "
            "WHERE receiverUsername = " + W.quote(username) + " AND state = 'pending';";
        pqxx::result R = W.exec(query);

        std::cout << "Incoming Friend Requests:\n";
        for (const auto& row : R) {
            std::string sender = row["senderUsername"].as<std::string>();
            std::string requestTime = row["requestTime"].c_str();
            std::cout << "From: " << sender << " at " << requestTime << "\n";
        }

        W.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void viewOutgoingFriendRequests(const std::string& username) {
    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string query =
            "SELECT receiverUsername, requestTime "
            "FROM Friends "
            "WHERE senderUsername = " + W.quote(username) + " AND state = 'pending';";
        pqxx::result R = W.exec(query);

        std::cout << "Outgoing Friend Requests:\n";
        for (const auto& row : R) {
            std::string receiver = row["receiverUsername"].as<std::string>();
            std::string requestTime = row["requestTime"].c_str();
            std::cout << "To: " << receiver << " at " << requestTime << "\n";
        }

        W.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void viewFriends(const std::string& username) {
    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string query =
            "SELECT CASE "
            "         WHEN senderUsername = " + W.quote(username) + " THEN receiverUsername "
            "         ELSE senderUsername "
            "       END AS friend, requestTime, updatedTime "
            "FROM Friends "
            "WHERE (senderUsername = " + W.quote(username) + " OR receiverUsername = " + W.quote(username) + ") "
            "AND state = 'accepted';";
        pqxx::result R = W.exec(query);

        std::cout << "Friends:\n";
        for (const auto& row : R) {
            std::string friendName = row["friend"].as<std::string>();
            std::string requestTime = row["requestTime"].c_str();
            std::string updatedTime = row["updatedTime"].c_str();
            std::cout << "Friend: " << friendName << " | Requested: " << requestTime << " | Accepted: " << updatedTime << "\n";
        }

        W.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void acceptFriendRequest(const std::string& receiverUsername) {
    std::string senderUsername;
    std::cout << "Enter the username of the user you want to accept: ";
    std::cin >> senderUsername;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string query =
            "SELECT 1 FROM Friends "
            "WHERE senderUsername = " + W.quote(senderUsername) +
            " AND receiverUsername = " + W.quote(receiverUsername) +
            " AND state = 'pending';";
        pqxx::result res = W.exec(query);

        if (res.empty()) {
            std::cout << "No pending friend request from '" << senderUsername << "'.\n";
            return;
        }

        query =
            "UPDATE Friends "
            "SET state = 'accepted', updatedTime = CURRENT_TIMESTAMP "
            "WHERE senderUsername = " + W.quote(senderUsername) +
            " AND receiverUsername = " + W.quote(receiverUsername) +
            " AND state = 'pending';";
        W.exec(query);
        W.commit();
        std::cout << "Friend request from '" << senderUsername << "' accepted!\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void rejectFriendRequest(const std::string& receiverUsername) {
    std::string senderUsername;
    std::cout << "Enter the username of the user you want to reject: ";
    std::cin >> senderUsername;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string query =
            "SELECT 1 FROM Friends "
            "WHERE senderUsername = " + W.quote(senderUsername) +
            " AND receiverUsername = " + W.quote(receiverUsername) +
            " AND state = 'pending';";
        pqxx::result res = W.exec(query);

        if (res.empty()) {
            std::cout << "No pending friend request from '" << senderUsername << "'.\n";
            return;
        }

        query =
            "UPDATE Friends "
            "SET state = 'rejected', updatedTime = CURRENT_TIMESTAMP "
            "WHERE senderUsername = " + W.quote(senderUsername) +
            " AND receiverUsername = " + W.quote(receiverUsername) +
            " AND state = 'pending';";
        W.exec(query);
        W.commit();
        std::cout << "Friend request from '" << senderUsername << "' rejected.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void deleteFriend(const std::string& username) {
    std::string otherUsername;
    std::cout << "Enter the username of the friend to delete: ";
    std::cin >> otherUsername;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string query =
            "SELECT 1 FROM Friends "
            "WHERE ((senderUsername = " + W.quote(username) + " AND receiverUsername = " + W.quote(otherUsername) + ") "
            "   OR (senderUsername = " + W.quote(otherUsername) + " AND receiverUsername = " + W.quote(username) + ")) "
            "   AND state = 'accepted';";
        pqxx::result check = W.exec(query);

        if (check.empty()) {
            std::cout << "You are not friends with '" << otherUsername << "'.\n";
            return;
        }

        // Set friendship state to 'deleted'
        query =
            "UPDATE Friends "
            "SET state = 'deleted', updatedTime = CURRENT_TIMESTAMP "
            "WHERE (senderUsername = " + W.quote(username) + " AND receiverUsername = " + W.quote(otherUsername) + ") "
            "   OR (senderUsername = " + W.quote(otherUsername) + " AND receiverUsername = " + W.quote(username) + ");";
        W.exec(query);

        // Delete shared stock lists between the two users
        query =
            "DELETE FROM ShareStockList "
            "WHERE (ownerUsername = " + W.quote(username) + " AND receiverUsername = " + W.quote(otherUsername) + ") "
            "   OR (ownerUsername = " + W.quote(otherUsername) + " AND receiverUsername = " + W.quote(username) + ");";
        W.exec(query);

        // Delete reviews written by either about the other's stock lists
        query =
            "DELETE FROM Review "
            "WHERE (writerUsername = " + W.quote(username) + " AND ownerUsername = " + W.quote(otherUsername) + ") "
            "   OR (writerUsername = " + W.quote(otherUsername) + " AND ownerUsername = " + W.quote(username) + ");";
        W.exec(query);

        W.commit();
        std::cout << "Friendship with '" << otherUsername << "' has been deleted.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

