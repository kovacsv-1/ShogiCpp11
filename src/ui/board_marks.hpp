#ifndef SHOGI_BOARD_MARKS_HPP
#define SHOGI_BOARD_MARKS_HPP

/**
 * @file board_marks.hpp
 * @brief User analysis marks: red squares and arrows on the board.
 */

#include "../core/types.hpp"

#include <vector>

namespace shogi {
namespace ui {

/** Directed arrow between two squares. */
struct BoardArrow {
    Square from;
    Square to;

    bool operator==(BoardArrow const& o) const {
        return from == o.from && to == o.to;
    }
};

/** Red square highlights and arrows (analysis marks). */
class BoardMarks {
public:
    void clear();
    bool empty() const;

    /** Toggle a red square mark. */
    void toggle_square(Square s);

    /** Toggle a red arrow between two squares. */
    void toggle_arrow(Square from, Square to);

    std::vector<Square> const& squares() const { return squares_; }
    std::vector<BoardArrow> const& arrows() const { return arrows_; }

private:
    std::vector<Square> squares_;
    std::vector<BoardArrow> arrows_;
};

} // namespace ui
} // namespace shogi

#endif
