#include <iostream>
#include <pqxx/pqxx>
#include "user.h"

void registerUser() {
    std::string username, password;
    std::cout << "Enter new username: ";
    std::cin >> username;
    std::cout << "Enter new password: ";
    std::cin >> password;

    try {
        pqxx::connection C("dbname=cppdb user=postgres password=123 hostaddr=127.0.0.1 port=5432");
        pqxx::work W(C);

        std::string query = "INSERT INTO \"User\" (username, password) VALUES (" +
                            W.quote(username) + ", " + W.quote(password) + ");";

        W.exec(query);
        W.commit();

        std::cout << "User registered successfully!\n";
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
            std::cout << "Login successful!\n";
        } else {
            std::cout << "Invalid username or password.\n";
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
