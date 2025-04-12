#include "global.h"
#include <iostream>
#include <limits>

std::string currentUsername = "";
std::string connect_info = "dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432";

void pauseConsole() {
    std::cout << "\nPress Enter to continue...\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}
