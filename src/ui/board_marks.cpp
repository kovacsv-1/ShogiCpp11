#include "board_marks.hpp"

#include <algorithm>

namespace shogi {
namespace ui {

void BoardMarks::clear() {
    squares_.clear();
    arrows_.clear();
}

bool BoardMarks::empty() const {
    return squares_.empty() && arrows_.empty();
}

void BoardMarks::toggle_square(Square s) {
    auto it = std::find(squares_.begin(), squares_.end(), s);
    if (it != squares_.end()) {
        squares_.erase(it);
    } else {
        squares_.push_back(s);
    }
}

void BoardMarks::toggle_arrow(Square from, Square to) {
    if (from == to) {
        toggle_square(from);
        return;
    }
    BoardArrow a{from, to};
    auto it = std::find(arrows_.begin(), arrows_.end(), a);
    if (it != arrows_.end()) {
        arrows_.erase(it);
    } else {
        arrows_.push_back(a);
    }
}

} // namespace ui
} // namespace shogi
