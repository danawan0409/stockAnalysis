#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <iomanip>
#include "portfolio.h"
#include "global.h"
#include "stock.h"

void createPortfolio(const std::string& ownerUsername) {
    std::string name;
    double initialCash;

    std::cout << "Enter portfolio name: ";
    std::cin.ignore();
    std::getline(std::cin, name);

    std::cout << "Enter initial cash amount: ";

    if (!getValidatedInput(initialCash)) return;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // check if portfolio name already exists for the user
        std::string checkQuery =
            "SELECT 1 FROM Portfolio WHERE name = " + W.quote(name) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";

        pqxx::result R = W.exec(checkQuery);
        if (!R.empty()) {
            std::cout << "You already have a portfolio named \"" << name << "\". Please pick another name.\n";
            return;
        }

        // insert new portfolio
        std::string query =
            "INSERT INTO Portfolio (name, cashAccount, ownerUsername) VALUES (" +
            W.quote(name) + ", " + W.quote(initialCash) + ", " + W.quote(ownerUsername) + ");";

        W.exec(query);
        W.commit();

        std::cout << "Portfolio created successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error creating portfolio: " << e.what() << std::endl;
    }
}

void viewPortfolios(const std::string& ownerUsername) {
    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string query =
            "SELECT name, cashAccount FROM Portfolio WHERE ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result portfolios = N.exec(query);

        std::cout << "\nYour Portfolios:\n";
        int index = 1;
        for (const auto& row : portfolios) {
            std::cout << index++ << ". Name: " << row["name"].as<std::string>()
                      << ", Cash: $" << row["cashaccount"].as<double>() << "\n";
        }

        int choice;
        std::cout << "Enter portfolio number to view market value (0 to cancel): ";
        if (!getValidatedInput(choice)) return;

        if (choice == 0) {
            std::cout << "Cancelled.\n";
            return;
        }

        if (choice < 0 || choice > static_cast<int>(portfolios.size())) {
            std::cout << "Invalid selection.\n";
            return;
        }

        std::string selectedPortfolio = portfolios[choice - 1]["name"].as<std::string>();
        double cash = portfolios[choice - 1]["cashaccount"].as<double>();

        std::string stockQuery =
            "SELECT phs.stockID, phs.quantity, s.close "
            "FROM PortfolioHasStock phs "
            "JOIN Stock s ON phs.stockID = s.symbol "
            "WHERE phs.portfolioName = " + N.quote(selectedPortfolio) +
            " AND phs.ownerUsername = " + N.quote(ownerUsername) + ";";

        pqxx::result stocks = N.exec(stockQuery);

        double totalStockValue = 0.0;

        std::cout << "\nPortfolio: " << selectedPortfolio << "\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Cash: $" << std::fixed << std::setprecision(2) << cash << "\n\n";

        if (stocks.empty()) {
            std::cout << "(no stocks held)\n";
        } else {
            std::cout << "Holdings:\n";
            std::cout << std::left << std::setw(10) << "Symbol"
                      << std::setw(10) << "Quantity"
                      << std::setw(12) << "Price"
                      << std::setw(12) << "Value" << "\n";
            std::cout << "--------------------------------------------------\n";

            for (const auto& stock : stocks) {
                std::string symbol = stock["stockid"].as<std::string>();
                int qty = stock["quantity"].as<int>();
                double price = stock["close"].as<double>();
                double value = qty * price;
                totalStockValue += value;

                std::cout << std::left << std::setw(10) << symbol
                          << std::setw(10) << qty
                          << "$" << std::setw(11) << std::fixed << std::setprecision(2) << price
                          << "$" << std::setw(11) << std::fixed << std::setprecision(2) << value
                          << "\n";
            }
            std::cout << "\nTotal Stock Value: $" << std::fixed << std::setprecision(2) << totalStockValue << "\n";
        }
        std::cout << "----------------------------------------\n";
        std::cout << "Portfolio Market Value: $" << std::fixed << std::setprecision(2)
                  << (cash + totalStockValue) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error retrieving portfolios: " << e.what() << std::endl;
    }
}

void deletePortfolio(const std::string& ownerUsername) {
    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // get all portfolios for the user
        std::string query = "SELECT name FROM Portfolio WHERE ownerUsername = " + W.quote(ownerUsername) + ";";
        pqxx::result portfolios = W.exec(query);

        if (portfolios.size() <= 1) {
            std::cout << "You must have at least one portfolio. Deletion not allowed.\n";
            return;
        }

        std::cout << "Your portfolios:\n";
        int idx = 1;
        for (const auto& row : portfolios) {
            std::cout << idx++ << ". " << row["name"].as<std::string>() << "\n";
        }

        std::cout << "Enter number of portfolio to delete: ";
        int choice;
        if (!getValidatedInput(choice)) return;
        if (choice < 1 || choice > static_cast<int>(portfolios.size())) {
            std::cout << "Invalid selection.\n";
            return;
        }

        std::string targetPortfolio = portfolios[choice - 1]["name"].as<std::string>();

        // check if portfolio has zero cash and no stock holdings
        std::string checkQuery =
            "SELECT 1 FROM Portfolio p "
            "WHERE p.name = " + W.quote(targetPortfolio) + " AND p.ownerUsername = " + W.quote(ownerUsername) +
            " AND p.cashAccount = 0 "
            " AND NOT EXISTS ( "
            "     SELECT 1 FROM PortfolioHasStock "
            "     WHERE portfolioName = " + W.quote(targetPortfolio) +
            "     AND ownerUsername = " + W.quote(ownerUsername) +
            " );";

        pqxx::result check = W.exec(checkQuery);
        if (check.empty()) {
            std::cout << "Cannot delete: portfolio must have zero cash and no stock holdings.\n";
            return;
        }

        // delete the portfolio
        std::string deleteQuery =
            "DELETE FROM Portfolio WHERE name = " + W.quote(targetPortfolio) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";

        W.exec(deleteQuery);
        W.commit();

        std::cout << "Portfolio \"" << targetPortfolio << "\" deleted successfully.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

std::vector<std::pair<std::string, double>> listUserPortfolios(const std::string& ownerUsername) {
    std::vector<std::pair<std::string, double>> portfolios;

    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string query =
            "SELECT name, cashAccount FROM Portfolio WHERE ownerUsername = " + N.quote(ownerUsername) + ";";

        pqxx::result R = N.exec(query);

        std::cout << "\nYour Portfolios:\n";
        int index = 1;
        for (const auto& row : R) {
            std::string name = row["name"].as<std::string>();
            double cash = row["cashaccount"].as<double>();
            std::cout << index << ". " << name << " ($" << cash << ")\n";
            portfolios.emplace_back(name, cash);
            index++;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error listing portfolios: " << e.what() << "\n";
    }

    return portfolios;
}

void depositCash(const std::string& ownerUsername) {
    auto portfolios = listUserPortfolios(ownerUsername);
    if (portfolios.empty()) {
        std::cout << "You have no portfolios.\n";
        return;
    }

    int choice;
    std::cout << "Select a portfolio to deposit into (enter number): ";
    if (!getValidatedInput(choice)) return;

    if (choice < 1 || choice > static_cast<int>(portfolios.size())) {
        std::cout << "Invalid selection.\n";
        return;
    }

    std::string targetPortfolio = portfolios[choice - 1].first;

    int sourceType;
    std::cout << "Deposit from:\n1. External bank account\n2. Another portfolio cash account\nEnter choice: ";
    if (!getValidatedInput(sourceType)) return;

    if (sourceType == 1) {
        double amount;
        std::cout << "Enter amount to deposit (0 to cancel): ";
        if (!getValidatedInput(amount)) return;
        if (amount == 0) {
            std::cout << "Deposit cancelled.\n";
            return;
        }
        if (amount < 0) {
            std::cout << "Invalid amount.\n";
            return;
        }

        try {
            pqxx::connection C(connect_info);
            pqxx::work W(C);

            std::string update =
                "UPDATE Portfolio SET cashAccount = cashAccount + " + W.quote(amount) +
                " WHERE name = " + W.quote(targetPortfolio) +
                " AND ownerUsername = " + W.quote(ownerUsername) + ";";

            W.exec(update);
            W.commit();
            std::cout << "Deposit from bank successful.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error during deposit: " << e.what() << "\n";
        }
    } else if (sourceType == 2) {
        std::cout << "Select source portfolio number (note: from portfolio above): ";
        int fromIndex;
        if (!getValidatedInput(fromIndex)) return;

        if (fromIndex < 1 || fromIndex > static_cast<int>(portfolios.size()) || fromIndex == choice) {
            std::cout << "Invalid transfer source.\n";
            return;
        }

        std::string sourcePortfolio = portfolios[fromIndex - 1].first;

        double amount;
        std::cout << "Enter amount to deposit (enter 0 to cancel): ";
        if (!getValidatedInput(amount)) return;

        if (amount == 0) {
            std::cout << "Transfer cancelled.\n";
            return;
        }
        if (amount < 0) {
            std::cout << "Invalid amount.\n";
            return;
        }

        try {
            pqxx::connection C(connect_info);
            pqxx::work W(C);

            std::string check =
                "SELECT cashAccount FROM Portfolio WHERE name = " + W.quote(sourcePortfolio) +
                " AND ownerUsername = " + W.quote(ownerUsername) + ";";

            pqxx::result R = W.exec(check);
            double available = R[0]["cashaccount"].as<double>();
            if (amount > available) {
                std::cout << "Insufficient funds in source portfolio.\n";
                return;
            }

            std::string deduct =
                "UPDATE Portfolio SET cashAccount = cashAccount - " + W.quote(amount) +
                " WHERE name = " + W.quote(sourcePortfolio) +
                " AND ownerUsername = " + W.quote(ownerUsername) + ";";

            std::string add =
                "UPDATE Portfolio SET cashAccount = cashAccount + " + W.quote(amount) +
                " WHERE name = " + W.quote(targetPortfolio) +
                " AND ownerUsername = " + W.quote(ownerUsername) + ";";

            W.exec(deduct);
            W.exec(add);
            W.commit();

            std::cout << "Transferred $" << amount << " from " << sourcePortfolio
                      << " to " << targetPortfolio << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error during deposit: " << e.what() << "\n";
        }
    } else {
        std::cout << "Invalid deposit source.\n";
    }
}

void withdrawCash(const std::string& ownerUsername) {
    auto portfolios = listUserPortfolios(ownerUsername);
    if (portfolios.empty()) {
        std::cout << "You have no portfolios.\n";
        return;
    }

    int choice;
    std::cout << "Select a portfolio to withdraw from: ";
    if (!getValidatedInput(choice)) return;

    if (choice < 1 || choice > static_cast<int>(portfolios.size())) {
        std::cout << "Invalid selection.\n";
        return;
    }

    std::string portfolioName = portfolios[choice - 1].first;
    double currentBalance = portfolios[choice - 1].second;

    double amount;
    std::cout << "Enter amount to withdraw (0 to cancel): ";
    if (!getValidatedInput(amount)) return;

    if (amount == 0) {
        std::cout << "Withdraw cancelled.\n";
        return;
    }
    if (amount < 0 || amount > currentBalance) {
        std::cout << "Invalid amount. Your balance is $" << currentBalance << "\n";
        return;
    }

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string update =
            "UPDATE Portfolio SET cashAccount = cashAccount - " + W.quote(amount) +
            " WHERE name = " + W.quote(portfolioName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";

        W.exec(update);
        W.commit();
        std::cout << "Withdrawal successful. $" << amount << " withdrawn from " << portfolioName << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void buyStock(const std::string& ownerUsername) {
    auto portfolios = listUserPortfolios(ownerUsername);

    int choice;
    std::cout << "Select portfolio by number: ";
    if (!getValidatedInput(choice)) return;

    if (choice < 1 || choice > static_cast<int>(portfolios.size())) {
        std::cout << "Invalid portfolio choice.\n";
        return;
    }

    std::string portfolioName = portfolios[choice - 1].first;
    double currentCash = portfolios[choice - 1].second;

    std::string stockSymbol;
    std::cout << "Enter stock symbol: ";
    if (!getValidatedInput(stockSymbol)) return;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // search for stock close price
        std::string getPrice =
            "SELECT close FROM Stock WHERE symbol = " + W.quote(stockSymbol) + ";";
        pqxx::result priceResult = W.exec(getPrice);
        if (priceResult.empty()) {
            std::cout << "Stock symbol not found.\n";
            return;
        }

        double price = priceResult[0]["close"].as<double>();
        std::cout << "Current price of " << stockSymbol << ": $" << price << "\n";

        int quantity;
        std::cout << "Enter quantity to buy (0 to cancel): ";
        if (!getValidatedInput(quantity)) return;
        if (quantity == 0) {
            std::cout << "Buy operation canceled.\n";
            return;
        }
        if (quantity < 0) {
            std::cout << "Quantity must be positive.\n";
            return;
        }

        double totalCost = price * quantity;

        if (totalCost > currentCash) {
            std::cout << "Not enough cash. You need $" << totalCost << ", but have only $" << currentCash << "\n";
            return;
        }

        std::string insertStock =
            "INSERT INTO PortfolioHasStock (portfolioName, ownerUsername, stockID, quantity) "
            "VALUES (" + W.quote(portfolioName) + ", " + W.quote(ownerUsername) + ", " + W.quote(stockSymbol) + ", " +
            std::to_string(quantity) + ") "
            "ON CONFLICT (portfolioName, ownerUsername, stockID) "
            "DO UPDATE SET quantity = PortfolioHasStock.quantity + EXCLUDED.quantity;";

        // deduct from cash account
        std::string updateCash =
            "UPDATE Portfolio SET cashAccount = cashAccount - " + std::to_string(totalCost) +
            " WHERE name = " + W.quote(portfolioName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";

        W.exec(insertStock);
        W.exec(updateCash);
        W.commit();

        std::cout << "Bought " << quantity << " shares of " << stockSymbol
                  << " at $" << price << " each, total cost $" << totalCost << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error during stock purchase: " << e.what() << "\n";
    }
}

void sellStock(const std::string& ownerUsername) {
    auto portfolios = listUserPortfolios(ownerUsername);

    int choice;
    std::cout << "Select portfolio by number: ";
    if (!getValidatedInput(choice)) return;

    if (choice < 1 || choice > static_cast<int>(portfolios.size())) {
        std::cout << "Invalid portfolio choice.\n";
        return;
    }

    std::string portfolioName = portfolios[choice - 1].first;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string holdingQuery =
            "SELECT phs.stockID, phs.quantity, s.close "
            "FROM PortfolioHasStock phs "
            "JOIN Stock s ON phs.stockID = s.symbol "
            "WHERE phs.portfolioName = " + W.quote(portfolioName) +
            " AND phs.ownerUsername = " + W.quote(ownerUsername) + ";";

        pqxx::result holdings = W.exec(holdingQuery);

        std::cout << "\nHoldings in \"" << portfolioName << "\":\n";
        if (holdings.empty()) {
            std::cout << "(no stocks held in this portfolio)\n";
            return;
        } else {
            std::cout << std::left << std::setw(10) << "Symbol"
                      << std::setw(10) << "Quantity"
                      << std::setw(12) << "Price"
                      << std::setw(12) << "Value" << "\n";
            std::cout << "--------------------------------------------------\n";

            for (const auto& row : holdings) {
                std::string symbol = row["stockid"].as<std::string>();
                int qty = row["quantity"].as<int>();
                double price = row["close"].as<double>();
                double value = qty * price;

                std::cout << std::left << std::setw(10) << symbol
                          << std::setw(10) << qty
                          << "$" << std::setw(11) << std::fixed << std::setprecision(2) << price
                          << "$" << std::setw(11) << std::fixed << std::setprecision(2) << value
                          << "\n";
            }
        }

        std::string stockSymbol;
        std::cout << "\nEnter stock symbol to sell: ";
        if (!getValidatedInput(stockSymbol)) return;

        std::string getPrice =
            "SELECT close FROM Stock WHERE symbol = " + W.quote(stockSymbol) + ";";
        pqxx::result priceResult = W.exec(getPrice);
        if (priceResult.empty()) {
            std::cout << "Stock symbol not found.\n";
            return;
        }
        double price = priceResult[0]["close"].as<double>();
        std::cout << "Current price of " << stockSymbol << ": $" << price << "\n";

        std::string getHolding =
            "SELECT quantity FROM PortfolioHasStock "
            "WHERE portfolioName = " + W.quote(portfolioName) +
            " AND ownerUsername = " + W.quote(ownerUsername) +
            " AND stockID = " + W.quote(stockSymbol) + ";";
        pqxx::result holding = W.exec(getHolding);
        if (holding.empty()) {
            std::cout << "You do not own any shares of " << stockSymbol << " in this portfolio.\n";
            return;
        }

        int owned = holding[0]["quantity"].as<int>();
        std::cout << "You currently own " << owned << " shares.\n";

        int quantity;
        std::cout << "Enter quantity to sell (0 to cancel): ";
        if (!getValidatedInput(quantity)) return;
        if (quantity == 0) {
            std::cout << "Sell operation canceled.\n";
            return;
        }
        if (quantity < 0 || quantity > owned) {
            std::cout << "Invalid quantity. You only have " << owned << " shares.\n";
            return;
        }

        double totalGain = price * quantity;

        std::string updateCash =
            "UPDATE Portfolio SET cashAccount = cashAccount + " + std::to_string(totalGain) +
            " WHERE name = " + W.quote(portfolioName) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";

        std::string updateHolding;
        if (quantity == owned) {
            updateHolding =
                "DELETE FROM PortfolioHasStock "
                "WHERE portfolioName = " + W.quote(portfolioName) +
                " AND ownerUsername = " + W.quote(ownerUsername) +
                " AND stockID = " + W.quote(stockSymbol) + ";";
        } else {
            updateHolding =
                "UPDATE PortfolioHasStock SET quantity = quantity - " + std::to_string(quantity) +
                " WHERE portfolioName = " + W.quote(portfolioName) +
                " AND ownerUsername = " + W.quote(ownerUsername) +
                " AND stockID = " + W.quote(stockSymbol) + ";";
        }

        W.exec(updateCash);
        W.exec(updateHolding);
        W.commit();

        std::cout << "Sold " << quantity << " shares of " << stockSymbol
                  << " at $" << std::fixed << std::setprecision(2) << price
                  << " each, total gain $" << totalGain << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error during stock sale: " << e.what() << "\n";
    }
}

void viewPortfolioHistorical(const std::string& ownerUsername) {
    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string queryPortfolio =
            "SELECT name FROM Portfolio WHERE ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result portfolios = N.exec(queryPortfolio);

        std::cout << "Select a portfolio:\n";
        for (size_t i = 0; i < portfolios.size(); ++i) {
            std::cout << i + 1 << ". " << portfolios[i]["name"].as<std::string>() << "\n";
        }

        int pchoice;
        if (!getValidatedInput(pchoice)) return;
        if (pchoice < 1 || pchoice > static_cast<int>(portfolios.size())) {
            std::cout << "Invalid selection.\n";
            return;
        }

        std::string portfolioName = portfolios[pchoice - 1]["name"].as<std::string>();

        std::string queryHoldings =
            "SELECT stockID FROM PortfolioHasStock WHERE portfolioName = " + N.quote(portfolioName) +
            " AND ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result holdings = N.exec(queryHoldings);

        if (holdings.empty()) {
            std::cout << "No stocks in this portfolio.\n";
            return;
        }

        std::cout << "Select a stock:\n";
        for (size_t i = 0; i < holdings.size(); ++i) {
            std::cout << i + 1 << ". " << holdings[i]["stockid"].as<std::string>() << "\n";
        }

        int schoice;
        if (!getValidatedInput(schoice)) return;
        if (schoice < 1 || schoice > static_cast<int>(holdings.size())) {
            std::cout << "Invalid selection.\n";
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

        std::vector<std::pair<std::string, double>> date_price;
        for (const auto& row : prices) {
            std::string date = row["timestamp"].as<std::string>();
            double close = row["close"].as<double>();
            date_price.emplace_back(date, close);
        }

        if (date_price.empty()) {
            std::cout << "No historical price data found for this range.\n";
            return;
        }

        if (rangeChoice >= 3) {
            drawASCII(downsampleData(date_price, 30));
        } else {
            drawASCII(date_price);
        }

        // Ask user if they want to display the table
        std::string showTable;
        std::cout << "Would you like to display the raw data table? (yes/no): ";
        if (!getValidatedInput(showTable)) return;

        if (showTable == "yes" || showTable == "y" || showTable == "Y") {
            std::cout << "\nDate        | Close Price\n";
            std::cout << "--------------------------\n";
        
            int count = 0;
            for (size_t i = 0; i < date_price.size(); ++i) {
                std::cout << date_price[i].first << " | $"
                          << std::fixed << std::setprecision(2)
                          << date_price[i].second << "\n";
        
                count++;
                if (count % 20 == 0 && i + 1 < date_price.size()) {
                    std::string cont;
                    std::cout << "\nShow more? (yes/no): ";
                    if (!getValidatedInput(cont)) return;
                    if (cont != "yes" && cont != "y" && cont != "Y") {
                        break;
                    }
                    std::cout << "\nDate        | Close Price\n";
                    std::cout << "--------------------------\n";
                }
            }
        }        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void viewPortfolioPastPerformance(const std::string& ownerUsername) {
    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string queryPortfolio =
            "SELECT name FROM Portfolio WHERE ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result portfolios = N.exec(queryPortfolio);

        std::cout << "Select a portfolio:\n";
        for (size_t i = 0; i < portfolios.size(); ++i) {
            std::cout << i + 1 << ". " << portfolios[i]["name"].as<std::string>() << "\n";
        }

        int pchoice;
        if (!getValidatedInput(pchoice)) return;
        if (pchoice < 1 || pchoice > static_cast<int>(portfolios.size())) {
            std::cout << "Invalid selection.\n";
            return;
        }

        std::string portfolioName = portfolios[pchoice - 1]["name"].as<std::string>();

        std::string queryHoldings =
            "SELECT stockID, quantity FROM PortfolioHasStock "
            "WHERE portfolioName = " + N.quote(portfolioName) +
            " AND ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result holdings = N.exec(queryHoldings);

        if (holdings.empty()) {
            std::cout << "This portfolio holds no stocks.\n";
            return;
        }

        std::map<std::string, int> stockQuantities;
        for (const auto& row : holdings) {
            stockQuantities[row["stockid"].as<std::string>()] = row["quantity"].as<int>();
        }

        std::map<std::string, std::map<std::string, double>> stockHistory;

        for (const auto& [symbol, _] : stockQuantities) {
            std::string query =
                "SELECT timestamp, close FROM StockHistory "
                "WHERE symbol = " + N.quote(symbol) +
                " AND timestamp <= DATE '2018-02-07' "
                "ORDER BY timestamp;";
            pqxx::result result = N.exec(query);
            for (const auto& row : result) {
                std::string date = row["timestamp"].as<std::string>();
                double price = row["close"].as<double>();
                stockHistory[date][symbol] = price;
            }
        }

        std::map<std::string, double> portfolioValueByDate;
        for (const auto& [date, prices] : stockHistory) {
            double total = 0.0;
            for (const auto& [symbol, qty] : stockQuantities) {
                if (prices.find(symbol) != prices.end()) {
                    total += prices.at(symbol) * qty;
                }
            }
            portfolioValueByDate[date] = total;
        }

        std::vector<std::pair<std::string, double>> valueData;
        for (const auto& [date, value] : portfolioValueByDate) {
            valueData.emplace_back(date, value);
        }

        if (valueData.size() > 30) {
            valueData = downsampleData(valueData, 30);
        }

        drawASCII(valueData);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void viewPortfolioPrediction(const std::string& ownerUsername) {
    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string queryPortfolio =
            "SELECT name FROM Portfolio WHERE ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result portfolios = N.exec(queryPortfolio);

        if (portfolios.empty()) {
            std::cout << "You have no portfolios.\n";
            return;
        }

        std::cout << "Select a portfolio:\n";
        for (size_t i = 0; i < portfolios.size(); ++i) {
            std::cout << i + 1 << ". " << portfolios[i]["name"].as<std::string>() << "\n";
        }

        int pchoice;
        if (!getValidatedInput(pchoice)) return;
        if (pchoice < 1 || pchoice > static_cast<int>(portfolios.size())) {
            std::cout << "Invalid selection.\n";
            return;
        }

        std::string portfolioName = portfolios[pchoice - 1]["name"].as<std::string>();

        std::string queryHoldings =
            "SELECT stockID FROM PortfolioHasStock WHERE portfolioName = " + N.quote(portfolioName) +
            " AND ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result holdings = N.exec(queryHoldings);

        if (holdings.empty()) {
            std::cout << "No stocks in this portfolio.\n";
            return;
        }

        std::cout << "Select a stock to predict:\n";
        for (size_t i = 0; i < holdings.size(); ++i) {
            std::cout << i + 1 << ". " << holdings[i]["stockid"].as<std::string>() << "\n";
        }

        int schoice;
        if (!getValidatedInput(schoice)) return;
        if (schoice < 1 || schoice > static_cast<int>(holdings.size())) {
            std::cout << "Invalid stock choice.\n";
            return;
        }

        std::string symbol = holdings[schoice - 1]["stockid"].as<std::string>();

        std::string sql =
            "SELECT timestamp, close FROM StockHistory "
            "WHERE symbol = " + N.quote(symbol) +
            " AND timestamp <= DATE '2018-02-07' "
            "ORDER BY timestamp;";
        pqxx::result R = N.exec(sql);

        std::vector<std::pair<std::string, double>> history;
        for (const auto& row : R) {
            std::string date = row["timestamp"].as<std::string>();
            double close = row["close"].as<double>();
            history.emplace_back(date, close);
        }

        if (history.empty()) {
            std::cout << "No historical data available.\n";
            return;
        }

        // predict future prices
        std::cout << "Enter number of days to predict into the future (range: 1-365 days): ";
        int days;
        if (!getValidatedInput(days)) return;
        if (days <= 0 || days > 365) {
            std::cout << "Please enter a number between 1 and 365.\n";
            return;
        }

        auto predicted = predictFuturePrices(history, days);
        drawASCII(predicted);

    } catch (const std::exception& e) {
        std::cerr << "Prediction Error: " << e.what() << "\n";
    }
}

void viewPortfolioPredictionPerformance(const std::string& ownerUsername) {
    try {
        pqxx::connection C(connect_info);
        pqxx::nontransaction N(C);

        std::string queryPortfolio =
            "SELECT name FROM Portfolio WHERE ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result portfolios = N.exec(queryPortfolio);

        if (portfolios.empty()) {
            std::cout << "You have no portfolios.\n";
            return;
        }

        std::cout << "Select a portfolio:\n";
        for (size_t i = 0; i < portfolios.size(); ++i) {
            std::cout << i + 1 << ". " << portfolios[i]["name"].as<std::string>() << "\n";
        }

        int pchoice;
        if (!getValidatedInput(pchoice)) return;
        if (pchoice < 1 || pchoice > static_cast<int>(portfolios.size())) {
            std::cout << "Invalid selection.\n";
            return;
        }

        std::string portfolioName = portfolios[pchoice - 1]["name"].as<std::string>();

        std::string queryHoldings =
            "SELECT stockID, quantity FROM PortfolioHasStock "
            "WHERE portfolioName = " + N.quote(portfolioName) +
            " AND ownerUsername = " + N.quote(ownerUsername) + ";";
        pqxx::result holdings = N.exec(queryHoldings);

        if (holdings.empty()) {
            std::cout << "No stocks in this portfolio.\n";
            return;
        }

        std::cout << "Enter number of days to predict into the future (1-365): ";
        int days;
        if (!getValidatedInput(days)) return;
        if (days <= 0 || days > 365) {
            std::cout << "Invalid days range.\n";
            return;
        }

        std::map<std::string, int> symbolQuantity;
        for (const auto& row : holdings) {
            std::string symbol = row["stockid"].as<std::string>();
            int qty = row["quantity"].as<int>();
            symbolQuantity[symbol] = qty;
        }

        std::map<std::string, double> dateToValue;

        for (const auto& [symbol, qty] : symbolQuantity) {
            std::string sql =
                "SELECT timestamp, close FROM StockHistory "
                "WHERE symbol = " + N.quote(symbol) +
                " AND timestamp <= DATE '2018-02-07' "
                "ORDER BY timestamp;";
            pqxx::result historyResult = N.exec(sql);

            std::vector<std::pair<std::string, double>> history;
            for (const auto& row : historyResult) {
                history.emplace_back(row["timestamp"].as<std::string>(), row["close"].as<double>());
            }

            if (history.empty()) continue;

            auto predictions = predictFuturePrices(history, days);

            for (const auto& [date, price] : predictions) {
                dateToValue[date] += price * qty;
            }
        }

        if (dateToValue.empty()) {
            std::cout << "Unable to generate predictions for any stock.\n";
            return;
        }

        std::vector<std::pair<std::string, double>> value_series;
        for (const auto& [date, value] : dateToValue) {
            value_series.emplace_back(date, value);
        }

        std::cout << "\nPredicted Portfolio Market Value for " << days << " days:\n";
        drawASCII(value_series);

    } catch (const std::exception& e) {
        std::cerr << "Prediction Error: " << e.what() << "\n";
    }
}
