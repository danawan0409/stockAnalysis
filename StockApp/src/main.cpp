#include <iostream>
#include "global.h"
#include "user.h"
#include "portfolio.h"
#include "friend.h"
#include "stocklist.h"
#include "analysis.h"

void portfolioMenu();
void friendMenu();
void stockListMenu();
void analysisMenu();

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
            std::cout << "5. Go to Analysis Menu\n";
            std::cout << "6. Exit\n";
        }

        std::cout << "Enter your choice: ";
        if (!getValidatedInput(choice)) continue;

        if (currentUsername.empty()) {
            switch (choice) {
                case 1:
                    registerUser();
                    pauseConsole();
                    break;
                case 2:
                    loginUser();
                    pauseConsole();
                    break;
                case 3:
                    running = false;
                    break;
                default:
                    std::cout << "Invalid choice. Try again.\n";
                    pauseConsole();
            }
        } else {
            switch (choice) {
                case 1:
                    currentUsername.clear();
                    std::cout << "Logged out successfully.\n";
                    pauseConsole();
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
                    analysisMenu(); 
                    break; 
                case 6:
                    running = false;
                    break;
                default:
                    std::cout << "Invalid choice. Try again.\n";
                    pauseConsole();
            }
        }
    }
    return 0;
}

void portfolioMenu() {
    clearConsole();
    int choice;

    while (1) {
        std::cout << "=============================\n";
        std::cout << "     Portfolio Menu\n";
        std::cout << "=============================\n";
        std::cout << "1. Create Portfolio\n";
        std::cout << "2. View My Portfolios (with Market Value)\n";
        std::cout << "3. Delete My Portfolio\n";
        std::cout << "4. Deposit Cash\n";
        std::cout << "5. Withdraw Cash\n";
        std::cout << "6. Buy Stock\n";
        std::cout << "7. Sell Stock\n";
        std::cout << "8. View Historical Price Graph\n";
        std::cout << "9. View Future Prediction Graph\n";
        std::cout << "10. Go Back to Main Menu\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                createPortfolio(currentUsername);
                pauseConsole();
                break;
            case 2:
                viewPortfolios(currentUsername);
                pauseConsole();
                break;
            case 3:
                deletePortfolio(currentUsername);
                pauseConsole();
                break;
            case 4:
                depositCash(currentUsername);
                pauseConsole();
                break;
            case 5:
                withdrawCash(currentUsername);
                pauseConsole();
                break;
            case 6:
                buyStock(currentUsername);
                pauseConsole();
                break;
            case 7:
                sellStock(currentUsername);
                pauseConsole();
                break;
            case 8:
                viewPortfolioHistorical(currentUsername);
                pauseConsole();
                break;      
            case 9:
                viewPortfolioPrediction(currentUsername);
                pauseConsole();
                break;      
            case 10:
                clearConsole();
                return;
            default:
                std::cout << "Invalid choice. Try again.\n";
                pauseConsole();
        }
    }
}

void friendMenu(){
    clearConsole();
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
                pauseConsole();
                break;
            case 2:
                viewIncomingFriendRequests(currentUsername);
                pauseConsole();
                break;
            case 3:
                viewOutgoingFriendRequests(currentUsername); 
                pauseConsole();
                break;
            case 4:
                viewFriends(currentUsername); 
                pauseConsole();
                break;
            case 5:
                acceptFriendRequest(currentUsername); 
                pauseConsole();
                break;
            case 6:
                rejectFriendRequest(currentUsername); 
                pauseConsole();
                break;
            case 7:
                deleteFriend(currentUsername);
                pauseConsole();
                break;
            case 8:
                clearConsole();
                return;       
            default:
                std::cout << "Invalid choice. Try again.\n";
                pauseConsole();
        }
    }
}

void stockListMenu(){
    clearConsole();
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
                pauseConsole();
                break;
            case 2:
                viewOwnStockLists(currentUsername); 
                pauseConsole();
                break;
            case 3:
                createStockList(currentUsername);
                pauseConsole();
                break;
            case 4:
                deleteStockList(currentUsername);
                pauseConsole();
                break;
            case 5:
                shareStockList(currentUsername); 
                pauseConsole();
                break;
            case 6:
                viewReviewStockList(currentUsername); 
                pauseConsole();
                break;
            case 7:
                addReviewStockList(currentUsername); 
                pauseConsole();
                break;        
            case 8:
                editReviewStockList(currentUsername); 
                pauseConsole();
                break;
            case 9:
                deleteReviewStockList(currentUsername); 
                pauseConsole();
                break;
            case 10: 
                viewStockListsStock(currentUsername); 
                pauseConsole();
                break; 
            case 11:
                clearConsole();
                return;        
            default:
                std::cout << "Invalid choice. Try again.\n";
                pauseConsole();
        }
    }
}

void analysisMenu(){
    clearConsole();
    int choice;

    while (1) {
        std::cout << "=============================\n";
        std::cout << "  Analysis Menu\n";
        std::cout << "=============================\n";
        std::cout << "1. View Correlation Matrix\n";
        std::cout << "2. View Covariance Matrix\n";
        std::cout << "3. View Variation for a Stock\n";
        std::cout << "4. View Beta for a Stock\n";
        std::cout << "5. Go Back to Main Menu\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                findStockListCorrelationMatrix();
                pauseConsole();
                break;
            case 2:
                findStockListCovarianceMatrix(); 
                pauseConsole();
                break;
            case 3:
                findVariation();
                pauseConsole();
                break;
            case 4:
                findBeta();
                pauseConsole();
                break;
            case 5:
                clearConsole();
                return;        
            default:
                std::cout << "Invalid choice. Try again.\n";
                pauseConsole();
        }
    }
}
