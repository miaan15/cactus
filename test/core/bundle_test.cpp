import std;
import cactus;

using namespace cactus;

int main() {

    SpinesDocument a = SpinesDocument::make();
    a.parse("test/core/idk.bundle");
    a.destroy();
}
