#include <iostream>
#include <pqxx/pqxx>
#include "stocklist.h"
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include "global.h"
#include <iomanip>


void findVariation(){
    return; 
}


void findBeta(){
    return; 
}


void printMatrix(const std::map<std::string, std::map<std::string, double>>& matrix, const std::string& label) {
    std::vector<std::string> symbols;
    for (const auto& row : matrix) symbols.push_back(row.first);

    std::cout << "\n" << label << " Matrix:\n   ";
    for (const auto& sym : symbols) std::cout << std::setw(8) << sym;
    std::cout << "\n";

    for (const auto& sym1 : symbols) {
        std::cout << std::setw(3) << sym1 << " ";
        for (const auto& sym2 : symbols) {
            std::cout << std::setw(8) << std::fixed << std::setprecision(2)
                      << matrix.at(sym1).at(sym2);
        }
        std::cout << "\n";
    }
}

void findStockListMatrix(const std::string& matrixType) {
    std::string stockListName, ownerUsername;
    std::cout << "Enter the stocklist owner username: ";
    std::cin >> ownerUsername;
    std::cout << "Enter the stocklist name: ";
    std::getline(std::cin >> std::ws, stockListName);

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        auto check = W.exec("SELECT 1 FROM StockList WHERE name = " + W.quote(stockListName) +
                            " AND ownerUsername = " + W.quote(ownerUsername) + ";");
        if (check.empty()) {
            std::cout << "Stock list does not exist.\n";
            return;
        }

        std::string query =
            "SELECT sym1, sym2, " + matrixType + " FROM CachedMatrix "
            "WHERE ownerUsername = " + W.quote(ownerUsername) +
            " AND stockListName = " + W.quote(stockListName) + ";";

        pqxx::result res = W.exec(query);
        if (res.empty()) {
            std::cout << "Matrix not cached. Please compute and cache it first.\n";
            return;
        }

        std::map<std::string, std::map<std::string, double>> matrix;
        for (const auto& row : res) {
            std::string s1 = row["sym1"].c_str();
            std::string s2 = row["sym2"].c_str();
            double val = row[matrixType].as<double>();
            matrix[s1][s2] = val;
            matrix[s2][s1] = val; // symmetric
        }

        printMatrix(matrix, matrixType);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void findStockListCovarianceMatrix() {
    findStockListMatrix("covariance");
}

void findStockListCorrelationMatrix() {
    findStockListMatrix("correlation");
}


