#include "notation.hpp"

#include "rules.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace shogi {
namespace {

bool parse_square(std::string const& s, std::size_t& i, Square& out) {
    if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) {
        return false;
    }
    int file = s[i] - '0';
    ++i;
    if (i >= s.size()) {
        return false;
    }
    char rk = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
    if (rk < 'a' || rk > 'i') {
        return false;
    }
    int rank = rk - 'a' + 1;
    ++i;
    out = Square{file, rank};
    return out.valid();
}

} // namespace

std::string Notation::game_header() {
    return "Shogi game (Western notation)\n";
}

std::string Notation::to_western(Board const& before, Color side, Move const& move) {
    std::ostringstream oss;

    if (move.is_drop) {
        oss << piece_letter(move.drop_type) << '*' << move.to.to_string();
        return oss.str();
    }

    Piece p = before.at(move.from).piece;
    oss << p.notation_token();

    // Disambiguate origin if needed
    std::vector<Move> legal = Rules::generate_legal(before, side);
    std::vector<Move> same_dest;
    std::copy_if(legal.begin(), legal.end(), std::back_inserter(same_dest), [&](Move const& m) {
        if (m.is_drop || m.to != move.to) {
            return false;
        }
        Piece q = before.at(m.from).piece;
        return q.type == p.type && q.promoted == p.promoted && q.color == p.color;
    });

    bool capture = before.at(move.to).occupied;
    if (same_dest.size() > 1) {
        oss << move.from.to_string();
    }

    oss << (capture ? 'x' : '-');
    oss << move.to.to_string();

    if (move.promotion == PromotionChoice::Promote) {
        oss << '+';
    } else if (move.promotion == PromotionChoice::Decline) {
        oss << '=';
    }
    return oss.str();
}

bool Notation::parse_western(Board const& before, Color side, std::string const& text, Move& out) {
    std::string s;
    s.reserve(text.size());
    for (char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            s.push_back(c);
        }
    }
    if (s.empty()) {
        return false;
    }

    std::size_t i = 0;
    bool promoted_piece = false;
    if (s[i] == '+') {
        promoted_piece = true;
        ++i;
    }
    if (i >= s.size()) {
        return false;
    }
    PieceType type = piece_from_letter(s[i]);
    ++i;

    if (i < s.size() && s[i] == '*') {
        ++i;
        Square to;
        if (!parse_square(s, i, to) || i != s.size()) {
            return false;
        }
        out = Move::drop(type, to);
        return Rules::is_legal(before, side, out);
    }

    Square maybe_from;
    bool has_from = false;
    if (i + 1 < s.size() && std::isdigit(static_cast<unsigned char>(s[i])) &&
        std::isalpha(static_cast<unsigned char>(s[i + 1]))) {
        // Could be from-square or we need to peek for x/-
        std::size_t j = i;
        Square sq;
        if (parse_square(s, j, sq)) {
            if (j < s.size() && (s[j] == 'x' || s[j] == '-')) {
                has_from = true;
                maybe_from = sq;
                i = j;
            }
        }
    }

    if (i >= s.size() || (s[i] != 'x' && s[i] != '-')) {
        return false;
    }
    ++i;
    Square to;
    if (!parse_square(s, i, to)) {
        return false;
    }

    PromotionChoice promo = PromotionChoice::None;
    if (i < s.size()) {
        if (s[i] == '+') {
            promo = PromotionChoice::Promote;
            ++i;
        } else if (s[i] == '=') {
            promo = PromotionChoice::Decline;
            ++i;
        }
    }
    if (i != s.size()) {
        return false;
    }

    auto legal = Rules::generate_legal(before, side);
    std::vector<Move> matches;
    std::copy_if(legal.begin(), legal.end(), std::back_inserter(matches), [&](Move const& m) {
        if (m.is_drop || m.to != to) {
            return false;
        }
        Piece p = before.at(m.from).piece;
        if (p.type != type || p.promoted != promoted_piece) {
            return false;
        }
        if (has_from && m.from != maybe_from) {
            return false;
        }
        if (promo == PromotionChoice::None) {
            return m.promotion == PromotionChoice::None ||
                   m.promotion == PromotionChoice::Decline; // allow ambiguous decline omission? no
        }
        return m.promotion == promo;
    });

    // If notation omitted promotion when forced/only one option
    if (matches.empty() && promo == PromotionChoice::None) {
        std::copy_if(legal.begin(), legal.end(), std::back_inserter(matches), [&](Move const& m) {
            if (m.is_drop || m.to != to) {
                return false;
            }
            Piece p = before.at(m.from).piece;
            if (p.type != type || p.promoted != promoted_piece) {
                return false;
            }
            if (has_from && m.from != maybe_from) {
                return false;
            }
            return true;
        });
        if (matches.size() == 1) {
            out = matches.front();
            return true;
        }
        return false;
    }

    if (matches.size() != 1) {
        return false;
    }
    out = matches.front();
    return true;
}

} // namespace shogi
