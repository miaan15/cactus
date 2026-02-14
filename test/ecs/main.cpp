import ECS;

int main() {
    cactus::ecs::SmallWorld<int, double> w;
    auto e = w.create_entity();
    w.emplace<int>(e, 5);
    w.erase<int>(e);
}
