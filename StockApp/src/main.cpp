#include <iostream>
#include "global.h"
#include "user.h"
#include "portfolio.h"
#include "friend.h"
#include "stocklist.h"

void portfolioMenu();
void friendMenu();
void stockListMenu();

int main() {
    int choice;
    bool running = true;

    while (running) {
        std::cout << "=============================\n";
        std::cout << "  Welcome to Stock Platform \n";
        std::cout << "=============================\n";

        if (currentUsername.empty()) {
            std::cout << "1. Register\n";
            std::cout << "2. Login\n";
            std::cout << "3. Exit\n";
        } else {
            std::cout << "Logged in as: " << currentUsername << "\n";
            std::cout << "1. Logout\n";
            std::cout << "2. Go to Portfolio Menu\n";
            std::cout << "3. Go to Friends Menu\n";
            std::cout << "4. Go to Stock List Menu\n";
            std::cout << "5. Exit\n";
        }

        std::cout << "Enter your choice: ";
        std::cin >> choice;

        if (currentUsername.empty()) {
            switch (choice) {
                case 1:
                    registerUser();
                    pause();
                    break;
                case 2:
                    loginUser();
                    pause();
                    break;
                case 3:
                    running = false;
                    break;
                default:
                    std::cout << "Invalid choice. Try again.\n";
                    pause();
            }
        } else {
            switch (choice) {
                case 1:
                    currentUsername.clear();
                    std::cout << "Logged out successfully.\n";
                    pause();
                    break;
                case 2:
                    portfolioMenu();
                    break;
                case 3:
                    friendMenu();
                    break;
                case 4:
                    stockListMenu();
                    break;
                case 5:
                    running = false;
                    break;
                default:
                    std::cout << "Invalid choice. Try again.\n";
                    pause();
            }
        }
    }
    return 0;
}

void portfolioMenu() {
    int choice;

    while (1) {
        std::cout << "=============================\n";
        std::cout << "     Portfolio Menu\n";
        std::cout << "=============================\n";
        std::cout << "1. Create Portfolio\n";
        std::cout << "2. View My Portfolios\n";
        std::cout << "3. Delete Portfolio\n";
        std::cout << "4. Deposit Cash\n";
        std::cout << "5. Withdraw Cash\n";
        std::cout << "6. Buy Stock\n";
        std::cout << "7. Sell Stock\n";
        std::cout << "8. Go Back to Main Menu\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                createPortfolio(currentUsername);
                pause();
                break;
            case 2:
                viewPortfolios(currentUsername);
                pause();
                break;
            case 3:
                deletePortfolio(currentUsername);
                pause();
                break;
            case 4:
                depositCash(currentUsername);
                pause();
                break;
            case 5:
                withdrawCash(currentUsername);
                pause();
                break;
            case 6:
                buyStock(currentUsername);
                pause();
                break;
            case 7:
                sellStock(currentUsername);
                pause();
                break;
            case 8:
                return;
            default:
                std::cout << "Invalid choice. Try again.\n";
                pause();
        }
    }
}

void friendMenu(){
    int choice;

    while (1) {
        std::cout << "=============================\n";
        std::cout << "  Friends Menu\n";
        std::cout << "=============================\n";
        std::cout << "1. Send Friend Request\n";
        std::cout << "2. View Incoming Friend Requests\n";
        std::cout << "3. View Outgoing Friend Requests\n";
        std::cout << "4. View Friends\n";
        std::cout << "5. Accept Friend Request\n";
        std::cout << "6. Reject Friend Request\n";
        std::cout << "7. Delete Friend\n";
        std::cout << "8. Go Back to Main Menu\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                sendFriendRequest(currentUsername);
                pause();
                break;
            case 2:
                viewIncomingFriendRequests(currentUsername);
                pause();
                break;
            case 3:
                viewOutgoingFriendRequests(currentUsername); 
                pause();
                break;
            case 4:
                viewFriends(currentUsername); 
                pause();
                break;
            case 5:
                acceptFriendRequest(currentUsername); 
                pause();
                break;
            case 6:
                rejectFriendRequest(currentUsername); 
                pause();
                break;
            case 7:
                deleteFriend(currentUsername);
                pause();
                break;
            case 8:
                return;       
            default:
                std::cout << "Invalid choice. Try again.\n";
                pause();
        }
    }
}

void stockListMenu(){
    int choice;

    while (1) {
        std::cout << "=============================\n";
        std::cout << "  Stock List Menu\n";
        std::cout << "=============================\n";
        std::cout << "1. View All (Accessible) Stock Lists\n";
        std::cout << "2. View Your Stock Lists\n";
        std::cout << "3. Create a Stock List\n";
        std::cout << "4. Delete a Stock List\n";
        std::cout << "5. Share a Stock List\n";
        std::cout << "6. View a Stock List Review\n";
        std::cout << "7. Add a Review for a Stock List\n";
        std::cout << "8. Edit a Review for a Stock List\n";
        std::cout << "9. Delete a Review for a Stock List\n";
        std::cout << "10. View all Stocks in a Stock Listt\n";
        std::cout << "11. Go Back to Main Menu\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                viewStockLists(currentUsername);
                pause();
                break;
            case 2:
                viewOwnStockLists(currentUsername); 
                pause();
                break;
            case 3:
                createStockList(currentUsername);
                pause();
                break;
            case 4:
                deleteStockList(currentUsername);
                pause();
                break;
            case 5:
                shareStockList(currentUsername); 
                pause();
                break;
            case 6:
                viewReviewStockList(currentUsername); 
                pause();
                break;
            case 7:
                addReviewStockList(currentUsername); 
                pause();
                break;        
            case 8:
                editReviewStockList(currentUsername); 
                pause();
                break;
            case 9:
                deleteReviewStockList(currentUsername); 
                pause();
                break;
            case 10: 
                viewStockListsStock(currentUsername); 
                pause();
                break; 
            case 11:
                return;        
            default:
                std::cout << "Invalid choice. Try again.\n";
                pause();
        }
    }
}
