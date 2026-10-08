#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace hanoi {
using Count = std::uint64_t;
struct Move {
    unsigned disk, from, to;
    bool operator==(const Move&) const = default;
};
enum class Algorithm { recursive, iterative, bitwise, formula };

inline Algorithm parse_algorithm(std::string_view name) {
    if (name == "recursive") return Algorithm::recursive;
    if (name == "iterative") return Algorithm::iterative;
    if (name == "bitwise") return Algorithm::bitwise;
    if (name == "formula") return Algorithm::formula;
    throw std::invalid_argument("unknown algorithm");
}
inline Count move_count(unsigned n) {
    if (n > 64) throw std::invalid_argument("disks must be in [0, 64]");
    return n == 64 ? std::numeric_limits<Count>::max() : (Count{1} << n) - 1;
}

namespace detail {
inline Move formula_move(unsigned n, Count m) {
    const unsigned k = std::countr_zero(m);
    const unsigned d = k + 1;
    // Two shifts keep the disk-64 case defined.
    const unsigned q = static_cast<unsigned>(((m >> 1) >> k) % 3);
    const bool forward = ((n + d) & 1U) != 0;
    const unsigned from = forward || q == 0 ? q : 3 - q;
    unsigned to = from + (forward ? 1U : 2U);
    if (to >= 3) to -= 3;
    return {d, from, to};
}
inline Move bitwise_move(Count m, const unsigned (&peg)[3]) {
    const Count previous = m - 1;
    const unsigned from = static_cast<unsigned>((m & previous) % 3);
    // Reduce before adding: (m | previous) + 1 could overflow at N=64.
    unsigned to = static_cast<unsigned>((m | previous) % 3) + 1;
    if (to == 3) to = 0;
    return {static_cast<unsigned>(std::countr_zero(m)) + 1, peg[from], peg[to]};
}
template<class Emit>
void recursive(unsigned n, unsigned from, unsigned to, Count& left, Emit& emit) {
    if (n == 0 || left == 0) return;
    const unsigned spare = 3 - from - to;
    recursive(n - 1, from, spare, left, emit);
    if (left == 0) return;
    emit(Move{n, from, to});
    --left;
    recursive(n - 1, spare, to, left, emit);
}
} // namespace detail

// Random access is useful for checking indices that cannot be reached in a test run.
inline Move move_at(unsigned n, Count m) {
    if (m == 0 || m > move_count(n)) throw std::invalid_argument("move index out of range");
    return detail::formula_move(n, m);
}

template<class Emit>
Count generate(unsigned n, Count limit, Algorithm algorithm, Emit&& emit) {
    const Count count = std::min(limit, move_count(n));
    switch (algorithm) {
    case Algorithm::recursive: {
        Count left = count;
        detail::recursive(n, 0, 2, left, emit);
        break;
    }
    case Algorithm::iterative: {
        std::vector<unsigned> position(n + 1, 0);
        for (Count i = 0; i < count; ++i) {
            const unsigned d = std::countr_zero(i + 1) + 1;
            const unsigned from = position[d];
            unsigned to = from + (((n + d) & 1U) ? 1U : 2U);
            if (to >= 3) to -= 3;
            position[d] = to;
            emit(Move{d, from, to});
        }
        break;
    }
    case Algorithm::bitwise: {
        const unsigned peg[] = {0, n & 1U ? 1U : 2U, n & 1U ? 2U : 1U};
        for (Count i = 0; i < count; ++i) emit(detail::bitwise_move(i + 1, peg));
        break;
    }
    case Algorithm::formula:
        for (Count i = 0; i < count; ++i) emit(detail::formula_move(n, i + 1));
        break;
    default:
        throw std::invalid_argument("invalid algorithm");
    }
    return count;
}

struct Checksum {
    Count value = 0;
    void operator()(Move move) {
        value = std::rotl(value, 7) ^ (Count{move.disk} * 16 + move.from * 4 + move.to);
    }
};
} // namespace hanoi
