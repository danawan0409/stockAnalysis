#include <iostream>
#include "user.h"
#include "friend.h"

int main() {
    int choice;
    bool running = true;

    while (running) {
        std::cout << "=============================\n";
        std::cout << "  Welcome to Stock Platform \n";
        std::cout << "=============================\n";
        std::cout << "1. Register\n";
        std::cout << "2. Login\n";
        std::cout << "3. Exit\n";
        std::cout << "4. Send Friend Request (test)\n";
        std::cout << "5. View Incoming Friend Requests (test)\n";
        std::cout << "6. View Outgoing Friend Requests (test)\n";
        std::cout << "7. View Friends (test)\n";
        std::cout << "8. Accept Friend Request (test)\n";
        std::cout << "9. Reject Friend Request (test)\n";
        std::cout << "10. Delete Friend (test)\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                registerUser();
                break;
            case 2:
                loginUser();
                break;
            case 4:
                sendFriendRequest(1);
                break;
            case 5:
                viewIncomingFriendRequests(1);
                break;
            case 6:
                viewOutgoingFriendRequests(1); 
                break;
            case 7:
                viewFriends(1); 
                break;
            case 8:
                acceptFriendRequest(1); 
                break;
            case 9:
                rejectFriendRequest(1); 
                break;
            case 10:
                deleteFriend(1);
                break;
            case 3:
                running = false;
                break;
            default:
                std::cout << "Invalid choice. Try again.\n";
        }
    }

    return 0;
}
