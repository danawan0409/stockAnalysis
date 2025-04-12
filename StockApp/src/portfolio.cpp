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

    try {
        pqxx::connection C("dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

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
