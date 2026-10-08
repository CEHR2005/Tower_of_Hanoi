#include "hanoi/hanoi.hpp"
#include "visualization.hpp"
#include <charconv>
#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace {
hanoi::Count number(std::string_view text) {
    hanoi::Count result{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (error != std::errc{} || end != text.data() + text.size())
        throw std::invalid_argument("expected an unsigned decimal integer: " + std::string(text));
    return result;
}
void help() {
    std::cout << "Usage: hanoi N [--count | --mode moves|quiet|ascii|ansi]\n"
                 "             [--algorithm formula|bitwise|iterative|recursive]\n"
                 "             [--limit M | --all] [--delay-ms M]\n"
                 "N: 0..64. Default: moves, limit 1000; quiet limit 1000000.\n"
                 "--limit 0 generates no moves. --all explicitly removes the limit.\n"
                 "--count prints only 2^N-1; generation options cannot be combined with it.\n"
                 "Delay: 0..60000 ms, only for ascii/ansi; ANSI requires a compatible terminal.\n";
}
}
int main(int argc, char** argv) {
    try {
        std::ios::sync_with_stdio(false);
        if (argc == 2 && std::string_view(argv[1]) == "--help") { help(); return 0; }
        if (argc < 2) throw std::invalid_argument("missing disk count; use --help");
        const auto raw_n = number(argv[1]);
        if (raw_n > 64) throw std::invalid_argument("disks must be in [0, 64]");
        const auto n = static_cast<unsigned>(raw_n);
        const auto total = hanoi::move_count(n);
        auto algorithm = hanoi::Algorithm::formula;
        std::string_view mode = "moves";
        std::optional<hanoi::Count> limit;
        hanoi::Count delay = 0;
        bool count_only = false, mode_set = false, algo_set = false, delay_set = false;
        for (int i = 2; i < argc; ++i) {
            const std::string_view option = argv[i];
            auto value = [&]() -> std::string_view {
                if (++i == argc) throw std::invalid_argument("missing value for " + std::string(option));
                return argv[i];
            };
            if (option == "--count" && !count_only) count_only = true;
            else if (option == "--mode" && !mode_set) { mode = value(); mode_set = true; }
            else if (option == "--algorithm" && !algo_set) {
                algorithm = hanoi::parse_algorithm(value()); algo_set = true;
            } else if (option == "--limit" && !limit) limit = number(value());
            else if (option == "--all" && !limit) limit = total;
            else if (option == "--delay-ms" && !delay_set) { delay = number(value()); delay_set = true; }
            else throw std::invalid_argument("unknown, duplicate or conflicting option: " + std::string(option));
        }
        if (count_only) {
            if (mode_set || algo_set || limit || delay_set)
                throw std::invalid_argument("--count cannot be combined with generation options");
            std::cout << total << '\n';
        } else {
            const bool visual = mode == "ascii" || mode == "ansi";
            if (!visual && mode != "moves" && mode != "quiet")
                throw std::invalid_argument("unknown mode");
            if (delay > 60000 || (delay_set && !visual))
                throw std::invalid_argument("--delay-ms requires ascii/ansi and must be <=60000");
            const auto maximum = limit.value_or(mode == "quiet" ? 1000000 : 1000);
            hanoi::Count generated = 0;
            if (mode == "quiet") {
                hanoi::Checksum checksum;
                generated = hanoi::generate(n, maximum, algorithm, checksum);
                std::cout << "checksum=" << checksum.value << '\n';
            } else if (mode == "moves") {
                generated = hanoi::generate(n, maximum, algorithm, [](hanoi::Move m) {
                    std::cout << m.disk << ' ' << char('A' + m.from) << " -> " << char('A' + m.to) << '\n';
                    if (!std::cout) throw std::runtime_error("output failed");
                });
            } else {
                Visualization view(n);
                view.draw(std::cout, 0, mode == "ansi");
                hanoi::Count step = 0;
                generated = hanoi::generate(n, maximum, algorithm, [&](hanoi::Move m) {
                    if (delay != 0) {
                        std::cout.flush();
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    }
                    view.apply(m);
                    view.draw(std::cout, ++step, mode == "ansi");
                    if (!std::cout) throw std::runtime_error("output failed");
                });
            }
            std::cout << "generated=" << generated << " total=" << total
                      << " complete=" << (generated == total ? "yes" : "no") << '\n';
        }
        std::cout.flush();
        if (!std::cout) throw std::runtime_error("output failed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
