#include "hanoi/hanoi.hpp"
#include "visualization.hpp"
#include <array>
#include <iostream>
#include <queue>
#include <random>
#include <sstream>

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error("check failed: " #condition); } while (false)
namespace {
constexpr std::array algorithms{hanoi::Algorithm::recursive, hanoi::Algorithm::iterative,
                                hanoi::Algorithm::bitwise, hanoi::Algorithm::formula};
template<class F> void rejects(F&& f) {
    bool threw = false;
    try { f(); } catch (const std::exception&) { threw = true; }
    CHECK(threw);
}
struct State {
    std::array<std::vector<unsigned>, 3> pegs;
    explicit State(unsigned n) { for (unsigned d = n; d; --d) pegs[0].push_back(d); }
    void apply(hanoi::Move m) {
        CHECK(m.from < 3 && m.to < 3 && m.from != m.to);
        CHECK(!pegs[m.from].empty() && pegs[m.from].back() == m.disk);
        CHECK(pegs[m.to].empty() || pegs[m.to].back() > m.disk);
        pegs[m.from].pop_back();
        pegs[m.to].push_back(m.disk);
    }
};
// Independent oracle: descend the recursive solution tree without enumerating its prefix.
hanoi::Move recursive_at(unsigned n, hanoi::Count m, unsigned from = 0, unsigned to = 2) {
    const auto middle = hanoi::Count{1} << (n - 1);
    if (m == middle) return {n, from, to};
    if (m < middle) return recursive_at(n - 1, m, from, 3 - from - to);
    return recursive_at(n - 1, m - middle, 3 - from - to, to);
}
unsigned bfs_distance(unsigned n) {
    unsigned states = 1;
    for (unsigned d = 0; d < n; ++d) states *= 3;
    std::vector<int> distance(states, -1);
    std::queue<unsigned> queue;
    distance[0] = 0; queue.push(0);
    while (!queue.empty()) {
        const auto state = queue.front(); queue.pop();
        if (state == states - 1) return static_cast<unsigned>(distance[state]);
        std::array<unsigned, 3> top{n, n, n};
        unsigned encoded = state;
        for (unsigned d = 0; d < n; ++d) {
            const auto peg = encoded % 3; encoded /= 3;
            if (top[peg] == n) top[peg] = d;
        }
        for (unsigned from = 0; from < 3; ++from) {
            for (unsigned to = 0; to < 3; ++to) {
                if (top[from] >= top[to]) continue;
                unsigned power = 1;
                for (unsigned d = 0; d < top[from]; ++d) power *= 3;
                const auto next = state - from * power + to * power;
                if (distance[next] != -1) continue;
                distance[next] = distance[state] + 1; queue.push(next);
            }
        }
    }
    throw std::runtime_error("BFS failed");
}
}
int main() {
    try {
        for (unsigned n = 0; n <= 12; ++n) {
            std::vector<hanoi::Move> reference;
            for (auto algorithm : algorithms) {
                State state(n);
                std::vector<hanoi::Move> moves;
                const auto count = hanoi::generate(n, UINT64_MAX, algorithm, [&](hanoi::Move m) {
                    state.apply(m); moves.push_back(m);
                });
                CHECK(count == hanoi::move_count(n) && count == moves.size());
                CHECK(state.pegs[0].empty() && state.pegs[1].empty() && state.pegs[2].size() == n);
                if (algorithm == hanoi::Algorithm::recursive) reference = moves;
                CHECK(moves == reference);
                for (hanoi::Count limit : {hanoi::Count{0}, hanoi::Count{1}, count / 2, count, count + 1}) {
                    hanoi::Count index = 0;
                    const auto generated = hanoi::generate(n, limit, algorithm, [&](hanoi::Move m) {
                        CHECK(m == reference.at(static_cast<std::size_t>(index++)));
                    });
                    CHECK(index == std::min(limit, count) && index == generated);
                }
            }
            if (n <= 6) CHECK(bfs_distance(n) == hanoi::move_count(n));
        }
        std::mt19937_64 random(42);
        for (unsigned n = 1; n <= 64; ++n) {
            const auto total = hanoi::move_count(n);
            for (unsigned i = 0; i < 1000; ++i) {
                const auto m = i == 0 ? total : 1 + random() % total;
                CHECK(hanoi::move_at(n, m) == recursive_at(n, m));
                const unsigned peg[] = {0, n & 1U ? 1U : 2U, n & 1U ? 2U : 1U};
                CHECK(hanoi::detail::bitwise_move(m, peg) == recursive_at(n, m));
            }
            for (auto algorithm : algorithms) {
                hanoi::Count m = 0;
                State state(n);
                hanoi::generate(n, 127, algorithm, [&](hanoi::Move move) {
                    CHECK(move == recursive_at(n, ++m)); state.apply(move);
                });
            }
        }
        CHECK(hanoi::move_count(0) == 0);
        CHECK(hanoi::move_count(63) == UINT64_MAX / 2);
        CHECK(hanoi::move_count(64) == UINT64_MAX);
        CHECK((hanoi::move_at(64, hanoi::Count{1} << 63) == hanoi::Move{64, 0, 2}));
        rejects([] { hanoi::move_count(65); });
        rejects([] { hanoi::move_at(0, 1); });
        rejects([] { hanoi::move_at(1, 0); });
        rejects([] { hanoi::move_at(1, 2); });
        rejects([] { hanoi::generate(65, 0, hanoi::Algorithm::formula, [](auto) {}); });
        rejects([] { hanoi::generate(1, 1, static_cast<hanoi::Algorithm>(100), [](auto) {}); });
        rejects([] { hanoi::parse_algorithm("bad"); });
        rejects([] { hanoi::generate(3, 7, hanoi::Algorithm::recursive, [](auto) { throw std::runtime_error("sink"); }); });
        Visualization visual(2);
        rejects([&] { visual.apply({2, 0, 2}); });
        rejects([&] { visual.apply({1, 0, 3}); });
        hanoi::generate(2, 3, hanoi::Algorithm::formula, [&](auto move) { visual.apply(move); });
        std::ostringstream ascii, ansi;
        visual.draw(ascii, 3, false); visual.draw(ansi, 3, true);
        CHECK(ascii.str().find("=====") != std::string::npos);
        CHECK(ascii.str().find('\033') == std::string::npos);
        CHECK(ansi.str() == "\033[2J\033[H" + ascii.str());
        std::cout << "core tests passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
