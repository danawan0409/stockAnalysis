#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <map>
#include <iomanip>
#include "json.hpp"
#include "global.h"

using json = nlohmann::json;

void findStatistic(const std::string& statName) {
    std::string choice;
    std::cout << "Do you want to find the " << statName << " of a stock in a (1) Portfolio or (2) StockList? ";
    std::cin >> choice;

    std::string symbol, username, name;
    bool valid = false;

    try {
        pqxx::connection C("your_connection_string_here");
        pqxx::work W(C);

        if (choice == "1") {
            std::cout << "Enter your username: ";
            std::cin >> username;
            std::cout << "Enter the name of the portfolio: ";
            std::cin.ignore();
            std::getline(std::cin, name);

            pqxx::result res = W.exec(
                "SELECT 1 FROM Portfolio WHERE name = " + W.quote(name) +
                " AND ownerUsername = " + W.quote(username) + ";"
            );

            if (res.empty()) {
                std::cout << "Portfolio not found.\n";
                return;
            }

            std::cout << "Enter the stock symbol: ";
            std::cin >> symbol;

            pqxx::result inPortfolio = W.exec(
                "SELECT 1 FROM PortfolioHasStock WHERE portfolioName = " + W.quote(name) +
                " AND ownerUsername = " + W.quote(username) +
                " AND stockID = " + W.quote(symbol) + ";"
            );

            if (inPortfolio.empty()) {
                std::cout << "Stock is not in the portfolio.\n";
                return;
            }

            valid = true;

        } else if (choice == "2") {
            std::cout << "Enter your username: ";
            std::cin >> username;
            std::cout << "Enter the owner of the stock list: ";
            std::string owner;
            std::cin >> owner;
            std::cout << "Enter the name of the stock list: ";
            std::cin.ignore();
            std::getline(std::cin, name);

            pqxx::result res = W.exec(
                "SELECT 1 FROM StockList WHERE name = " + W.quote(name) +
                " AND ownerUsername = " + W.quote(owner) + ";"
            );

            if (res.empty()) {
                std::cout << "Stock list not found.\n";
                return;
            }

            pqxx::result access = W.exec(
                "SELECT 1 FROM StockList WHERE name = " + W.quote(name) +
                " AND ownerUsername = " + W.quote(owner) +
                " AND (visibility = 'public' OR ownerUsername = " + W.quote(username) +
                " OR EXISTS (SELECT 1 FROM ShareStockList WHERE ownerUsername = " + W.quote(owner) +
                " AND receiverUsername = " + W.quote(username) +
                " AND stockListName = " + W.quote(name) + "));"
            );

            if (access.empty()) {
                std::cout << "You do not have access to this stock list.\n";
                return;
            }

            std::cout << "Enter the stock symbol: ";
            std::cin >> symbol;

            pqxx::result inList = W.exec(
                "SELECT 1 FROM StockListHasStock WHERE stockListName = " + W.quote(name) +
                " AND ownerUsername = " + W.quote(owner) +
                " AND stockID = " + W.quote(symbol) + ";"
            );

            if (inList.empty()) {
                std::cout << "Stock is not in the stock list.\n";
                return;
            }

            valid = true;
        } else {
            std::cout << "Invalid choice.\n";
            return;
        }

        if (valid) {
            pqxx::result result = W.exec(
                "SELECT " + statName + " FROM CachedStockStatistics WHERE symbol = " + W.quote(symbol) + ";"
            );

            if (result.empty()) {
                std::cout << "Statistic not cached yet for stock " << symbol << ".\n";
            } else {
                std::cout << statName << " for " << symbol << " is: " << result[0][0].as<std::string>() << "\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << "\n";
    }
}

void findVariation() {
    findStatistic("correlation");
}

void findBeta() {
    findStatistic("beta");
}

bool hasAccessToStockList(pqxx::work& W, const std::string& user, const std::string& owner, const std::string& listName) {
    std::string accessQuery = R"(
        SELECT 1 FROM StockList S
        LEFT JOIN ShareStockList SS ON S.name = SS.stockListName AND S.ownerUsername = SS.ownerUsername
        WHERE S.name = )" + W.quote(listName) + " AND S.ownerUsername = " + W.quote(owner) + R"(
        AND (
            S.ownerUsername = )" + W.quote(user) + R"( OR
            SS.receiverUsername = )" + W.quote(user) + R"( OR
            S.visibility = 'public'
        )
        LIMIT 1;
    )";

    pqxx::result res = W.exec(accessQuery);
    return !res.empty();
}

void drawMatrix(const json& matrix) {
    std::vector<std::string> labels;
    for (const auto& [key, _] : matrix.items()) {
        labels.push_back(key);
    }

    std::cout << std::setw(10) << " ";
    for (const auto& label : labels) {
        std::cout << std::setw(10) << label;
    }
    std::cout << "\n";

    for (const auto& row : labels) {
        std::cout << std::setw(10) << row;
        for (const auto& col : labels) {
            double val = matrix[row][col];
            std::cout << std::setw(10) << std::fixed << std::setprecision(2) << val;
        }
        std::cout << "\n";
    }
}

void findMatrix(const std::string& matrixType) {
    std::string owner, list;
    std::cout << "Enter stock list owner: ";
    std::cin >> owner;
    std::cout << "Enter stock list name: ";
    std::getline(std::cin >> std::ws, list);

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        // Check access
        if (!hasAccessToStockList(W, currentUsername, owner, list)) {
            std::cout << "Access denied.\n";
            return;
        }

        // Try cached matrix
        std::string cacheQuery =
            "SELECT matrixData FROM CachedMatrix WHERE stockListName = " + W.quote(list) +
            " AND ownerUsername = " + W.quote(owner) +
            " AND matrixType = " + W.quote(matrixType) + ";";

        pqxx::result cached = W.exec(cacheQuery);
        if (!cached.empty()) {
            json matrix = json::parse(cached[0]["matrixdata"].c_str());
            std::cout << matrixType << " matrix (cached):\n";
            drawMatrix(matrix);
            return;
        }

        // Compute matrix in SQL
        std::string sql =
            "WITH symbols AS ("
            "  SELECT stockID FROM StockListHasStock "
            "  WHERE stockListName = " + W.quote(list) + " AND ownerUsername = " + W.quote(owner) +
            "), "
            "prices AS ("
            "  SELECT symbol, timestamp, close "
            "  FROM StockHistory "
            "  WHERE symbol IN (SELECT stockID FROM symbols)"
            "), "
            "pivoted AS ("
            "  SELECT timestamp, jsonb_object_agg(symbol, close) AS row "
            "  FROM prices GROUP BY timestamp"
            "), "
            "unpacked AS ("
            "  SELECT (row ->> key)::text AS symbol, "
            "         (row ->> key)::numeric AS value, timestamp "
            "  FROM pivoted, jsonb_each(row)"
            "), "
            "matrix AS ("
            "  SELECT a.symbol AS sym1, b.symbol AS sym2, "
                + std::string(matrixType == "correlation" ? "corr" : "covar_pop") +
            "(a.value, b.value) AS val "
            "  FROM unpacked a JOIN unpacked b ON a.timestamp = b.timestamp "
            "  GROUP BY sym1, sym2"
            ") "
            "SELECT jsonb_object_agg(sym1, pairs) AS matrix FROM ("
            "  SELECT sym1, jsonb_object_agg(sym2, val) AS pairs FROM matrix GROUP BY sym1"
            ") AS result;";


        pqxx::result res = W.exec(sql);
        if (res.empty()) {
            std::cout << "Matrix calculation returned nothing.\n";
            return;
        }

        json matrix = json::parse(res[0]["matrix"].c_str());

        // Insert into cache
        std::string insertQuery =
            "INSERT INTO CachedMatrix (stockListName, ownerUsername, matrixType, matrixData) VALUES (" +
            W.quote(list) + ", " + W.quote(owner) + ", " + W.quote(matrixType) + ", " + W.quote(matrix.dump()) + ") "
            "ON CONFLICT (stockListName, ownerUsername, matrixType) DO UPDATE SET "
            "matrixData = EXCLUDED.matrixData, lastUpdated = CURRENT_TIMESTAMP;";
        W.exec(insertQuery);
        W.commit();

        std::cout << matrixType << " matrix:\n";
        drawMatrix(matrix);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void findStockListCovarianceMatrix() {
    findMatrix("covariance");
}

void findStockListCorrelationMatrix() {
    findMatrix("correlation");
}
