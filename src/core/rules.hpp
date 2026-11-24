#ifndef SHOGI_RULES_HPP
#define SHOGI_RULES_HPP

/**
 * @file rules.hpp
 * @brief Static rule helpers: attacks, legality, mate, promotion.
 */

#include "board.hpp"

#include <vector>

namespace shogi {

/**
 * Pure functions over a @ref Board — no game history.
 * Pseudo-legal generators ignore own-king safety; @ref generate_legal filters check,
 * nifu and uchifuzume.
 */
class Rules {
public:
    /** @return King square for @p c, or invalid square if missing. */
    static Square find_king(Board const& board, Color c);

    /** @return True if @p by attacks @p target. */
    static bool square_attacked(Board const& board, Square target, Color by);

    /** @return True if @p c's king is attacked. */
    static bool in_check(Board const& board, Color c);

    /** Pseudo-legal board moves for the piece on @p from (ignores own-king safety). */
    static void generate_piece_moves(Board const& board, Square from, std::vector<Move>& out);

    /** Pseudo-legal drops for @p side. */
    static void generate_drops(Board const& board, Color side, std::vector<Move>& out);

    /** All pseudo-legal moves (board + drops), still ignoring king safety / uchifuzume. */
    static std::vector<Move> generate_pseudolegal(Board const& board, Color side);

    /** Fully legal moves including check filter, nifu, uchifuzume. */
    static std::vector<Move> generate_legal(Board const& board, Color side);

    /** @return True if @p move is among @ref generate_legal. */
    static bool is_legal(Board const& board, Color side, Move move);

    /** @return True if @p side is in check and has no legal move. */
    static bool is_checkmate(Board const& board, Color side);

    /** Two unpromoted pawns of @p side on the same file after a pawn drop on @p to. */
    static bool drop_causes_nifu(Board const& board, Color side, Square to);

    /** Drop square where the piece could never move (e.g. pawn/lance on last rank). */
    static bool is_dead_drop_square(PieceType t, Color side, Square to);

    /** After applying @p move, does opponent have zero legal replies while in check? */
    static bool move_delivers_mate(Board const& board, Color side, Move move);

    static bool promotion_available(Board const& board, Move const& move, Color side);
    static bool promotion_forced(Board const& board, Move const& move, Color side);

    /**
     * Jishogi (impasse) helpers — Tokyo / hobby manual style.
     * Rook & Bishop = 5 (ignore promotion); King = 0; all other pieces = 1
     * (board + hand). Threshold 24. Ending is via @ref Game::claim_jishogi.
     */
    /** @return True if both kings sit in their own promotion zones (entered). */
    static bool both_kings_entered(Board const& board);
    /** Piece-point total for @p c (see above). */
    static int jishogi_points(Board const& board, Color c);
};

} // namespace shogi

#endif
