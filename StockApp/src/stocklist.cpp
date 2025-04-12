#include <iostream>
#include <pqxx/pqxx>
#include "stocklist.h"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include "global.h"
#include "stock.h"

void createStockList(const std::string& ownerUsername) {
    std::string listName;
    std::cout << "Enter the name of the stock list: ";
    std::getline(std::cin >> std::ws, listName); // get full name including spaces

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check if the user already has a stocklist with this name
        std::string checkQuery =
            "SELECT 1 FROM StockList WHERE name = " + W.quote(listName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";
        pqxx::result checkRes = W.exec(checkQuery);
        if (!checkRes.empty()) {
            std::cout << "You already have a stock list with the name '" << listName << "'.\n";
            return;
        }

        // Ask for visibility
        std::string visibility;
        int choice;
        std::cout << "Choose visibility:\n1. Public\n2. Private\nEnter your choice: ";
        std::cin >> choice;
        if (choice == 1) {
            visibility = "public";
        } else if (choice == 2) {
            visibility = "private";
        } else {
            std::cout << "Invalid choice.\n";
            return;
        }

        // Insert the new stock list
        std::string insertStockList =
            "INSERT INTO StockList (name, ownerUsername, visibility) VALUES (" +
            W.quote(listName) + ", " + W.quote(ownerUsername) + ", " + W.quote(visibility) + ");";
        W.exec(insertStockList);

        // Add stocks
        std::string response = "y"; 
        do {
            std::string input;
            std::cout << "Add a stock (format: SYMBOL, QUANTITY): ";
            std::getline(std::cin >> std::ws, input);

            size_t commaPos = input.find(',');
            if (commaPos == std::string::npos) {
                std::cout << "Invalid format. Use SYMBOL, QUANTITY.\n";
                continue;
            }

            std::string symbol = input.substr(0, commaPos);
            std::string quantityStr = input.substr(commaPos + 1);
            int quantity = 0;
            try {
                quantity = std::stoi(quantityStr);
            } catch (...) {
                std::cout << "Invalid quantity.\n";
                continue;
            }

            // Check if stock exists
            std::string checkStock = "SELECT 1 FROM Stock WHERE symbol = " + W.quote(symbol) + ";";
            pqxx::result stockCheck = W.exec(checkStock);
            if (stockCheck.empty()) {
                std::cout << "Stock '" << symbol << "' does not exist.\n";
                continue;
            }

            // Check if stock is already in the stock list
            std::string checkDup =
                "SELECT 1 FROM StockListHasStock WHERE stockListName = " + W.quote(listName) +
                " AND ownerUsername = " + W.quote(ownerUsername) +
                " AND stockID = " + W.quote(symbol) + ";";
            pqxx::result dupCheck = W.exec(checkDup);
            if (!dupCheck.empty()) {
                std::cout << "Stock '" << symbol << "' is already in the stock list.\n";
                continue;
            }

            // Insert stock to StockListHasStock
            std::string insertStock =
                "INSERT INTO StockListHasStock (stockListName, ownerUsername, stockID, quantity) VALUES (" +
                W.quote(listName) + ", " + W.quote(ownerUsername) + ", " + W.quote(symbol) + ", " + W.quote(quantity) + ");";
            W.exec(insertStock);

            std::cout << "Do you want to add more stocks? (y/n): ";
            std::cin >> response;
        } while (response == "y" || response == "Y");

        W.commit();
        std::cout << "Stock list '" << listName << "' has been successfully created.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void addStocktoStockList(const std::string& ownerUsername){
    return; 
}

void deleteStockfromStockList(const std::string& ownerUsername){
    return; 
}

void changeStockListStatus(const std::string& ownerUsername){
    return; 
}

// Function to read single character input without requiring Enter press
char getch() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ICANON;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}


void viewStockLists(const std::string& ownerUsername) {
    int page = 0;
    int lastPrintedPage = -1;  // To track if we need to re-fetch
    bool running = true;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        while (running) {
            if (page != lastPrintedPage) {
                int offset = page * 20;

                std::string query =
                    "SELECT DISTINCT S.name, S.ownerUsername, "
                    "CASE "
                    "  WHEN S.visibility = 'public' THEN 'public' "
                    "  WHEN S.ownerUsername = " + W.quote(ownerUsername) + " THEN 'private' "
                    "  WHEN EXISTS ("
                    "    SELECT 1 FROM ShareStockList SS "
                    "    WHERE SS.stockListName = S.name AND SS.ownerUsername = S.ownerUsername "
                    "      AND SS.receiverUsername = " + W.quote(ownerUsername) +
                    ") THEN 'shared' "
                    "  ELSE 'private' "
                    "END AS visibility "
                    "FROM StockList S "
                    "LEFT JOIN ShareStockList SS ON S.name = SS.stockListName AND S.ownerUsername = SS.ownerUsername "
                    "WHERE "
                    "(S.ownerUsername = " + W.quote(ownerUsername) + ") "
                    "OR (SS.receiverUsername = " + W.quote(ownerUsername) + ") "
                    "OR (S.visibility = 'public') "
                    "ORDER BY S.ownerUsername, S.name "
                    "LIMIT 20 OFFSET " + W.quote(offset) + ";";

                pqxx::result res = W.exec(query);

                if (res.empty()) {
                    std::cout << "No stock lists found for page " << page << ".\n";
                    page = std::max(0, page - 1);  // Prevent showing empty page
                    break;
                }

                std::cout << "\nStock Lists (Page " << page << "):\n";
                for (const auto& row : res) {
                    std::cout << "- " << row["name"].c_str()
                              << " (Owner: " << row["ownerUsername"].c_str()
                              << ", Visibility: " << row["visibility"].c_str() << ")\n";
                }

                std::cout << "\nUse the arrow keys to navigate (Right to next page, Left to previous page).\n";
                std::cout << "Type 'e' to stop toggling.\n";

                lastPrintedPage = page;  // Mark current page as printed
            }

            char key = getch();
            if (key == 27) { // ESC
                char secondKey = getch();
                char thirdKey = getch();
                if (secondKey == '[') {
                    if (thirdKey == 'C') { // Right
                        page++;
                    } else if (thirdKey == 'D') { // Left
                        if (page > 0) page--;
                    }
                }
            } else if (key == 'e') {
                running = false;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void viewOwnStockLists(const std::string& ownerUsername) {
    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string query =
            "SELECT name, visibility "
            "FROM StockList "
            "WHERE ownerUsername = " + W.quote(ownerUsername) + " "
            "ORDER BY name ASC;";

        pqxx::result res = W.exec(query);

        if (res.empty()) {
            std::cout << "You have no stock lists.\n";
            return;
        }

        std::cout << "Your Stock Lists:\n";
        for (const auto& row : res) {
            std::cout << "- " << row["name"].c_str()
                      << " (Visibility: " << row["visibility"].c_str() << ")\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void viewStockListsStock(const std::string& viewerUsername) {
    std::string listName, ownerUsername;
    std::cout << "Enter the stock list name you want to view: ";
    std::getline(std::cin >> std::ws, listName);  // Support spaces in name

    std::cout << "Enter the owner of this stock list: ";
    std::getline(std::cin >> std::ws, ownerUsername);

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check access permission
        std::string accessQuery =
            "SELECT 1 FROM StockList S "
            "LEFT JOIN ShareStockList SS ON S.name = SS.stockListName AND S.ownerUsername = SS.ownerUsername "
            "WHERE S.name = " + W.quote(listName) + " AND S.ownerUsername = " + W.quote(ownerUsername) + " AND ("
            "S.visibility = 'public' OR "
            "S.ownerUsername = " + W.quote(viewerUsername) + " OR "
            "SS.receiverUsername = " + W.quote(viewerUsername) +
            ");";

        pqxx::result accessResult = W.exec(accessQuery);
        if (accessResult.empty()) {
            std::cout << "Access denied: You don't have permission to view this stock list.\n";
            return;
        }

        // Fetch stocks in the list
        std::string stockQuery =
            "SELECT stockID, quantity FROM StockListHasStock "
            "WHERE stockListName = " + W.quote(listName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";

        pqxx::result stockResult = W.exec(stockQuery);
        if (stockResult.empty()) {
            std::cout << "No stocks found in the stock list.\n";
            return;
        }

        std::cout << "\nStocks in \"" << listName << "\" by " << ownerUsername << ":\n";
        for (const auto& row : stockResult) {
            std::cout << "- Stock: " << row["stockID"].c_str()
                      << ", Quantity: " << row["quantity"].as<int>() << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void shareStockList(const std::string& ownerUsername) {
    std::string stockListName, receiverUsername;
    std::cout << "Enter the name of the stocklist to share: ";
    std::getline(std::cin >> std::ws, stockListName);

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check if stocklist exists and belongs to user
        auto check = W.exec(
            "SELECT 1 FROM StockList WHERE name = " + W.quote(stockListName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";");
        if (check.empty()) {
            std::cout << "Stocklist does not exist or you are not the owner.\n";
            return;
        }

        std::cout << "Enter the username to share with: ";
        std::cin >> receiverUsername;

        if (receiverUsername == ownerUsername) {
            std::cout << "You can't share with yourself.\n";
            return;
        }

        // Check if user is friends with receiver
        auto friendCheck = W.exec(
            "SELECT 1 FROM Friends WHERE ((senderUsername = " + W.quote(ownerUsername) +
            " AND receiverUsername = " + W.quote(receiverUsername) + ") OR " +
            "(senderUsername = " + W.quote(receiverUsername) +
            " AND receiverUsername = " + W.quote(ownerUsername) + ")) AND state = 'accepted';");

        if (friendCheck.empty()) {
            std::cout << "You're not friends with this user.\n";
            return;
        }

        // Check if already shared
        auto alreadyShared = W.exec(
            "SELECT 1 FROM ShareStockList WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND receiverUsername = " + W.quote(receiverUsername) +
            " AND stockListName = " + W.quote(stockListName) + ";");

        if (!alreadyShared.empty()) {
            std::cout << "Stocklist already shared with this user.\n";
            return;
        }

        W.exec(
            "INSERT INTO ShareStockList (ownerUsername, receiverUsername, stockListName) VALUES (" +
            W.quote(ownerUsername) + ", " + W.quote(receiverUsername) + ", " + W.quote(stockListName) + ");");
        W.commit();
        std::cout << "Stocklist shared successfully with '" << receiverUsername << "'.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
}


void addReviewStockList(const std::string& writerusername) {
    std::string stockListName, ownerUsername;
    std::cout << "Enter the stocklist owner username: ";
    std::cin >> ownerUsername;
    std::cout << "Enter the name of the stocklist: ";
    std::getline(std::cin >> std::ws, stockListName);

    if (ownerUsername == writerusername) {
        std::cout << "You cannot review your own stocklist.\n";
        return;
    }

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check if shared with user
        auto sharedCheck = W.exec(
            "SELECT 1 FROM ShareStockList WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND receiverUsername = " + W.quote(writerusername) +
            " AND stockListName = " + W.quote(stockListName) + ";");

        if (sharedCheck.empty()) {
            std::cout << "This stocklist is not shared with you.\n";
            return;
        }

        auto reviewCheck = W.exec(
            "SELECT 1 FROM Review WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND stockListName = " + W.quote(stockListName) +
            " AND writerusername = " + W.quote(writerusername) + ";");

        if (!reviewCheck.empty()) {
            std::cout << "You already reviewed this stocklist. Do you want to edit it? (y/n): ";
            char choice;
            std::cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                editReviewStockList(writerusername);
            } else {
                std::cout << "Review not modified.\n";
            }
            return;
        }

        std::string content;
        std::cout << "Enter your review: ";
        std::getline(std::cin >> std::ws, content);

        W.exec(
            "INSERT INTO Review (ownerUsername, stockListName, writerusername, content) VALUES (" +
            W.quote(ownerUsername) + ", " + W.quote(stockListName) + ", " +
            W.quote(writerusername) + ", " + W.quote(content) + ");");
        W.commit();
        std::cout << "Review added successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
}


void editReviewStockList(const std::string& writerusername) {
    std::string stockListName, ownerUsername;
    std::cout << "Enter the stocklist owner username: ";
    std::cin >> ownerUsername;
    std::cout << "Enter the name of the stocklist: ";
    std::getline(std::cin >> std::ws, stockListName);

    if (ownerUsername == writerusername) {
        std::cout << "You cannot review your own stocklist.\n";
        return;
    }

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check if shared with user
        auto sharedCheck = W.exec(
            "SELECT 1 FROM ShareStockList WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND receiverUsername = " + W.quote(writerusername) +
            " AND stockListName = " + W.quote(stockListName) + ";");

        if (sharedCheck.empty()) {
            std::cout << "This stocklist is not shared with you.\n";
            return;
        }

        auto reviewCheck = W.exec(
            "SELECT 1 FROM Review WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND stockListName = " + W.quote(stockListName) +
            " AND writerusername = " + W.quote(writerusername) + ";");

        if (reviewCheck.empty()) {
            std::cout << "No review exists. Do you want to add one? (y/n): ";
            char choice;
            std::cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                addReviewStockList(writerusername);
            } else {
                std::cout << "No review added.\n";
            }
            return;
        }

        std::string newText;
        std::cout << "Enter new review text: ";
        std::getline(std::cin >> std::ws, newText);

        W.exec(
            "UPDATE Review SET content = " + W.quote(newText) +
            " WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND stockListName = " + W.quote(stockListName) +
            " AND writerusername = " + W.quote(writerusername) + ";");
        W.commit();
        std::cout << "Review updated successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
}


void deleteReviewStockList(const std::string& writerusername) {
    std::string stockListName, ownerUsername;
    std::cout << "Enter the stocklist owner username: ";
    std::cin >> ownerUsername;
    std::cout << "Enter the name of the stocklist: ";
    std::getline(std::cin >> std::ws, stockListName);

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check if the stocklist exists
        auto stockListCheck = W.exec(
            "SELECT 1 FROM StockList WHERE name = " + W.quote(stockListName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";");
        if (stockListCheck.empty()) {
            std::cout << "The specified stocklist does not exist.\n";
            return;
        }

        std::string targetReviewer = writerusername;

        if (writerusername == ownerUsername) {
            std::cout << "Enter the username of the review to delete: ";
            std::cin >> targetReviewer;
        }

        // Check if review exists
        auto reviewCheck = W.exec(
            "SELECT 1 FROM Review WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND stockListName = " + W.quote(stockListName) +
            " AND writerusername = " + W.quote(targetReviewer) + ";");

        if (reviewCheck.empty()) {
            std::cout << "No review found for this stocklist by '" << targetReviewer << "'.\n";
            return;
        }

        // Delete the review
        W.exec(
            "DELETE FROM Review WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND stockListName = " + W.quote(stockListName) +
            " AND writerusername = " + W.quote(targetReviewer) + ";");

        W.commit();
        std::cout << "Review deleted successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
}


void viewReviewStockList(const std::string& writerusername) {
    std::string stockListName, ownerUsername;
    std::cout << "Enter the stocklist owner username: ";
    std::cin >> ownerUsername;
    std::cout << "Enter the name of the stocklist: ";
    std::getline(std::cin >> std::ws, stockListName);

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check if the stocklist exists
        auto stockListCheck = W.exec(
            "SELECT 1 FROM StockList WHERE name = " + W.quote(stockListName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";");
        if (stockListCheck.empty()) {
            std::cout << "The specified stocklist does not exist.\n";
            return;
        }

        std::string targetReviewer = writerusername;

        if (writerusername == ownerUsername) {
            std::cout << "Enter the username of the review to view: ";
            std::cin >> targetReviewer;
        }

        // Check if review exists
        auto reviewResult = W.exec(
            "SELECT content FROM Review WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND stockListName = " + W.quote(stockListName) +
            " AND writerusername = " + W.quote(targetReviewer) + ";");

        if (reviewResult.empty()) {
            std::cout << "No review found for this stocklist by '" << targetReviewer << "'.\n";
            return;
        }

        std::cout << "Review by '" << targetReviewer << "':\n";
        std::cout << reviewResult[0]["content"].as<std::string>() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
}


void deleteStockList(const std::string& ownerUsername) {
    std::string listName;
    std::cout << "Enter the name of the stocklist you want to delete: ";
    std::getline(std::cin >> std::ws, listName); // to handle whitespace properly

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check if the stocklist exists and belongs to the user
        std::string checkQuery =
            "SELECT 1 FROM StockList WHERE name = " + W.quote(listName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";

        pqxx::result res = W.exec(checkQuery);
        if (res.empty()) {
            std::cout << "You do not own a stocklist named '" << listName << "'.\n";
            return;
        }

        // Delete the stocklist (StockListHasStock and ShareStockList will be auto-deleted via ON DELETE CASCADE)
        std::string deleteQuery =
            "DELETE FROM StockList WHERE name = " + W.quote(listName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";
        W.exec(deleteQuery);
        W.commit();

        std::cout << "Stocklist '" << listName << "' has been successfully deleted.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void viewStockListHistorical(const std::string& ownerUsername) {
    std::string listName;
    std::cout << "Enter your stock list name: ";
    std::getline(std::cin >> std::ws, listName);

    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string query =
            "SELECT stockID FROM StockListHasStock "
            "WHERE stockListName = " + N.quote(listName) +
            " AND ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result holdings = N.exec(query);

        if (holdings.empty()) {
            std::cout << "This stock list has no stocks.\n";
            return;
        }

        std::cout << "Select a stock:\n";
        for (size_t i = 0; i < holdings.size(); ++i) {
            std::cout << i + 1 << ". " << holdings[i]["stockid"].as<std::string>() << "\n";
        }

        int schoice;
        if (!getValidatedInput(schoice)) return;
        if (schoice < 1 || schoice > static_cast<int>(holdings.size())) {
            std::cout << "Invalid stock selection.\n";
            return;
        }

        std::string symbol = holdings[schoice - 1]["stockid"].as<std::string>();

        std::cout << "Select time interval:\n";
        std::cout << "1. Past 1 week\n";
        std::cout << "2. Past 1 month\n";
        std::cout << "3. Past 3 months\n";
        std::cout << "4. Past 1 year\n";
        std::cout << "5. Past 5 years\n";

        int rangeChoice;
        if (!getValidatedInput(rangeChoice)) return;

        std::string interval;
        switch (rangeChoice) {
            case 1: interval = "7 days"; break;
            case 2: interval = "1 month"; break;
            case 3: interval = "3 months"; break;
            case 4: interval = "1 year"; break;
            case 5: interval = "5 years"; break;
            default:
                std::cout << "Invalid interval.\n";
                return;
        }

        std::string sql =
            "SELECT timestamp, close FROM StockHistory "
            "WHERE symbol = " + N.quote(symbol) +
            " AND timestamp BETWEEN DATE '2018-02-07' - INTERVAL '" + interval + "' AND DATE '2018-02-07' "
            "ORDER BY timestamp;";
        pqxx::result prices = N.exec(sql);

        std::vector<std::pair<std::string, double>> history;
        for (const auto& row : prices) {
            history.emplace_back(row["timestamp"].as<std::string>(), row["close"].as<double>());
        }

        if (history.empty()) {
            std::cout << "No historical price data.\n";
            return;
        }

        drawASCII(rangeChoice >= 3 ? downsampleData(history, 30) : history);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void viewStockListPastPerformance(const std::string& ownerUsername) {
    std::string listName;
    std::cout << "Enter your stock list name: ";
    std::getline(std::cin >> std::ws, listName);

    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string stockQuery =
            "SELECT stockID, quantity FROM StockListHasStock "
            "WHERE stockListName = " + N.quote(listName) +
            " AND ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result stocks = N.exec(stockQuery);

        if (stocks.empty()) {
            std::cout << "This stock list has no stocks.\n";
            return;
        }

        std::map<std::string, int> stockQuantities;
        for (const auto& row : stocks) {
            stockQuantities[row["stockid"].as<std::string>()] = row["quantity"].as<int>();
        }

        std::string symbolsInClause = "(";
        for (auto it = stockQuantities.begin(); it != stockQuantities.end(); ++it) {
            if (it != stockQuantities.begin()) symbolsInClause += ", ";
            symbolsInClause += N.quote(it->first);
        }
        symbolsInClause += ")";

        std::string historyQuery =
            "SELECT symbol, timestamp, close FROM StockHistory "
            "WHERE timestamp BETWEEN DATE '2018-02-07' - INTERVAL '1 year' AND DATE '2018-02-07' "
            "AND symbol IN " + symbolsInClause + " ORDER BY timestamp;";

        pqxx::result history = N.exec(historyQuery);

        if (history.empty()) {
            std::cout << "No historical data available for the stock list.\n";
            return;
        }

        std::map<std::string, double> dailyValueMap;

        for (const auto& row : history) {
            std::string date = row["timestamp"].as<std::string>();
            std::string symbol = row["symbol"].as<std::string>();
            double close = row["close"].as<double>();
            int qty = stockQuantities[symbol];
            dailyValueMap[date] += close * qty;
        }

        std::vector<std::pair<std::string, double>> performance;
        for (const auto& entry : dailyValueMap) {
            performance.emplace_back(entry.first, entry.second);
        }

        if (performance.empty()) {
            std::cout << "No data available to display performance.\n";
            return;
        }

        drawASCII(performance.size() >= 60 ? downsampleData(performance, 30) : performance);

    } catch (const std::exception& e) {
        std::cerr << "Error in viewStockListPerformance: " << e.what() << "\n";
    }
}

void viewStockListPrediction(const std::string& ownerUsername) {
    std::string listName;
    std::cout << "Enter your stock list name: ";
    std::getline(std::cin >> std::ws, listName);

    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string query =
            "SELECT stockID FROM StockListHasStock "
            "WHERE stockListName = " + N.quote(listName) +
            " AND ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result holdings = N.exec(query);

        if (holdings.empty()) {
            std::cout << "This stock list has no stocks.\n";
            return;
        }

        std::cout << "Select a stock to predict:\n";
        for (size_t i = 0; i < holdings.size(); ++i) {
            std::cout << i + 1 << ". " << holdings[i]["stockid"].as<std::string>() << "\n";
        }

        int schoice;
        if (!getValidatedInput(schoice)) return;
        if (schoice < 1 || schoice > static_cast<int>(holdings.size())) {
            std::cout << "Invalid stock selection.\n";
            return;
        }

        std::string symbol = holdings[schoice - 1]["stockid"].as<std::string>();

        std::string sql =
            "SELECT timestamp, close FROM StockHistory "
            "WHERE symbol = " + N.quote(symbol) +
            " AND timestamp <= DATE '2018-02-07' "
            "ORDER BY timestamp;";
        pqxx::result prices = N.exec(sql);

        std::vector<std::pair<std::string, double>> history;
        for (const auto& row : prices) {
            history.emplace_back(row["timestamp"].as<std::string>(), row["close"].as<double>());
        }

        if (history.empty()) {
            std::cout << "No historical price data.\n";
            return;
        }

        std::cout << "Enter number of days to predict into the future (1-365): ";
        int days;
        if (!getValidatedInput(days)) return;
        if (days <= 0 || days > 365) {
            std::cout << "Invalid prediction range.\n";
            return;
        }

        auto predictions = predictFuturePrices(history, days);
        drawASCII(predictions);

    } catch (const std::exception& e) {
        std::cerr << "Prediction Error: " << e.what() << "\n";
    }
}
