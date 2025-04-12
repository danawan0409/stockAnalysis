#ifndef PORTFOLIO_H
#define PORTFOLIO_H

void createPortfolio(const std::string& ownerUsername);
void viewPortfolios(const std::string& ownerUsername);
void deletePortfolio(const std::string& ownerUsername);
void depositCash(const std::string& ownerUsername);
void withdrawCash(const std::string& ownerUsername);
void buyStock(const std::string& ownerUsername);
void sellStock(const std::string& ownerUsername);
void viewPortfolioHistorical(const std::string& ownerUsername);
void viewPortfolioPrediction(const std::string& ownerUsername)

#endif
