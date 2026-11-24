#include "piece.hpp"

namespace shogi {

char piece_letter(PieceType t) {
    switch (t) {
    case PieceType::Pawn:   return 'P';
    case PieceType::Lance:  return 'L';
    case PieceType::Knight: return 'N';
    case PieceType::Silver: return 'S';
    case PieceType::Gold:   return 'G';
    case PieceType::Bishop: return 'B';
    case PieceType::Rook:   return 'R';
    case PieceType::King:   return 'K';
    }
    return '?';
}

PieceType piece_from_letter(char c) {
    switch (c) {
    case 'P': case 'p': return PieceType::Pawn;
    case 'L': case 'l': return PieceType::Lance;
    case 'N': case 'n': return PieceType::Knight;
    case 'S': case 's': return PieceType::Silver;
    case 'G': case 'g': return PieceType::Gold;
    case 'B': case 'b': return PieceType::Bishop;
    case 'R': case 'r': return PieceType::Rook;
    case 'K': case 'k': return PieceType::King;
    default:            return PieceType::Pawn;
    }
}

std::string piece_display_name(PieceType t, bool promoted) {
    if (promoted) {
        switch (t) {
        case PieceType::Pawn:   return "+P";
        case PieceType::Lance:  return "+L";
        case PieceType::Knight: return "+N";
        case PieceType::Silver: return "+S";
        case PieceType::Bishop: return "+B";
        case PieceType::Rook:   return "+R";
        default: break;
        }
    }
    return std::string(1, piece_letter(t));
}

char Piece::letter() const {
    return piece_letter(type);
}

std::string Piece::notation_token() const {
    return piece_display_name(type, promoted);
}

bool can_promote(PieceType t) {
    return t == PieceType::Pawn || t == PieceType::Lance || t == PieceType::Knight ||
           t == PieceType::Silver || t == PieceType::Bishop || t == PieceType::Rook;
}

bool in_promotion_zone(Color c, Square s) {
    // Rank a=1 is Gote's back; i=9 is Sente's back (Hodges / Japanese correspondence).
    if (c == Color::Sente) {
        return s.rank <= 3; // a,b,c
    }
    return s.rank >= 7; // g,h,i
}

bool must_promote_on(PieceType t, Color c, Square to) {
    if (t == PieceType::Pawn || t == PieceType::Lance) {
        return (c == Color::Sente && to.rank == 1) || (c == Color::Gote && to.rank == 9);
    }
    if (t == PieceType::Knight) {
        return (c == Color::Sente && to.rank <= 2) || (c == Color::Gote && to.rank >= 8);
    }
    return false;
}

} // namespace shogi
