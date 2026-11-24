#ifndef SHOGI_NOTATION_HPP
#define SHOGI_NOTATION_HPP

/**
 * @file notation.hpp
 * @brief Western (Hodges) move notation for kifu save/load.
 */

#include "board.hpp"

#include <string>
#include <vector>

namespace shogi {

/** Encode / parse Western algebraic moves used in @c .kifu files. */
class Notation {
public:
    /** Western move string, e.g. @c P-7f, @c Bx2b+, @c P*5e, @c Nx7c=. */
    static std::string to_western(Board const& before, Color side, Move const& move);

    /**
     * Parse a single Western move against @p before for @p side.
     * @param before Position before the move.
     * @param side   Side to move.
     * @param text   Western token (e.g. P-7f).
     * @param[out] out Filled on success.
     * @return False on failure.
     */
    static bool parse_western(Board const& before, Color side, std::string const& text, Move& out);

    /** Header line written at the top of a kifu file. */
    static std::string game_header();
};

} // namespace shogi

#endif
