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

void findStatistic(const std::string& statName) {
    std::string choice;
    std::cout << "Do you want to find the " << statName << " of a stock in a (1) Portfolio or (2) StockList? ";
    std::cin >> choice;

    std::string symbol, name;
    bool valid = false;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        std::string owner, name, symbol;

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
            valid = false;

            std::cout << "Enter the owner of the stock list: ";
            std::cin >> owner;
            std::cout << "Enter the name of the stock list: ";
            std::cin.ignore();
            std::getline(std::cin, name);

            pqxx::connection C(connect_info);
            pqxx::work W(C);

            // Check if the stock list exists
            pqxx::result res = W.exec(
                "SELECT 1 FROM StockList WHERE name = " + W.quote(name) +
                " AND ownerUsername = " + W.quote(owner) + ";"
            );
            if (res.empty()) {
                std::cout << "Stock list not found.\n";
                return;
            }

            // Use the helper function to check access
            if (!hasAccessToStockList(W, currentUsername, owner, name)) {
                std::cout << "You do not have access to this stock list.\n";
                return;
            }

            std::cout << "Enter the stock symbol: ";
            std::cin >> symbol;

            // Check if the stock is in the stock list
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

                std::cout << "SELECT " + statName + " FROM CachedStockStatistics WHERE symbol = " + W.quote(symbol) + ";"; 

                if (result.empty()) {
                    std::cout << "Statistic not cached yet for stock" << symbol << ".\n";
                } else {
                    std::cout << "Interval: All (Default)\n";
                    std::cout << statName << " for " << symbol << " is: " << result[0][0].as<std::string>() << "\n";
                }

            } else if (intervalChoice == "2") {
                std::string start, end;
                std::regex dateRegex(R"(\d{4}-\d{2}-\d{2})");
                
                std::cout << "Enter start date (YYYY-MM-DD): ";
                std::cin >> start;
                if (!std::regex_match(start, dateRegex)) {
                    std::cout << "Invalid start date format.\n";
                    return;
                }
                
                std::cout << "Enter end date (YYYY-MM-DD): ";
                std::cin >> end;
                if (!std::regex_match(end, dateRegex)) {
                    std::cout << "Invalid end date format.\n";
                    return;
                }

                std::string query = R"(
                    WITH prices AS (
                        SELECT timestamp, symbol, close
                        FROM StockHistory
                        WHERE timestamp BETWEEN )" + W.quote(start) + R"( AND )" + W.quote(end) + R"(
                    ),
                    stock_returns AS (
                        SELECT
                            timestamp,
                            symbol,
                            (close - LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp)) /
                            NULLIF(LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp), 0) AS ret
                        FROM prices
                    )
                )" + (statName == "variation" ?
                    R"(SELECT VAR_POP(ret) AS result
                    FROM stock_returns
                    WHERE symbol = )" + W.quote(symbol) + ";" :
                    R"(, market_returns AS (
                        SELECT
                            timestamp,
                            AVG(ret) AS market_return
                        FROM stock_returns
                        GROUP BY timestamp
                    )
                    SELECT COVAR_POP(s.ret, m.market_return) AS result
                    FROM stock_returns s
                    JOIN market_returns m ON s.timestamp = m.timestamp
                    WHERE s.symbol = )" + W.quote(symbol) + ";");


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

            if (!hasAccessToStockList(W, currentUsername, ownerUsername, listOrPortfolioName)) {
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

        int useDefault;
        std::string startDate, endDate;
        std::cout << "Use default interval (all data)? (1 for yes, 2 for no): ";
        std::cin >> useDefault;

        if (useDefault != 1 && useDefault != 2) {
            std::cout << "Invalid option. Please enter 1 or 2.\n";
            return;
        }

        if (useDefault == 1) {
            // Load from CachedMatrix
            std::map<std::string, std::map<std::string, double>> matrix;
        
            for (const auto& s1 : symbols) {
                for (const auto& s2 : symbols) {
                    std::string first = std::min(s1, s2);
                    std::string second = std::max(s1, s2);
        
                    std::string q = "SELECT " + matrixType + " FROM CachedMatrix WHERE symbol1 = " +
                                    W.quote(first) + " AND symbol2 = " + W.quote(second);
                    pqxx::result r = W.exec(q);
        
                    double val = r.empty() ? 0.0 : r[0][matrixType].as<double>();
                    matrix[s1][s2] = val;
                }
            }
        
            printMatrix(symbols, matrix);
            return;
        }        

        if (useDefault == 2) {
            std::regex dateRegex(R"(\d{4}-\d{2}-\d{2})");
        
            std::cout << "Enter start date (YYYY-MM-DD): ";
            std::cin >> startDate;
            if (!std::regex_match(startDate, dateRegex)) {
                std::cout << "Invalid start date format.\n";
                return;
            }
        
            std::cout << "Enter end date (YYYY-MM-DD): ";
            std::cin >> endDate;
            if (!std::regex_match(endDate, dateRegex)) {
                std::cout << "Invalid end date format.\n";
                return;
            }
        }
        
        // Compute matrix directly from returns
        std::map<std::string, std::map<std::string, double>> matrix;
        for (const auto& s1 : symbols) {
            for (const auto& s2 : symbols) {
                std::string query =
                    "WITH returns AS ("
                    "  SELECT symbol, timestamp, "
                    "         (close - LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp)) / "
                    "         NULLIF(LAG(close) OVER (PARTITION BY symbol ORDER BY timestamp), 0) AS daily_return "
                    "  FROM StockHistory "
                    "  WHERE (symbol = " + W.quote(s1) + " OR symbol = " + W.quote(s2) + ")";
                if (useDefault == 2)
                    query += " AND timestamp BETWEEN " + W.quote(startDate) + " AND " + W.quote(endDate);
                query += ") "
                        "SELECT a.daily_return AS r1, b.daily_return AS r2 FROM returns a "
                        "JOIN returns b ON a.timestamp = b.timestamp "
                        "WHERE a.symbol = " + W.quote(s1) + " AND b.symbol = " + W.quote(s2);

                pqxx::result returns = W.exec(query);
                std::vector<std::pair<double, double>> pairs;
                for (const auto& row : returns) {
                    if (!row["r1"].is_null() && !row["r2"].is_null()) {
                        pairs.emplace_back(row["r1"].as<double>(), row["r2"].as<double>());
                    }
                }

                // Now compute the correlation or covariance manually in C++
                double sum_x = 0, sum_y = 0, sum_x2 = 0, sum_y2 = 0, sum_xy = 0;
                int n = 0;
                for (const auto& [x, y] : pairs) {
                    sum_x += x;
                    sum_y += y;
                    sum_x2 += x * x;
                    sum_y2 += y * y;
                    sum_xy += x * y;
                    ++n;
                }

                double cov = (n > 1) ? (sum_xy - sum_x * sum_y / n) / (n - 1) : 0.0;
                double corr = (n > 1)
                    ? (cov / (std::sqrt((sum_x2 - sum_x * sum_x / n) / (n - 1)) *
                            std::sqrt((sum_y2 - sum_y * sum_y / n) / (n - 1))))
                    : 0.0;

                matrix[s1][s2] = matrixType == "covariance" ? cov : corr;

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
