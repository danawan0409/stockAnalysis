#include <iostream>
#include "global.h"
#include "user.h"
#include "portfolio.h"
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

        if (!currentUsername.empty()) {
            std::cout << "4. Send Friend Request\n";
            std::cout << "5. View Incoming Friend Requests\n";
            std::cout << "6. View Outgoing Friend Requests\n";
            std::cout << "7. View Friends\n";
            std::cout << "8. Accept Friend Request\n";
            std::cout << "9. Reject Friend Request\n";
            std::cout << "10. Delete Friend\n";
            std::cout << "11. Create Portfolio\n";
            std::cout << "12. View My Portfolios\n";
            std::cout << "13. Delete Portfolio\n";
            std::cout << "14. Deposit Cash\n";
            std::cout << "15. Withdraw Cash\n";
            std::cout << "16. Buy Stock\n";
        }

        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                registerUser();
                break;
            case 2:
                loginUser();
                break;
            case 3:
                running = false;
                break;
            case 4:
                sendFriendRequest(currentUsername);
                break;
            case 5:
                viewIncomingFriendRequests(currentUsername);
                break;
            case 6:
                viewOutgoingFriendRequests(currentUsername); 
                break;
            case 7:
                viewFriends(currentUsername); 
                break;
            case 8:
                acceptFriendRequest(currentUsername); 
                break;
            case 9:
                rejectFriendRequest(currentUsername); 
                break;
            case 10:
                deleteFriend(currentUsername);
                break;
            case 11:
                createPortfolio(currentUsername);
                break;
            case 12:
                viewPortfolios(currentUsername);
                break;
            case 13:
                deletePortfolio(currentUsername);
                break;
            case 14:
                depositCash(currentUsername);
                break;
            case 15:
                withdrawCash(currentUsername);
                break;
            case 16:
                buyStock(currentUsername);
                break;
            default:
                std::cout << "Invalid choice. Try again.\n";
        }
    }

    return 0;
}
