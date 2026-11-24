#include "board.hpp"

#include <cctype>
#include <sstream>

namespace shogi {

Board::Board() {
    clear();
}

Board::Board(Board&& other) noexcept
    : squares_(std::move(other.squares_)),
      sente_hand_(std::move(other.sente_hand_)),
      gote_hand_(std::move(other.gote_hand_)) {}

Board& Board::operator=(Board&& other) noexcept {
    if (this != &other) {
        squares_ = std::move(other.squares_);
        sente_hand_ = std::move(other.sente_hand_);
        gote_hand_ = std::move(other.gote_hand_);
    }
    return *this;
}

void Board::clear() {
    for (auto& s : squares_) {
        s.clear();
    }
    sente_hand_.clear();
    gote_hand_.clear();
}

PieceSlot const& Board::at(Square s) const {
    return squares_[static_cast<std::size_t>(s.index())];
}

PieceSlot& Board::at(Square s) {
    return squares_[static_cast<std::size_t>(s.index())];
}

Hand& Board::hand(Color c) {
    return c == Color::Sente ? sente_hand_ : gote_hand_;
}

Hand const& Board::hand(Color c) const {
    return c == Color::Sente ? sente_hand_ : gote_hand_;
}

void Board::place(Square s, Piece p) {
    at(s).set(p);
}

void Board::remove(Square s) {
    at(s).clear();
}

void Board::add_to_hand(Color c, PieceType t) {
    hand(c)[t] += 1;
}

bool Board::take_from_hand(Color c, PieceType t) {
    auto& h = hand(c);
    auto it = h.find(t);
    if (it == h.end() || it->second <= 0) {
        return false;
    }
    --(it->second);
    if (it->second == 0) {
        h.erase(it);
    }
    return true;
}

int Board::hand_count(Color c, PieceType t) const {
    auto const& h = hand(c);
    auto it = h.find(t);
    return it == h.end() ? 0 : it->second;
}

void Board::apply(Move& m, Color side) {
    m.is_capture = false;
    if (m.is_drop) {
        take_from_hand(side, m.drop_type);
        place(m.to, Piece{side, m.drop_type, false});
        return;
    }

    Piece moving = at(m.from).piece;
    remove(m.from);

    if (at(m.to).occupied) {
        m.is_capture = true;
        m.captured = at(m.to).piece;
        add_to_hand(side, m.captured.demoted().type);
    }

    if (m.promotion == PromotionChoice::Promote) {
        moving.promoted = true;
    }
    place(m.to, moving);
}

void Board::undo(Move const& m, Color side, Piece moved_before_promo) {
    if (m.is_drop) {
        remove(m.to);
        add_to_hand(side, m.drop_type);
        return;
    }

    remove(m.to);
    place(m.from, moved_before_promo);

    if (m.is_capture) {
        place(m.to, m.captured);
        take_from_hand(side, m.captured.demoted().type);
    }
}

Board Board::starting_position() {
    Board b;

    auto put_back = [&](Color c, int rank) {
        b.place(Square{1, rank}, Piece{c, PieceType::Lance});
        b.place(Square{2, rank}, Piece{c, PieceType::Knight});
        b.place(Square{3, rank}, Piece{c, PieceType::Silver});
        b.place(Square{4, rank}, Piece{c, PieceType::Gold});
        b.place(Square{5, rank}, Piece{c, PieceType::King});
        b.place(Square{6, rank}, Piece{c, PieceType::Gold});
        b.place(Square{7, rank}, Piece{c, PieceType::Silver});
        b.place(Square{8, rank}, Piece{c, PieceType::Knight});
        b.place(Square{9, rank}, Piece{c, PieceType::Lance});
    };

    // Sente on ranks i/h/g (9/8/7); Gote on a/b/c (1/2/3)
    put_back(Color::Sente, 9);
    put_back(Color::Gote, 1);

    // Sente: bishop 8h, rook 2h; Gote: bishop 2b, rook 8b
    b.place(Square{8, 8}, Piece{Color::Sente, PieceType::Bishop});
    b.place(Square{2, 8}, Piece{Color::Sente, PieceType::Rook});
    b.place(Square{2, 2}, Piece{Color::Gote, PieceType::Bishop});
    b.place(Square{8, 2}, Piece{Color::Gote, PieceType::Rook});

    for (int f = 1; f <= 9; ++f) {
        b.place(Square{f, 7}, Piece{Color::Sente, PieceType::Pawn});
        b.place(Square{f, 3}, Piece{Color::Gote, PieceType::Pawn});
    }

    return b;
}

std::string Board::position_key(Color side_to_move) const {
    std::ostringstream oss;
    oss << (side_to_move == Color::Sente ? 'B' : 'W');
    for (auto const& slot : squares_) {
        if (!slot.occupied) {
            oss << '.';
            continue;
        }
        char ch = slot.piece.letter();
        if (slot.piece.color == Color::Gote) {
            ch = static_cast<char>(std::tolower(ch));
        }
        if (slot.piece.promoted) {
            oss << '+';
        }
        oss << ch;
    }
    auto dump_hand = [&](Hand const& h) {
        oss << '[';
        for (auto const& kv : h) {
            oss << piece_letter(kv.first) << kv.second;
        }
        oss << ']';
    };
    dump_hand(sente_hand_);
    dump_hand(gote_hand_);
    return oss.str();
}

bool Board::operator==(Board const& o) const {
    return squares_ == o.squares_ && sente_hand_ == o.sente_hand_ && gote_hand_ == o.gote_hand_;
}

} // namespace shogi
