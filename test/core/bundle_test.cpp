import std;
import cactus;

using namespace cactus;

int main() {
    SpinesDocument a = SpinesDocument::make();
    auto parse_err = a.parse("test/core/idk.bundle");
    if (parse_err.type != SpinesDocumentParsingError::Type::NONE) {
        std::println("CRITICAL: Failed to parse document! Error Code: {}", (int)parse_err.type);
        a.destroy();
        return -1;
    }

    std::println("\n--- Testing Accessor Navigation ---");

    auto log_error = [](auto p) {
        int err_code = (int)p.first;
        size_t depth = p.second;

        switch(err_code) {
            case 0: std::print("Error: INVALID_NAME (Couldn't find child identifier)"); break;
            case 1: std::print("Error: INDEX_OUT_OF_BOUND (pick(index) was too high)"); break;
            case 2: std::print("Error: INVALID_OPERATION (Tried to pick a name after picking an index)"); break;
            case 3: std::print("Error: NOT_A_VALUE (Called as<T> without calling pick(index) first)"); break;
            case 4: std::print("Error: WRONG_VALUE_TYPE (Data is not the type you requested)"); break;
            default: std::print("Error: UNKNOWN"); break;
        }
        std::println("; Failed at chain depth: {}", depth);
        
        return p; // FIX: Just return the raw error pair unchanged!
    };

    // 1. Deeply nested integer lookup
    // Path: a -> aa -> aaa -> [0]
    a.pick("a").pick("aa").pick("aaa").pick(0).as<int>()
        .transform([](int v) { std::println("1. Nested Int: a.aa.aaa = {}", v); return v; })
        .transform_error(log_error);

    // 2. Deeply nested string lookup
    // Path: a -> aa -> aac -> [0]
    a.pick("a").pick("aa").pick("aac").pick(0).as<std::string_view>()
        .transform([](std::string_view v) { std::println("2. Nested Str: a.aa.aac = \"{}\"", v); return v; })
        .transform_error(log_error);

    // 3. Array elements lookup
    // Path: a -> ac -> [0, 1, 2]
    auto node_ac = a.pick("a").pick("ac");
    node_ac.pick(0).as<int>()
        .transform([](int v) { std::println("3. Array Vals: a.ac[0] = {}", v); return v; })
        .transform_error(log_error);
        
    node_ac.pick(1).as<int>()
        .transform([](int v) { std::println("               a.ac[1] = {}", v); return v; })
        .transform_error(log_error);
        
    node_ac.pick(2).as<int>()
        .transform([](int v) { std::println("               a.ac[2] = {}", v); return v; })
        .transform_error(log_error);

    // 4. Mixed block lookup
    // Path: a -> ad -> [0] (the '4') AND a -> ad -> flag -> [0] (the '5')
    auto node_ad = a.pick("a").pick("ad");
    node_ad.pick(0).as<int>()
        .transform([](int v) { std::println("4. Mixed Blck: a.ad[0] (unnamed) = {}", v); return v; })
        .transform_error(log_error);

    node_ad.pick("flag").pick(0).as<int>()
        .transform([](int v) { std::println("               a.ad.flag = {}", v); return v; })
        .transform_error(log_error);

    std::println("-----------------------------------\n");

    a.destroy();
    return 0;
}
