#include <iostream>
#include <pqxx/pqxx>
#include "global.h"
#include "user.h"

void registerUser() {
    std::string username, password;
    std::cout << "Enter new username: ";
    std::cin >> username;
    std::cout << "Enter new password: ";
    std::cin >> password;

    try {
        pqxx::connection C("dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        std::string userQuery = "INSERT INTO \"User\" (username, password) VALUES (" +
                            W.quote(username) + ", " + W.quote(password) + ");";
        W.exec(userQuery);

        std::string portfolioName = username + "'s portfolio";
        std::string portfolioQuery = "INSERT INTO Portfolio (name, cashAccount, ownerUsername) VALUES (" +
                            W.quote(portfolioName) + ", 0.0, " + W.quote(username) + ");";
        W.exec(portfolioQuery);
        W.commit();

        std::cout << "User registered successfully with default portfolio!\n";
    } catch (const pqxx::sql_error &e) {
        if (std::string(e.what()).find("duplicate key") != std::string::npos) {
            std::cerr << "Username already exists.\n";
        } else {
            std::cerr << "SQL Error: " << e.what() << "\n";
        }
    } catch (const std::exception &e) {
        std::cerr << "General Error: " << e.what() << "\n";
    }
}

void loginUser() {
    std::string username, password;
    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;

    try {
        pqxx::connection C("dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432");
        pqxx::nontransaction N(C);

        std::string query = "SELECT * FROM \"User\" WHERE username = " +
                            N.quote(username) + " AND password = " +
                            N.quote(password) + ";";

        pqxx::result R = N.exec(query);

        if (R.size() == 1) {
            currentUsername = username;
            std::cout << "Login successful! Welcome, " << currentUsername << "\n";
        } else {
            std::cout << "Invalid username or password.\n";
        }
    } catch (const std::exception &e) {
        std::cerr << "Login Error: " << e.what() << "\n";
    }
}
