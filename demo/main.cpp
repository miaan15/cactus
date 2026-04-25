import std;
import cactus.core.strat.slotmap;

int main() {
    auto sm = cactus::SlotMap<double>::make();

    cactus::SlotMapKey _;
    _ = sm.push(10);
    _ = sm.push(20);
    _ = sm.push(30);

    std::cout << sm.remove(_) << "\n";
    std::cout << sm.remove(_) << "\n";

    _ = sm.push(40);
    _ = sm.push(50);

    std::cout << sm.get(_).value_or(-1) << "\n";

    for (auto x : sm) { std::cout << x << " "; }
    std::cout << "\n";

    auto r = sm | std::views::filter([](int x) { return x / 10 % 2 == 0; }) |
             std::views::transform([](int n) { return n / 10 + 1; });
    for (auto x : r) { std::cout << x << " "; }
    std::cout << "\n";
}
