#include "handicap.hpp"

namespace shogi {
namespace {

/** Lance on the bishop's side (JSA): Sente 9i, Gote 1a. */
Square bishop_side_lance(Color uwate) {
    return uwate == Color::Sente ? Square{9, 9} : Square{1, 1};
}

Square rook_sq(Color c) {
    return c == Color::Sente ? Square{2, 8} : Square{8, 2};
}

Square bishop_sq(Color c) {
    return c == Color::Sente ? Square{8, 8} : Square{2, 2};
}

Square other_lance(Color c) {
    return c == Color::Sente ? Square{1, 9} : Square{9, 1};
}

Square knight_bishop_side(Color c) {
    return c == Color::Sente ? Square{8, 9} : Square{2, 1};
}

Square knight_rook_side(Color c) {
    return c == Color::Sente ? Square{2, 9} : Square{8, 1};
}

void remove_sq(Board& b, Square s) {
    if (s.valid() && b.at(s).occupied) {
        b.remove(s);
    }
}

} // namespace

char const* handicap_label(int rank_diff) {
    if (rank_diff <= 1) {
        return "Even (weaker first)";
    }
    if (rank_diff == 2) {
        return "Lance";
    }
    if (rank_diff == 3) {
        return "Bishop";
    }
    if (rank_diff == 4) {
        return "Rook";
    }
    if (rank_diff == 5) {
        return "Rook-Lance";
    }
    if (rank_diff <= 7) {
        return "2-piece";
    }
    if (rank_diff <= 9) {
        return "4-piece";
    }
    return "6-piece";
}

Board make_handicap_position(HandicapSettings const& settings, Color& first) {
    Board b = Board::starting_position();
    if (!settings.enabled || settings.rank_diff <= 1) {
        first = opposite(settings.uwate); // shitate first
        return b;
    }

    Color u = settings.uwate;
    int d = settings.rank_diff;
    if (d == 2) {
        remove_sq(b, bishop_side_lance(u));
    } else if (d == 3) {
        remove_sq(b, bishop_sq(u));
    } else if (d == 4) {
        remove_sq(b, rook_sq(u));
    } else if (d == 5) {
        remove_sq(b, rook_sq(u));
        remove_sq(b, bishop_side_lance(u));
    } else if (d <= 7) {
        remove_sq(b, rook_sq(u));
        remove_sq(b, bishop_sq(u));
    } else if (d <= 9) {
        remove_sq(b, rook_sq(u));
        remove_sq(b, bishop_sq(u));
        remove_sq(b, bishop_side_lance(u));
        remove_sq(b, other_lance(u));
    } else {
        remove_sq(b, rook_sq(u));
        remove_sq(b, bishop_sq(u));
        remove_sq(b, bishop_side_lance(u));
        remove_sq(b, other_lance(u));
        remove_sq(b, knight_bishop_side(u));
        remove_sq(b, knight_rook_side(u));
    }

    first = u; // uwate moves first
    return b;
}

} // namespace shogi
