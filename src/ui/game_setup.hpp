#ifndef SHOGI_GAME_SETUP_HPP
#define SHOGI_GAME_SETUP_HPP

/**
 * @file game_setup.hpp
 * @brief Pre-game clock / handicap choices collected by the menu dialog.
 */

#include "../core/handicap.hpp"
#include "../core/types.hpp"

namespace shogi {
namespace ui {

/**
 * Values confirmed in the New game / Engine setup popup.
 * @see PieceSetMenu setup dialog (row navigation; Enter confirms uwate).
 */
struct GameSetupConfig {
    bool use_clock;
    int main_minutes;      ///< 0..180
    int increment_seconds; ///< 0..180 (not both zero with main)
    bool use_handicap;
    Color stronger;        ///< uwate
    int rank_diff;         ///< 1..10

    GameSetupConfig()
        : use_clock(false),
          main_minutes(10),
          increment_seconds(10),
          use_handicap(false),
          stronger(Color::Gote),
          rank_diff(2) {}

    bool clock_valid() const {
        if (!use_clock) {
            return true;
        }
        return !(main_minutes == 0 && increment_seconds == 0);
    }

    HandicapSettings to_handicap() const {
        HandicapSettings h;
        h.enabled = use_handicap;
        h.uwate = stronger;
        h.rank_diff = rank_diff < 1 ? 1 : rank_diff;
        return h;
    }
};

} // namespace ui
} // namespace shogi

#endif
