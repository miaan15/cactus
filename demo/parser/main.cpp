#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

import Parser;
import Common;

int main() {
    auto dir = cactus::root_dir / "demo/parser/test.spines";

    std::ifstream file(dir);
    if (!file.is_open()) return 1;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    cactus::Document d;
    d.parse(content.c_str());

    return 0;
}
