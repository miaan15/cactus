import std;
import cactus;

using namespace cactus;

int main() {
    SpinesDocument a = SpinesDocument::make();
    a.parse("test/core/idk.bundle");

    std::println("\n--- Testing Accessor Navigation ---");

    // 1. Deeply nested integer lookup
    // Path: a -> aa -> aaa -> [0]
    auto val_aaa = a.pick("a").pick("aa").pick("aaa").pick(0).as<int>();
    if (val_aaa) { std::println("1. Nested Int: a.aa.aaa = {}", val_aaa.value()); }

    // 2. Deeply nested string lookup
    // Path: a -> aa -> aac -> [0]
    auto val_aac = a.pick("a").pick("aa").pick("aac").pick(0).as<std::string_view>();
    if (val_aac) { std::println("2. Nested Str: a.aa.aac = \"{}\"", val_aac.value()); }

    // 3. Array elements lookup
    // Path: a -> ac -> [0, 1, 2]
    auto node_ac = a.pick("a").pick("ac");
    std::println("3. Array Vals: a.ac[0] = {}", node_ac.pick(0).as<int>().value_or(-1));
    std::println("               a.ac[1] = {}", node_ac.pick(1).as<int>().value_or(-1));
    std::println("               a.ac[2] = {}", node_ac.pick(2).as<int>().value_or(-1));

    // 4. Mixed block lookup (unnamed data point alongside a named child identifier)
    // Path: a -> ad -> [0] (the '4') AND a -> ad -> flag -> [0] (the '5')
    auto node_ad = a.pick("a").pick("ad");
    std::println("4. Mixed Blck: a.ad[0] (unnamed) = {}", node_ad.pick(0).as<int>().value_or(-1));

    auto val_flag = node_ad.pick("flag").pick(0).as<int>();
    if (val_flag) { std::println("               a.ad.flag = {}", val_flag.value()); }

    std::println("-----------------------------------\n");

    a.destroy();
    return 0;
}
