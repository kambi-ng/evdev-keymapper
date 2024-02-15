#ifndef PRINT_H_
#define PRINT_H_
#include <iostream>

template<typename T, typename... Args>
void print(T first, Args... args) {
    std::cout << first << " ";
    if constexpr (sizeof...(args) > 0) {
        print(args...); // Recursive call to handle the rest of the arguments
    }

    if constexpr (sizeof...(args) == 0) {
        std::cout << std::endl;
    }
}

template<typename T, typename... Args>
void printerr(T first, Args... args) {
    std::cerr << first << " ";
    if constexpr (sizeof...(args) > 0) {
        printerr(args...); // Recursive call to handle the rest of the arguments
    }

    if constexpr (sizeof...(args) == 0) {
        std::cerr << std::endl;
    }
}

#endif // PRINT_H_
