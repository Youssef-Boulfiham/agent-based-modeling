#include "System.h"
#include <iostream>

int main() {
    try {
        System system;

        if (!system.initialize()) {
            return 1;
        }

        system.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
