#include "hanoi/hanoi.hpp"
#include <charconv>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::invalid_argument("usage: hanoi_bench algorithm N");
        const auto algorithm = hanoi::parse_algorithm(argv[1]);
        unsigned n{};
        const std::string_view arg = argv[2];
        const auto [end, error] = std::from_chars(arg.data(), arg.data() + arg.size(), n);
        if (error != std::errc{} || end != arg.data() + arg.size() || n > 30)
            throw std::invalid_argument("benchmark N must be in [0,30]");
        const auto count = hanoi::move_count(n);
        hanoi::Checksum checksum;
        const auto start = std::chrono::steady_clock::now();
        hanoi::generate(n, count, algorithm, [&](hanoi::Move m) {
#if defined(__GNUC__) || defined(__clang__)
            // Make every generated field observable without a per-move external call.
            asm volatile("" : "+r"(m.disk), "+r"(m.from), "+r"(m.to));
#else
#error "Benchmark compiler barrier requires GCC or Clang"
#endif
            checksum(m);
        });
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        // VmHWM belongs to this exec image; ru_maxrss can retain the Python parent's peak.
        std::ifstream status("/proc/self/status");
        std::string line;
        long peak_rss_kib = 0;
        while (std::getline(status, line)) {
            if (line.starts_with("VmHWM:")) {
                std::istringstream value(line.substr(6));
                value >> peak_rss_kib;
                break;
            }
        }
        if (peak_rss_kib <= 0) throw std::runtime_error("cannot read VmHWM from /proc/self/status");
        std::cout << argv[1] << ',' << n << ',' << count << ',' << std::setprecision(12)
                  << seconds << ',' << (seconds == 0 ? 0 : static_cast<double>(count) / seconds)
                  << ',' << peak_rss_kib << ',' << checksum.value << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}
