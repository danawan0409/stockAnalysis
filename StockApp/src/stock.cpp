#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cmath>
#include <algorithm>

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

    for (int i = 0; i < predictDays; ++i) {
        double sum = 0;
        for (int j = 0; j < windowSize; ++j) sum += recentPrices[recentPrices.size() - windowSize + j];
        double predicted = sum / windowSize;
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
