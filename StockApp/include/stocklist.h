#ifndef STOCKLIST_H
#define STOCKLIST_H

void createStockList(const std::string& ownerUsername);

void addStocktoStockList(const std::string& ownerUsername);

void deleteStockfromStockList(const std::string& ownerUsername);

void changeStockListStatus(const std::string& ownerUsername);

void viewStockLists(const std::string& ownerUsername);

void viewOwnStockLists(const std::string& ownerUsername);

void shareStockList(const std::string& ownerUsername);

void addReviewStockList(const std::string& writerusername);

void editReviewStockList(const std::string& writerusername);

void deleteReviewStockList(const std::string& writerusername);

void viewReviewStockList(const std::string& writerusername);

void deleteStockList(const std::string& ownerUsername);

void viewStockListsStock(const std::string& viewerUsername); 

void viewStockListHistorical(const std::string& viewerUsername);

void viewStockListPastPerformance(const std::string& ownerUsername);

void viewStockListPrediction(const std::string& viewerUsername);

#endif