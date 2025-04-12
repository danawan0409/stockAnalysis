#ifndef STOCK_H
#define STOCK_H

#include <string>
#include <vector>

void drawASCII(const std::vector<std::pair<std::string, double>>& data);
std::vector<std::pair<std::string, double>> downsampleData(const std::vector<std::pair<std::string, double>>& data, size_t maxPoints);

#endif
