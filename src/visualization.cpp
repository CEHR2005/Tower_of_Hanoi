#include "visualization.hpp"
#include <ostream>
#include <string>

Visualization::Visualization(unsigned disks) : disks_(disks) {
    hanoi::move_count(disks);
    for (unsigned d = disks; d > 0; --d) pegs_[0].push_back(d);
}
void Visualization::apply(hanoi::Move move) {
    if (move.from >= 3 || move.to >= 3 || move.from == move.to)
        throw std::logic_error("invalid peg");
    auto& source = pegs_[move.from];
    auto& target = pegs_[move.to];
    if (source.empty() || source.back() != move.disk ||
        (!target.empty() && target.back() < move.disk))
        throw std::logic_error("illegal move");
    target.push_back(move.disk);
    source.pop_back();
}
void Visualization::draw(std::ostream& out, hanoi::Count step, bool ansi) const {
    if (ansi) out << "\033[2J\033[H";
    out << "Step " << step << '\n';
    for (unsigned row = disks_; row > 0; --row) {
        for (const auto& peg : pegs_) {
            const unsigned d = row <= peg.size() ? peg[row - 1] : 0;
            out << std::string(disks_ - d, ' ')
                << (d == 0 ? "|" : std::string(2 * d + 1, '='))
                << std::string(disks_ - d + 2, ' ');
        }
        out << '\n';
    }
    for (char peg : {'A', 'B', 'C'})
        out << std::string(disks_, ' ') << peg << std::string(disks_ + 2, ' ');
    out << '\n';
}
