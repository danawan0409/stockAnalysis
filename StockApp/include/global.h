#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>

#define CURRENT_DATE "'2018-02-07'"

extern std::string currentUsername;
extern std::string connect_info;

template<typename T>
bool getValidatedInput(T& input);
char getch(); 
void pauseConsole();
void clearConsole();

#endif
