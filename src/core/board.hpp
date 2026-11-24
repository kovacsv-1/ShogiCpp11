#ifndef SHOGI_BOARD_HPP
#define SHOGI_BOARD_HPP

/**
 * @file board.hpp
 * @brief Board state: 81 squares, hands, apply/undo moves.
 */

#include "piece.hpp"
#include "types.hpp"

#include <array>
#include <map>
#include <string>
#include <utility>

namespace shogi {

/** Occupancy of a single square. */
struct PieceSlot {
    bool occupied;
    Piece piece;

    PieceSlot() : occupied(false), piece() {}
    explicit PieceSlot(Piece p) : occupied(true), piece(p) {}

    void clear() { occupied = false; }

    void set(Piece p) {
        occupied = true;
        piece = p;
    }

    bool operator==(PieceSlot const& o) const {
        if (occupied != o.occupied) {
            return false;
        }
        return !occupied || piece == o.piece;
    }
};

/**
 * A board move or drop. After @ref Board::apply, capture fields are filled in.
 */
struct Move {
    bool is_drop;
    Square from;
    Square to;
    PieceType drop_type;
    PromotionChoice promotion;
    bool is_capture;
    Piece captured; ///< Valid if @ref is_capture

    Move()
        : is_drop(false),
          from(),
          to(),
          drop_type(PieceType::Pawn),
          promotion(PromotionChoice::None),
          is_capture(false),
          captured() {}

    /** Construct a non-drop move (promotion optional). */
    static Move board_move(Square f, Square t, PromotionChoice promo = PromotionChoice::None) {
        Move m;
        m.is_drop = false;
        m.from = f;
        m.to = t;
        m.promotion = promo;
        return m;
    }

    /** Construct a drop of @p type onto @p t. */
    static Move drop(PieceType type, Square t) {
        Move m;
        m.is_drop = true;
        m.drop_type = type;
        m.to = t;
        m.promotion = PromotionChoice::None;
        return m;
    }

    bool operator==(Move const& o) const {
        if (is_drop != o.is_drop || to != o.to || promotion != o.promotion) {
            return false;
        }
        if (is_drop) {
            return drop_type == o.drop_type;
        }
        return from == o.from;
    }
};

/** Pieces in hand: type → count. */
using Hand = std::map<PieceType, int>;

/**
 * Mutable position: squares + both hands.
 *
 * Copyable by value; move constructor/assignment avoid copying 81 slots and two maps.
 */
class Board {
public:
    Board();
    Board(Board const&) = default;
    Board& operator=(Board const&) = default;
    /** Move avoids copying 81 slots + two hand maps. */
    Board(Board&& other) noexcept;
    Board& operator=(Board&& other) noexcept;

    /** Classic starting setup. */
    static Board starting_position();

    PieceSlot const& at(Square s) const;
    PieceSlot& at(Square s);

    Hand& hand(Color c);
    Hand const& hand(Color c) const;

    void clear();
    void place(Square s, Piece p);
    void remove(Square s);

    void add_to_hand(Color c, PieceType t);
    bool take_from_hand(Color c, PieceType t);
    int hand_count(Color c, PieceType t) const;

    /**
     * Apply a structurally valid move (board or drop). Fills capture info on @p m.
     * @param m   Move to apply (updated with capture metadata).
     * @param side Side making the move.
     */
    void apply(Move& m, Color side);

    /**
     * Undo using capture/promotion info stored on @p m (after a matching apply).
     * @param m                  Move previously passed to apply.
     * @param side               Side that made the move.
     * @param moved_before_promo Piece identity before promotion was applied.
     */
    void undo(Move const& m, Color side, Piece moved_before_promo);

    /** Compact key for repetition detection (SFEN-like, includes side to move). */
    std::string position_key(Color side_to_move) const;

    bool operator==(Board const& o) const;

private:
    std::array<PieceSlot, 81> squares_;
    Hand sente_hand_;
    Hand gote_hand_;
};

} // namespace shogi

#endif
