#include <iostream>
#include "global.h"
#include "user.h"
#include "portfolio.h"
#include "friend.h"
#include "stocklist.h"

std::string connect_info = "dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432";

void friendMenu();
void stockListMenu();

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
            std::cout << "4. Create Portfolio\n";
            std::cout << "5. View My Portfolios\n";
            std::cout << "6. Delete Portfolio\n";
            std::cout << "7. Deposit Cash\n";
            std::cout << "8. Withdraw Cash\n";
            std::cout << "9. Buy Stock\n";
            std::cout << "10. Go to Friends Menu\n";
            std::cout << "11. Go to Stock List Menu\n";
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
                createPortfolio(currentUsername);
                break;
            case 5:
                viewPortfolios(currentUsername);
                break;
            case 6:
                deletePortfolio(currentUsername);
                break;
            case 7:
                depositCash(currentUsername);
                break;
            case 8:
                withdrawCash(currentUsername);
                break;   
            case 9:
                buyStock(currentUsername);
                break;   
            case 10:
                friendMenu(); 
                break;   
            case 11:
                stockListMenu(); 
                break;      
            default:
                std::cout << "Invalid choice. Try again.\n";
        }
    }

    return 0;
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
                break;
            case 2:
                viewIncomingFriendRequests(currentUsername);
                break;
            case 3:
                viewOutgoingFriendRequests(currentUsername); 
                break;
            case 4:
                viewFriends(currentUsername); 
                break;
            case 5:
                acceptFriendRequest(currentUsername); 
                break;
            case 6:
                rejectFriendRequest(currentUsername); 
                break;
            case 7:
                deleteFriend(currentUsername);
                break;
            case 8:
                return;       
            default:
                std::cout << "Invalid choice. Try again.\n";
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
        std::cout << "10. Go Back to Main Menu\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                viewStockLists(currentUsername);
                break;
            case 2:
                viewOwnStockLists(currentUsername); 
                break;
            case 3:
                createStockList(currentUsername);
                break;
            case 4:
                deleteStockList(currentUsername);
                break;
            case 5:
                shareStockList(currentUsername); 
                break;
            case 6:
                viewReviewStockList(currentUsername); 
                break;
            case 7:
                addReviewStockList(currentUsername); 
                break;        
            case 8:
                editReviewStockList(currentUsername); 
                break;
            case 9:
                deleteReviewStockList(currentUsername); 
                break;
            case 10:
                return;        
            default:
                std::cout << "Invalid choice. Try again.\n";
        }
    }
} 
