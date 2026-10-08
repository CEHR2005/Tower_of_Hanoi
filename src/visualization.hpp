#pragma once
#include "hanoi/hanoi.hpp"
#include <array>
#include <iosfwd>
#include <vector>

class Visualization {
public:
    explicit Visualization(unsigned disks);
    void apply(hanoi::Move move);
    void draw(std::ostream& out, hanoi::Count step, bool ansi) const;
private:
    unsigned disks_;
    std::array<std::vector<unsigned>, 3> pegs_;
};
