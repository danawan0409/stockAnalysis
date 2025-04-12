#ifndef STOCKLIST_H
#define STOCKLIST_H

std::string connect_info = "dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432";

void createStockList(const std::string& ownerUsername);

void addStocktoStockList(const std::string& ownerUsername);

void deleteStockfromStockList(const std::string& ownerUsername);

void changeStockListStatus(const std::string& ownerUsername);

void viewStockLists(const std::string& ownerUsername);

void viewOwnStockLists(const std::string& ownerUsername);

void shareStockList(const std::string& ownerUsername);

void addReviewStockList(const std::string& reviewerUsername);

void editReviewStockList(const std::string& reviewerUsername);

void deleteReviewStockList(const std::string& reviewerUsername);

void viewReviewStockList(const std::string& reviewerUsername);

void deleteStockList(const std::string& ownerUsername);

#endif