#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <random>
#include <pqxx/pqxx>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include "global.h"
#include <regex>

void drawASCII(const std::vector<std::pair<std::string, double>>& data) {
    if (data.empty()) return;

    double minPrice = data[0].second, maxPrice = data[0].second;
    for (const auto& [_, price] : data) {
        minPrice = std::min(minPrice, price);
        maxPrice = std::max(maxPrice, price);
    }

    int graphHeight = 10;
    double step = (maxPrice - minPrice) / graphHeight;

    std::vector<std::string> graph(graphHeight);

    for (size_t col = 0; col < data.size(); ++col) {
        double price = data[col].second;
        int height = static_cast<int>((price - minPrice) / step);
        for (int row = 0; row < graphHeight; ++row) {
            if ((graphHeight - row - 1) <= height)
                graph[row] += " █ ";
            else
                graph[row] += "   ";
        }
    }

    std::cout << "\nHistorical Price Graph (Close Price)\n";
    std::cout << "Price\n";

    // print graph with labels
    for (int i = 0; i < graphHeight; ++i) {
        double label = maxPrice - i * step;
        std::cout << std::setw(7) << std::fixed << std::setprecision(2) << label << " |" << graph[i] << "\n";
    }

    std::cout << "         ";
    for (size_t i = 0; i < data.size(); ++i) std::cout << "---";
    std::cout << "\n";

    std::cout << "Date: " << data.front().first << " to " << data.back().first << "\n";
}

std::vector<std::pair<std::string, double>> downsampleData(
    const std::vector<std::pair<std::string, double>>& data, size_t maxPoints) {

    if (data.size() <= maxPoints) return data;

    std::vector<std::pair<std::string, double>> result;
    size_t step = data.size() / maxPoints;

    for (size_t i = 0; i < data.size(); i += step) {
        result.push_back(data[i]);
    }

    return result;
}

std::vector<std::pair<std::string, double>> predictFuturePrices(
    const std::vector<std::pair<std::string, double>>& historical, int predictDays = 14, int windowSize = 5) {
    
    std::vector<std::pair<std::string, double>> predictions;
    if (historical.size() < static_cast<size_t>(windowSize)) return predictions;

    std::vector<double> recentPrices;
    for (size_t i = historical.size() - static_cast<size_t>(windowSize); i < historical.size(); ++i) {
        recentPrices.push_back(historical[i].second);
    }

    std::tm start{};
    std::istringstream ss("2018-02-08");
    ss >> std::get_time(&start, "%Y-%m-%d");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-0.005, 0.005);  // ±0.5%

    for (int i = 0; i < predictDays; ++i) {
        double sum = 0;
        for (int j = 0; j < windowSize; ++j) sum += recentPrices[recentPrices.size() - windowSize + j];
        double predicted = sum / windowSize;
        double noise = predicted * dist(gen);
        predicted += noise;
        recentPrices.push_back(predicted);

        char buf[11];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", &start);
        predictions.emplace_back(std::string(buf), predicted);

        start.tm_mday++;
        mktime(&start);
    }

    if (predictDays > 30) {
        return downsampleData(predictions, 30);
    } else {
        return predictions;
    }
}

void viewAllStocks() {
    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        const int pageSize = 10;
        int offset = 0;

        while (true) {
            pqxx::result rows = W.exec(
                "SELECT symbol, close FROM Stock ORDER BY symbol LIMIT " +
                W.quote(pageSize) + " OFFSET " + W.quote(offset));

            if (rows.empty()) {
                std::cout << "No more stocks to display.\n";
                break;
            }

            std::cout << "\n--- Stocks ---\n";
            for (const auto& row : rows) {
                std::string symbol = row["symbol"].c_str();
                if (row["close"].is_null()) {
                    std::cout << symbol << " | close: (N/A)\n";
                } else {
                    double close = row["close"].as<double>();
                    std::cout << symbol << " | close: " << close << "\n";
                }
            }

            std::cout << "\nPress n (next), p (previous), q (quit): ";
            char ch = getch();
            if (ch == 'n') {
                offset += pageSize;
            } else if (ch == 'p') {
                offset = std::max(0, offset - pageSize);
            } else if (ch == 'q') {
                break;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void addStock() {
    std::string symbol;
    std::string closeInput;

    std::cout << "Enter stock symbol: ";
    std::cin >> symbol;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        pqxx::result existing = W.exec(
            "SELECT 1 FROM Stock WHERE symbol = " + W.quote(symbol));
        if (!existing.empty()) {
            std::cout << "Stock already exists.\n";
            return;
        }

        std::cout << "Enter most recent close value (or leave empty to skip): ";
        std::cin.ignore();
        std::getline(std::cin, closeInput);

        std::string query;
        if (!closeInput.empty()) {
            double closeVal;
            try {
                closeVal = std::stod(closeInput);
            } catch (...) {
                std::cout << "Invalid number format.\n";
                return;
            }

            query = "INSERT INTO Stock (symbol, close) VALUES (" +
                    W.quote(symbol) + ", " + W.quote(closeVal) + ")";
        } else {
            query = "INSERT INTO Stock (symbol) VALUES (" + W.quote(symbol) + ")";
        }

        W.exec(query);
        W.commit();
        std::cout << "Stock added successfully.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}


void addStockRecord() {
    std::string symbol;
    std::cout << "Enter stock symbol: ";
    std::cin >> symbol;

    try {
        pqxx::connection C(connect_info);
        pqxx::work W(C);

        pqxx::result res = W.exec(
            "SELECT 1 FROM Stock WHERE symbol = " + W.quote(symbol));
        if (res.empty()) {
            std::cout << "Stock does not exist.\n";
            return;
        }

        std::string date;
        std::regex dateRegex(R"(\d{4}-\d{2}-\d{2})");

        std::cout << "Enter date (YYYY-MM-DD): ";
        std::cin >> date;
        if (!std::regex_match(date, dateRegex)) {
            std::cout << "Invalid date format.\n";
            return;
        }

        double open, close, high, low;
        long long volume;

        std::cout << "Enter open: ";
        std::cin >> open;
        std::cout << "Enter close: ";
        std::cin >> close;
        std::cout << "Enter high: ";
        std::cin >> high;
        std::cout << "Enter low: ";
        std::cin >> low;
        std::cout << "Enter volume: ";
        std::cin >> volume;

        W.exec(
            "INSERT INTO StockHistory (symbol, timestamp, open, close, high, low, volume) VALUES (" +
            W.quote(symbol) + ", " + W.quote(date) + ", " +
            W.quote(open) + ", " + W.quote(close) + ", " +
            W.quote(high) + ", " + W.quote(low) + ", " +
            W.quote(volume) + ")"
        );
        W.commit();
        std::cout << "Stock record added.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
