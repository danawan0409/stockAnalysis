#include "global.h"
#include <iostream>
#include <limits>

std::string currentUsername = "";
std::string connect_info = "dbname=c43final user=postgres password=123 hostaddr=127.0.0.1 port=5432";

template<typename T>
bool getValidatedInput(T& input) {
    std::cin >> input;
    if (std::cin.fail()) {
        std::cin.clear(); // clear the error state
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard invalid input
        std::cout << "Invalid input. Please enter a valid value.\n";
        return false;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // discard trailing newline
    return true;
}

template bool getValidatedInput<int>(int&);
template bool getValidatedInput<double>(double&);
template bool getValidatedInput<std::string>(std::string&);

void pauseConsole() {
    std::cin.clear();
    std::string dummy;
    std::cout << "\nPress Enter to continue...";
    std::getline(std::cin, dummy);

    // Clear the screen after user presses Enter
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void clearConsole() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}   
