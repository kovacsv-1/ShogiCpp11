#ifndef SHOGI_GAME_HPP
#define SHOGI_GAME_HPP

/**
 * @file game.hpp
 * @brief Full game session: history, undo/redo, end conditions, kifu I/O.
 */

#include "board.hpp"
#include "handicap.hpp"
#include "rules.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace shogi {

/** One recorded ply (Western notation + metadata for undo / perpetual-check). */
struct HistoryEntry {
    Move move;
    std::string notation;
    Piece moved_before;
    bool gave_check;
};

/**
 * Owns the live @ref Board, side to move, and move history.
 * Hard mode swaps the pluggable @ref MoveGenerator to pseudo-legal moves.
 */
class Game {
public:
    Game();

    Board const& board() const { return board_; }
    Color side_to_move() const { return side_; }
    Color start_side() const { return start_side_; }
    GameStatus status() const { return status_; }
    Color winner() const { return winner_; }
    std::vector<HistoryEntry> const& history() const { return history_; }
    std::string const& message() const { return message_; }

    /**
     * Pluggable legal-move filter.
     * Hard mode installs pseudo-legal generation; normal mode uses @ref Rules::generate_legal.
     */
    using MoveGenerator = std::function<std::vector<Move>(Board const&, Color)>;

    /** Enable/disable hard mode and rebind the default @ref MoveGenerator. */
    void set_hard_mode(bool on);
    bool hard_mode() const { return hard_mode_; }
    /** Override move generation; empty @p gen restores the default for current hard mode. */
    void set_move_generator(MoveGenerator gen);

    /**
     * Viewing the tip of history (not an earlier ply).
     * New moves only here; playing clears the redo stack.
     */
    bool at_current_ply() const { return view_ply_ == history_.size(); }
    std::size_t view_ply() const { return view_ply_; }

    bool can_undo() const { return view_ply_ > 0; }
    bool can_redo() const {
        return view_ply_ < history_.size() || !redo_.empty();
    }

    /** If true, a move from a reviewed ply truncates later moves (new fork). */
    void set_allow_review_fork(bool on) { allow_review_fork_ = on; }
    bool allow_review_fork() const { return allow_review_fork_; }

    /** Whether the UI may accept a board/hand move right now. */
    bool can_accept_moves() const;

    /** Moves the UI may offer (via current @ref MoveGenerator). */
    std::vector<Move> legal_moves() const;

    /**
     * Play @p move if offered by @ref legal_moves.
     * In hard mode an illegal attempt ends the game (returns true with IllegalMove status).
     */
    bool try_play(Move move);
    /** Hard mode: declare an illegal attempt — side to move loses. */
    void lose_by_illegal();
    bool undo();
    bool redo();
    void resign();
    /**
     * True when both kings have entered and the game is still live at tip
     * (manual claim via UI — not automatic).
     */
    bool can_claim_jishogi() const;
    /** Apply jishogi point count if @ref can_claim_jishogi; otherwise false. */
    bool claim_jishogi();
    void reset();
    /** New game with a chosen first player (furigoma). */
    void reset_starting(Color first);

    /**
     * New game from komaochi @p hc (clears clock unless reconfigured after).
     * Undo/redo rebuild from this start (private show_ply replay).
     */
    void reset_handicap(HandicapSettings const& hc);

    /**
     * Load an arbitrary position (tests / tools). Clears history.
     * @param board Taken by value and moved into place.
     * @param first Both start_side and current side_to_move.
     */
    void set_position(Board board, Color first);

    /**
     * Furigoma: majority promoted (tokin face) → Gote starts, else Sente
     * (thrower treated as Sente).
     */
    static Color furigoma_starter(int promoted_count, int total = 5);

    /** Chess clock (main + increment). Disabled by default. */
    void configure_clock(bool enabled, int main_minutes, int increment_seconds);
    void clear_clock();
    bool clock_enabled() const { return clock_enabled_; }
    int clock_main_ms() const { return clock_main_ms_; }
    int clock_inc_ms() const { return clock_inc_ms_; }
    int clock_remaining_ms(Color c) const {
        return c == Color::Sente ? clock_sente_ms_ : clock_gote_ms_;
    }
    /**
     * Advance the side-to-move clock by @p dt_ms. Flags on timeout.
     * No-op if clock off, game over, or not at tip.
     */
    void tick_clock(int dt_ms);
    /** After a successful move by @p mover: add increment to mover's clock. */
    void apply_clock_increment(Color mover);

    HandicapSettings const& handicap() const { return handicap_; }

    bool save_kifu(std::string const& path) const;
    bool load_kifu(std::string const& path);

private:
    void record_position();
    void update_end_conditions();
    void set_result(GameStatus st, Color win, std::string msg);
    /** Resolve entered-king impasse into @ref JishogiWin / @ref JishogiDraw. */
    void apply_jishogi_result();
    /**
     * Rebuild board by replaying the first @p ply history moves from the true
     * start (handicap position if enabled, else flat starting position).
     */
    void show_ply(std::size_t ply);

    Board board_;
    Color side_;
    Color start_side_;
    GameStatus status_;
    Color winner_;
    std::vector<HistoryEntry> history_;
    std::vector<HistoryEntry> redo_;
    std::size_t view_ply_;
    std::unordered_map<std::string, int> repetition_;
    std::string message_;
    bool resigned_;
    bool allow_review_fork_;
    bool hard_mode_;
    MoveGenerator move_gen_;

    bool clock_enabled_;
    int clock_main_ms_;
    int clock_inc_ms_;
    int clock_sente_ms_;
    int clock_gote_ms_;
    HandicapSettings handicap_;

    void truncate_future();
    void bind_default_move_gen();
    void flag_timeout(Color loser);
};

} // namespace shogi

#endif
