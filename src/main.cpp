#include <iostream>
#include "app/app.hpp"
#include "logger.hpp"

int main() {
    std::cout << "F95 Manager C++ port scaffold" << std::endl;
    logger::info("Startup");
    app::App application;
    return application.run();
}
