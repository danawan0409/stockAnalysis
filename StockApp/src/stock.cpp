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
    std::cout << "         ";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 2 == 0)
            std::cout << data[i].first << " ";
        else
            std::cout << "           ";
    }    
    std::cout << "\n";
}
