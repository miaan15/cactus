import std;
import cactus;

using namespace cactus;

int main() {

    Parser a = Parser::make();
    a.parse("test/core/idk.bundle");
    a.destroy();
}
