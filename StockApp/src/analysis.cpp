#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <map>
#include <iomanip>
#include <regex>
#include <sstream>
#include "json.hpp"
#include "global.h"

using json = nlohmann::json;

void findStatistic(const std::string& statName) {
    std::string choice;
    std::cout << "Do you want to find the " << statName << " of a stock in a (1) Portfolio or (2) StockList? ";
    std::cin >> choice;

    std::string symbol, name;
    bool valid = false;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        if (choice == "1") {
            std::cout << "Enter the name of the portfolio: ";
            std::cin.ignore();
            std::getline(std::cin, name);

            pqxx::result res = W.exec(
                "SELECT 1 FROM Portfolio WHERE name = " + W.quote(name) +
                " AND ownerUsername = " + W.quote(currentUsername) + ";"
            );
            if (res.empty()) {
                std::cout << "Portfolio not found.\n";
                return;
            }

            std::cout << "Enter the stock symbol: ";
            std::cin >> symbol;

            pqxx::result inPortfolio = W.exec(
                "SELECT 1 FROM PortfolioHasStock WHERE portfolioName = " + W.quote(name) +
                " AND ownerUsername = " + W.quote(currentUsername) +
                " AND stockID = " + W.quote(symbol) + ";"
            );
            if (inPortfolio.empty()) {
                std::cout << "Stock is not in the portfolio.\n";
                return;
            }

            valid = true;
        } else if (choice == "2") {
            std::string owner;
            std::cout << "Enter the owner of the stock list: ";
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
                " AND (visibility = 'public' OR ownerUsername = " + W.quote(currentUsername) +
                " OR EXISTS (SELECT 1 FROM ShareStockList WHERE ownerUsername = " + W.quote(owner) +
                " AND receiverUsername = " + W.quote(currentUsername) +
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
            std::cout << "Use default interval? (1 for yes / 2 for no): ";
            std::string intervalChoice;
            std::cin >> intervalChoice;

            if (intervalChoice == "1") {
                pqxx::result result = W.exec(
                    "SELECT " + statName + " FROM CachedStockStatistics WHERE symbol = " + W.quote(symbol) + ";"
                );

                if (result.empty()) {
                    std::cout << "Statistic not cached yet for stock " << symbol << ".\n";
                } else {
                    std::cout << "Interval: All (Default)\n";
                    std::cout << statName << " for " << symbol << " is: " << result[0][0].as<std::string>() << "\n";
                }

            } else if (intervalChoice == "2") {
                std::string start, end;
                std::cout << "Enter start date (YYYY-MM-DD): ";
                std::cin >> start;
                std::cout << "Enter end date (YYYY-MM-DD): ";
                std::cin >> end;

                std::string query = R"(
                    WITH prices AS (
                        SELECT timestamp, close
                        FROM StockHistory
                        WHERE symbol = )" + W.quote(symbol) + R"( AND timestamp BETWEEN )" + W.quote(start) + R"( AND )" + W.quote(end) + R"(
                    ),
                    returns AS (
                        SELECT (close - LAG(close) OVER (ORDER BY timestamp)) / NULLIF(LAG(close) OVER (ORDER BY timestamp), 0) AS ret
                        FROM prices
                    )
                    SELECT
                        )" + (statName == "variation"
                            ? "VAR_POP(ret)"
                            : "COVAR_POP(ret, mr.market_return)") + R"( AS result
                    FROM returns r )" +
                    (statName == "beta"
                        ? R"(
                        JOIN (
                            SELECT timestamp, AVG((close - LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp)) / NULLIF(LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp), 0)) AS market_return
                            FROM StockHistory
                            WHERE timestamp BETWEEN )" + W.quote(start) + " AND " + W.quote(end) + R"(
                            GROUP BY timestamp
                        ) mr ON r.timestamp = mr.timestamp
                    )"
                        : "") + ";";

                pqxx::result calc = W.exec(query);

                if (calc.empty() || calc[0][0].is_null()) {
                    std::cout << "Unable to compute " << statName << " in the given interval.\n";
                } else {
                    double value = calc[0][0].as<double>();
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(4) << value;
                    std::cout << "Interval: " << start << " to " << end << "\n";
                    std::cout << "Calculated " << statName << " for " << symbol << ": " << ss.str() << "\n";
                }

            } else {
                std::cout << "Invalid option.\n";
                return;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Database error: " << e.what() << "\n";
    }
}

void findVariation() {
    findStatistic("variation");
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

void printMatrix(const std::vector<std::string>& symbols, const std::map<std::string, std::map<std::string, double>>& matrix) {
    std::cout << std::fixed << std::setprecision(4);
    std::cout << std::setw(10) << " ";
    for (const auto& col : symbols) {
        std::cout << std::setw(10) << col;
    }
    std::cout << "\n";

    for (const auto& row : symbols) {
        std::cout << std::setw(10) << row;
        for (const auto& col : symbols) {
            std::cout << std::setw(10) << matrix.at(row).at(col);
        }
        std::cout << "\n";
    }
}

void findMatrix(const std::string& matrixType) {
    std::string ownerUsername, listOrPortfolioName;
    int type;

    std::cout << "Do you want to find the " << matrixType << " matrix for a portfolio (1) or stock list (2)? ";
    std::cin >> type;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::vector<std::string> symbols;

        if (type == 1) {
            std::cout << "Enter the portfolio name: ";
            std::cin.ignore();
            std::getline(std::cin, listOrPortfolioName);

            auto r = W.exec("SELECT 1 FROM Portfolio WHERE name = " + W.quote(listOrPortfolioName) +
                            " AND ownerUsername = " + W.quote(currentUsername));
            if (r.empty()) {
                std::cout << "Portfolio not found.\n";
                return;
            }

            auto rows = W.exec("SELECT stockID FROM PortfolioHasStock WHERE portfolioName = " +
                               W.quote(listOrPortfolioName) + " AND ownerUsername = " + W.quote(currentUsername));
            for (const auto& row : rows)
                symbols.push_back(row["stockID"].c_str());

        } else if (type == 2) {
            std::cout << "Enter owner username: ";
            std::cin >> ownerUsername;
            std::cout << "Enter the stock list name: ";
            std::cin.ignore();
            std::getline(std::cin, listOrPortfolioName);

            auto r = W.exec("SELECT visibility FROM StockList WHERE name = " + W.quote(listOrPortfolioName) +
                            " AND ownerUsername = " + W.quote(ownerUsername));
            if (r.empty()) {
                std::cout << "Stock list not found.\n";
                return;
            }

            auto visibility = r[0]["visibility"].c_str();
            if (visibility == std::string("private") && ownerUsername != ownerUsername) {
                std::cout << "You do not have access to this stock list.\n";
                return;
            }

            auto rows = W.exec("SELECT stockID FROM StockListHasStock WHERE stockListName = " +
                               W.quote(listOrPortfolioName) + " AND ownerUsername = " + W.quote(ownerUsername));
            for (const auto& row : rows)
                symbols.push_back(row["stockID"].c_str());
        } else {
            std::cout << "Invalid choice.\n";
            return;
        }

        // Load matrix values
        std::map<std::string, std::map<std::string, double>> matrix;
        for (const auto& s1 : symbols) {
            for (const auto& s2 : symbols) {
                pqxx::result result = W.exec(
                    "SELECT " + W.esc(matrixType) + " FROM CachedMatrix "
                    "WHERE (symbol1 = " + W.quote(s1) + " AND symbol2 = " + W.quote(s2) + ") "
                    "   OR (symbol1 = " + W.quote(s2) + " AND symbol2 = " + W.quote(s1) + ")");

                double value = result.empty() ? 0.0 : result[0][matrixType].as<double>();
                matrix[s1][s2] = value;
            }
        }

        printMatrix(symbols, matrix);

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
