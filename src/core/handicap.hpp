#ifndef SHOGI_HANDICAP_HPP
#define SHOGI_HANDICAP_HPP

/**
 * @file handicap.hpp
 * @brief Official-style komaochi (piece handicap) setup helpers.
 */

#include "board.hpp"
#include "types.hpp"

#include <string>

namespace shogi {

/** Handicap settings chosen before a new game. */
struct HandicapSettings {
    bool enabled;
    Color uwate;      ///< Stronger side (pieces removed; usually moves first)
    int rank_diff;    ///< 1..10+ (Tokyo Shogi Kaikan style ladder)

    HandicapSettings()
        : enabled(false), uwate(Color::Gote), rank_diff(2) {}
};

/**
 * Map rank difference to a short English label.
 * 1 = even (weaker moves first); 2+ = piece removals.
 */
char const* handicap_label(int rank_diff);

/**
 * Build starting board + who moves first under komaochi rules.
 * Removed pieces leave the game (not in hand).
 * @param settings Enabled flag, uwate color, rank difference (1..10+).
 * @param[out] first Side to move (uwate, except rank_diff &lt;= 1 → opposite of uwate).
 */
Board make_handicap_position(HandicapSettings const& settings, Color& first);

} // namespace shogi

#endif
