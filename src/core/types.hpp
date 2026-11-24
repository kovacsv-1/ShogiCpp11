#ifndef SHOGI_TYPES_HPP
#define SHOGI_TYPES_HPP

/**
 * @file types.hpp
 * @brief Core enumerations and board coordinates (Western file/rank).
 */

#include <cstddef>
#include <functional>
#include <string>

namespace shogi {

/** Side to move / piece ownership: Sente (Black) moves first by default. */
enum class Color { Sente, Gote };

/** @return The opposite side. */
inline Color opposite(Color c) {
    return c == Color::Sente ? Color::Gote : Color::Sente;
}

/** Unpromoted piece kinds (promotion is a flag on @ref Piece). */
enum class PieceType {
    Pawn,
    Lance,
    Knight,
    Silver,
    Gold,
    Bishop,
    Rook,
    King
};

/** How a @ref Game ended (or still @c Playing). */
enum class GameStatus {
    Playing,
    Checkmate,
    Resignation,
    IllegalMove,
    KingCaptured,
    DrawRepetition,
    DrawPerpetualCheck,
    TimeForfeit,
    JishogiWin,  ///< Impasse claimed: one side under 24 points
    JishogiDraw  ///< Impasse claimed: both ≥24 (or tie under 24)
};

/** Promotion choice attached to a board move. */
enum class PromotionChoice {
    None,
    Promote,
    Decline
};

/**
 * Board square in Western notation: file 1..9, rank 1..9 (1 = a … 9 = i).
 * Index 0..80 is file-major: @c (file-1) + (rank-1)*9.
 */
struct Square {
    int file; ///< 1..9 (Western)
    int rank; ///< 1..9 where 1=a .. 9=i

    Square() : file(0), rank(0) {}
    Square(int f, int r) : file(f), rank(r) {}

    /** @return True if file/rank are on the 9×9 board. */
    bool valid() const {
        return file >= 1 && file <= 9 && rank >= 1 && rank <= 9;
    }

    /** @return Flat index in @c [0,80]. */
    int index() const {
        return (file - 1) + (rank - 1) * 9;
    }

    /** @param idx Flat index in @c [0,80]. */
    static Square from_index(int idx) {
        return Square{(idx % 9) + 1, (idx / 9) + 1};
    }

    /** @return Western token such as @c "7f". */
    std::string to_string() const {
        return std::to_string(file) + static_cast<char>('a' + rank - 1);
    }

    bool operator==(Square const& o) const {
        return file == o.file && rank == o.rank;
    }

    bool operator!=(Square const& o) const {
        return !(*this == o);
    }
};

/** Hash for unordered containers keyed by @ref Square. */
inline std::size_t square_hash(Square s) {
    return static_cast<std::size_t>(s.index());
}

} // namespace shogi

#endif
