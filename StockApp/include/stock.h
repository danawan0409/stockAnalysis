#ifndef STOCK_H
#define STOCK_H

#include <string>
#include <vector>

void drawASCII(const std::vector<std::pair<std::string, double>>& data);
std::vector<std::pair<std::string, double>> downsampleData(const std::vector<std::pair<std::string, double>>& data, size_t maxPoints);
std::vector<std::pair<std::string, double>> predictFuturePrices(
    const std::vector<std::pair<std::string, double>>& historical, int predictDays, int windowSize = 5);

void addStockRecord(); 
void addStock(); 
void viewAllStocks(); 

#endif
