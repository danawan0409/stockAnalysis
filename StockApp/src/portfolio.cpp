#include <iostream>
#include <pqxx/pqxx>
#include "portfolio.h"

void createPortfolio(const std::string& ownerUsername) {
    std::string name;
    double initialCash;

    std::cout << "Enter portfolio name: ";
    std::cin.ignore();
    std::getline(std::cin, name);

    std::cout << "Enter initial cash amount: ";
    std::cin >> initialCash;

    if (initialCash < 0) {
        std::cout << "Initial cash cannot be negative.\n";
        return;
    }

    try {
        pqxx::connection C("dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        // check if portfolio name already exists for the user
        std::string checkQuery =
            "SELECT 1 FROM Portfolio WHERE name = " + W.quote(name) +
            " AND ownerUsername = " + W.quote(ownerUsername) + ";";

        pqxx::result R = W.exec(checkQuery);
        if (!R.empty()) {
            std::cout << "You already have a portfolio named \"" << name << "\". Please pick another one.\n";
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
        pqxx::connection C("dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432");
        pqxx::nontransaction N(C);

        std::string query =
            "SELECT name, cashAccount FROM Portfolio WHERE ownerUsername = " + N.quote(ownerUsername) + ";";

        pqxx::result R = N.exec(query);

        std::cout << "\nYour Portfolios:\n";
        if (R.empty()) {
            std::cout << "(none)\n";
        } else {
            for (const auto& row : R) {
                std::cout << "Name: " << row["name"].as<std::string>()
                          << ", Cash: $" << row["cashaccount"].as<double>() << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error retrieving portfolios: " << e.what() << std::endl;
    }
}

std::vector<std::pair<std::string, double>> listUserPortfolios(const std::string& ownerUsername) {
    std::vector<std::pair<std::string, double>> portfolios;

    try {
        pqxx::connection C("dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432");
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
    std::cin >> choice;

    if (choice < 1 || choice > static_cast<int>(portfolios.size())) {
        std::cout << "Invalid selection.\n";
        return;
    }

    std::string targetPortfolio = portfolios[choice - 1].first;

    int sourceType;
    std::cout << "Deposit from:\n1. External bank account\n2. Another portfolio cash account\nEnter choice: ";
    std::cin >> sourceType;

    double amount;
    std::cout << "Enter amount to deposit: ";
    std::cin >> amount;
    if (amount <= 0) {
        std::cout << "Invalid amount.\n";
        return;
    }

    try {
        pqxx::connection C("dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        if (sourceType == 1) {
            // Bank deposit (just add to cashAccount)
            std::string update =
                "UPDATE Portfolio SET cashAccount = cashAccount + " + W.quote(amount) +
                " WHERE name = " + W.quote(targetPortfolio) +
                " AND ownerUsername = " + W.quote(ownerUsername) + ";";

            W.exec(update);
            W.commit();
            std::cout << "Deposit from bank successful.\n";

        } else if (sourceType == 2) {
            // Select source portfolio
            std::cout << "Select portfolio to transfer from (other than target): ";
            int fromIndex;
            std::cin >> fromIndex;

            if (fromIndex < 1 || fromIndex > static_cast<int>(portfolios.size()) || fromIndex == choice) {
                std::cout << "Invalid transfer source.\n";
                return;
            }

            std::string sourcePortfolio = portfolios[fromIndex - 1].first;

            // Check if source has enough
            std::string check =
                "SELECT cashAccount FROM Portfolio WHERE name = " + W.quote(sourcePortfolio) +
                " AND ownerUsername = " + W.quote(ownerUsername) + ";";

            pqxx::result R = W.exec(check);
            double available = R[0]["cashaccount"].as<double>();
            if (amount > available) {
                std::cout << "Insufficient funds in source portfolio.\n";
                return;
            }

            // Do transfer
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
        } else {
            std::cout << "Invalid deposit source.\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during deposit: " << e.what() << "\n";
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
    std::cin >> choice;

    if (choice < 1 || choice > static_cast<int>(portfolios.size())) {
        std::cout << "Invalid selection.\n";
        return;
    }

    std::string portfolioName = portfolios[choice - 1].first;
    double currentBalance = portfolios[choice - 1].second;

    double amount;
    std::cout << "Enter amount to withdraw: ";
    std::cin >> amount;

    if (amount <= 0 || amount > currentBalance) {
        std::cout << "Invalid amount. Your balance is $" << currentBalance << "\n";
        return;
    }

    try {
        pqxx::connection C("dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432");
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
