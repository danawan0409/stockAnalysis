#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cmath>

void drawStockHistoryASCII(const std::vector<std::pair<std::string, double>>& data) {
    if (data.empty()) return;

    double minPrice = data[0].second, maxPrice = data[0].second;
    for (const auto& [_, price] : data) {
        if (price < minPrice) minPrice = price;
        if (price > maxPrice) maxPrice = price;
    }

    int graphHeight = 10;

    std::cout << "\nHistorical Price Graph (Close Price):\n";
    for (int level = graphHeight; level >= 1; --level) {
        double threshold = minPrice + (maxPrice - minPrice) * (level - 1) / (graphHeight - 1);
        std::cout << std::setw(7) << std::fixed << std::setprecision(2) << threshold << " | ";
        for (const auto& [_, price] : data) {
            std::cout << (price >= threshold ? "*" : " ") << " ";
        }
        std::cout << "\n";
    }

    std::cout << "         +";
    for (size_t i = 0; i < data.size(); ++i) std::cout << "--";
    std::cout << "\n          ";

    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 5 == 0)
            std::cout << data[i].first.substr(5, 2) << " ";
        else
            std::cout << "  ";
    }
    std::cout << "\n";
}
