#ifndef SHOGI_PIECE_HPP
#define SHOGI_PIECE_HPP

/**
 * @file piece.hpp
 * @brief Piece value type and promotion helpers.
 */

#include "types.hpp"
#include <string>

namespace shogi {

/** A coloured piece, optionally promoted. */
struct Piece {
    Color color;
    PieceType type;
    bool promoted;

    Piece() : color(Color::Sente), type(PieceType::Pawn), promoted(false) {}
    Piece(Color c, PieceType t, bool p = false) : color(c), type(t), promoted(p) {}

    bool operator==(Piece const& o) const {
        return color == o.color && type == o.type && promoted == o.promoted;
    }

    bool operator!=(Piece const& o) const {
        return !(*this == o);
    }

    /** Captured pieces enter the hand unpromoted. */
    Piece demoted() const {
        return Piece{color, type, false};
    }

    /** ASCII letter used in SFEN / Western notation (case encodes colour). */
    char letter() const;
    /** Short display token for UI / debug. */
    std::string notation_token() const;
};

/** @return Uppercase SFEN letter for an unpromoted type (e.g. @c 'P'). */
char piece_letter(PieceType t);
/** Inverse of @ref piece_letter; undefined for invalid chars. */
PieceType piece_from_letter(char c);
/** Human-readable name (English), optional promoted form. */
std::string piece_display_name(PieceType t, bool promoted);

/** @return True if @p t can ever promote (not Gold/King). */
bool can_promote(PieceType t);
/** @return True if landing on @p to forces promotion for @p t. */
bool must_promote_on(PieceType t, Color c, Square to);
/** @return True if @p s is in @p c's promotion zone (last three ranks). */
bool in_promotion_zone(Color c, Square s);

} // namespace shogi

#endif
