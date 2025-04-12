#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <map>
#include <iomanip>
#include "json.hpp"
#include "global.h"

using json = nlohmann::json;

void findVariation(){
    return; 
}

void findBeta(){
    return; 
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
