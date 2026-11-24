#include "rules.hpp"

#include <algorithm>
#include <array>

namespace shogi {
namespace {

/** Sente advances toward rank a (decreasing rank); Gote toward rank i. */
int forward_dir(Color c) {
    return c == Color::Sente ? -1 : 1;
}

void add_step(Board const& board, Square from, int df, int dr, Color side, std::vector<Move>& out) {
    Square to{from.file + df, from.rank + dr};
    if (!to.valid()) {
        return;
    }
    auto const& slot = board.at(to);
    if (slot.occupied && slot.piece.color == side) {
        return;
    }
    out.push_back(Move::board_move(from, to));
}

void add_ray(Board const& board, Square from, int df, int dr, Color side, std::vector<Move>& out) {
    Square cur{from.file + df, from.rank + dr};
    while (cur.valid()) {
        auto const& slot = board.at(cur);
        if (slot.occupied) {
            if (slot.piece.color != side) {
                out.push_back(Move::board_move(from, cur));
            }
            break;
        }
        out.push_back(Move::board_move(from, cur));
        cur.file += df;
        cur.rank += dr;
    }
}

void gold_moves(Board const& board, Square from, Color side, std::vector<Move>& out) {
    int f = forward_dir(side);
    add_step(board, from, 0, f, side, out);
    add_step(board, from, -1, f, side, out);
    add_step(board, from, 1, f, side, out);
    add_step(board, from, -1, 0, side, out);
    add_step(board, from, 1, 0, side, out);
    add_step(board, from, 0, -f, side, out);
}

void silver_moves(Board const& board, Square from, Color side, std::vector<Move>& out) {
    int f = forward_dir(side);
    add_step(board, from, 0, f, side, out);
    add_step(board, from, -1, f, side, out);
    add_step(board, from, 1, f, side, out);
    add_step(board, from, -1, -f, side, out);
    add_step(board, from, 1, -f, side, out);
}

void king_like(Board const& board, Square from, Color side, std::vector<Move>& out) {
    static std::array<std::pair<int, int>, 8> const dirs = {{
        {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}
    }};
    for (auto const& d : dirs) {
        add_step(board, from, d.first, d.second, side, out);
    }
}

void expand_promotions(Board const& board, Color side, std::vector<Move>& moves) {
    std::vector<Move> expanded;
    expanded.reserve(moves.size() * 2);
    for (auto const& m : moves) {
        if (m.is_drop) {
            expanded.push_back(m);
            continue;
        }
        Piece p = board.at(m.from).piece;
        bool zone = in_promotion_zone(side, m.from) || in_promotion_zone(side, m.to);
        bool can = !p.promoted && can_promote(p.type) && zone;
        bool forced = can && must_promote_on(p.type, side, m.to);
        if (forced) {
            Move promo = m;
            promo.promotion = PromotionChoice::Promote;
            expanded.push_back(promo);
        } else if (can) {
            Move promo = m;
            promo.promotion = PromotionChoice::Promote;
            Move decline = m;
            decline.promotion = PromotionChoice::Decline;
            expanded.push_back(promo);
            expanded.push_back(decline);
        } else {
            Move plain = m;
            plain.promotion = PromotionChoice::None;
            expanded.push_back(plain);
        }
    }
    moves.swap(expanded);
}

} // namespace

Square Rules::find_king(Board const& board, Color c) {
    for (int i = 0; i < 81; ++i) {
        Square s = Square::from_index(i);
        auto const& slot = board.at(s);
        if (slot.occupied && slot.piece.color == c && slot.piece.type == PieceType::King) {
            return s;
        }
    }
    return Square{};
}

bool Rules::square_attacked(Board const& board, Square target, Color by) {
    // Generate opponent piece attacks by scanning all pieces of `by`.
    for (int i = 0; i < 81; ++i) {
        Square from = Square::from_index(i);
        auto const& slot = board.at(from);
        if (!slot.occupied || slot.piece.color != by) {
            continue;
        }
        std::vector<Move> attacks;
        generate_piece_moves(board, from, attacks);
        if (std::any_of(attacks.begin(), attacks.end(), [&](Move const& m) {
                return m.to == target;
            })) {
            return true;
        }
    }
    return false;
}

bool Rules::in_check(Board const& board, Color c) {
    Square king = find_king(board, c);
    if (!king.valid()) {
        return false;
    }
    return square_attacked(board, king, opposite(c));
}

void Rules::generate_piece_moves(Board const& board, Square from, std::vector<Move>& out) {
    auto const& slot = board.at(from);
    if (!slot.occupied) {
        return;
    }
    Piece const& p = slot.piece;
    Color side = p.color;
    int fdir = forward_dir(side);

    if (p.promoted) {
        switch (p.type) {
        case PieceType::Pawn:
        case PieceType::Lance:
        case PieceType::Knight:
        case PieceType::Silver:
            gold_moves(board, from, side, out);
            return;
        case PieceType::Bishop:
            add_ray(board, from, -1, -1, side, out);
            add_ray(board, from, 1, -1, side, out);
            add_ray(board, from, -1, 1, side, out);
            add_ray(board, from, 1, 1, side, out);
            add_step(board, from, 0, -1, side, out);
            add_step(board, from, 0, 1, side, out);
            add_step(board, from, -1, 0, side, out);
            add_step(board, from, 1, 0, side, out);
            return;
        case PieceType::Rook:
            add_ray(board, from, 0, -1, side, out);
            add_ray(board, from, 0, 1, side, out);
            add_ray(board, from, -1, 0, side, out);
            add_ray(board, from, 1, 0, side, out);
            add_step(board, from, -1, -1, side, out);
            add_step(board, from, 1, -1, side, out);
            add_step(board, from, -1, 1, side, out);
            add_step(board, from, 1, 1, side, out);
            return;
        default:
            break;
        }
    }

    switch (p.type) {
    case PieceType::Pawn:
        add_step(board, from, 0, fdir, side, out);
        break;
    case PieceType::Lance:
        add_ray(board, from, 0, fdir, side, out);
        break;
    case PieceType::Knight:
        add_step(board, from, -1, 2 * fdir, side, out);
        add_step(board, from, 1, 2 * fdir, side, out);
        break;
    case PieceType::Silver:
        silver_moves(board, from, side, out);
        break;
    case PieceType::Gold:
        gold_moves(board, from, side, out);
        break;
    case PieceType::King:
        king_like(board, from, side, out);
        break;
    case PieceType::Bishop:
        add_ray(board, from, -1, -1, side, out);
        add_ray(board, from, 1, -1, side, out);
        add_ray(board, from, -1, 1, side, out);
        add_ray(board, from, 1, 1, side, out);
        break;
    case PieceType::Rook:
        add_ray(board, from, 0, -1, side, out);
        add_ray(board, from, 0, 1, side, out);
        add_ray(board, from, -1, 0, side, out);
        add_ray(board, from, 1, 0, side, out);
        break;
    }
}

bool Rules::is_dead_drop_square(PieceType t, Color side, Square to) {
    if (t == PieceType::Pawn || t == PieceType::Lance) {
        return (side == Color::Sente && to.rank == 1) || (side == Color::Gote && to.rank == 9);
    }
    if (t == PieceType::Knight) {
        return (side == Color::Sente && to.rank <= 2) || (side == Color::Gote && to.rank >= 8);
    }
    return false;
}

bool Rules::drop_causes_nifu(Board const& board, Color side, Square to) {
    int const file = to.file;
    std::array<int, 9> ranks = {{1, 2, 3, 4, 5, 6, 7, 8, 9}};
    int pawns = static_cast<int>(std::count_if(ranks.begin(), ranks.end(), [&](int r) {
        Square s{file, r};
        auto const& slot = board.at(s);
        return slot.occupied && slot.piece.color == side && slot.piece.type == PieceType::Pawn &&
               !slot.piece.promoted;
    }));
    return pawns >= 1;
}

void Rules::generate_drops(Board const& board, Color side, std::vector<Move>& out) {
    Hand const& h = board.hand(side);
    if (h.empty()) {
        return;
    }

    for (auto const& kv : h) {
        if (kv.second <= 0) {
            continue;
        }
        PieceType t = kv.first;
        for (int i = 0; i < 81; ++i) {
            Square to = Square::from_index(i);
            if (board.at(to).occupied) {
                continue;
            }
            if (is_dead_drop_square(t, side, to)) {
                continue;
            }
            if (t == PieceType::Pawn && drop_causes_nifu(board, side, to)) {
                continue;
            }
            out.push_back(Move::drop(t, to));
        }
    }
}

std::vector<Move> Rules::generate_pseudolegal(Board const& board, Color side) {
    std::vector<Move> moves;
    for (int i = 0; i < 81; ++i) {
        Square from = Square::from_index(i);
        auto const& slot = board.at(from);
        if (slot.occupied && slot.piece.color == side) {
            generate_piece_moves(board, from, moves);
        }
    }
    generate_drops(board, side, moves);
    expand_promotions(board, side, moves);
    return moves;
}

bool Rules::move_delivers_mate(Board const& board, Color side, Move move) {
    Board tmp = board;
    tmp.apply(move, side);
    Color opp = opposite(side);
    return in_check(tmp, opp) && generate_legal(tmp, opp).empty();
}

std::vector<Move> Rules::generate_legal(Board const& board, Color side) {
    std::vector<Move> pseudo = generate_pseudolegal(board, side);
    std::vector<Move> legal;
    legal.reserve(pseudo.size());

    std::copy_if(pseudo.begin(), pseudo.end(), std::back_inserter(legal), [&](Move m) {
        Board tmp = board;
        tmp.apply(m, side);
        if (in_check(tmp, side)) {
            return false;
        }
        // Uchifuzume: pawn drop that delivers mate is illegal
        if (m.is_drop && m.drop_type == PieceType::Pawn && in_check(tmp, opposite(side))) {
            Color opp = opposite(side);
            std::vector<Move> replies = generate_pseudolegal(tmp, opp);
            bool any_escape = std::any_of(replies.begin(), replies.end(), [&](Move r) {
                Board t2 = tmp;
                t2.apply(r, opp);
                return !in_check(t2, opp);
            });
            if (!any_escape) {
                return false;
            }
        }
        return true;
    });

    return legal;
}

bool Rules::is_legal(Board const& board, Color side, Move move) {
    auto legal = generate_legal(board, side);
    return std::any_of(legal.begin(), legal.end(), [&](Move const& m) {
        return m == move;
    });
}

bool Rules::is_checkmate(Board const& board, Color side) {
    return in_check(board, side) && generate_legal(board, side).empty();
}

bool Rules::promotion_available(Board const& board, Move const& move, Color side) {
    if (move.is_drop) {
        return false;
    }
    Piece p = board.at(move.from).piece;
    if (p.promoted || !can_promote(p.type)) {
        return false;
    }
    return in_promotion_zone(side, move.from) || in_promotion_zone(side, move.to);
}

bool Rules::promotion_forced(Board const& board, Move const& move, Color side) {
    if (!promotion_available(board, move, side)) {
        return false;
    }
    Piece p = board.at(move.from).piece;
    return must_promote_on(p.type, side, move.to);
}

bool Rules::both_kings_entered(Board const& board) {
    Square sk = find_king(board, Color::Sente);
    Square gk = find_king(board, Color::Gote);
    if (!sk.valid() || !gk.valid()) {
        return false;
    }
    return in_promotion_zone(Color::Sente, sk) && in_promotion_zone(Color::Gote, gk);
}

int Rules::jishogi_points(Board const& board, Color c) {
    auto piece_pts = [](PieceType t) -> int {
        if (t == PieceType::King) {
            return 0;
        }
        if (t == PieceType::Rook || t == PieceType::Bishop) {
            return 5;
        }
        return 1;
    };
    int pts = 0;
    for (int i = 0; i < 81; ++i) {
        PieceSlot const& slot = board.at(Square::from_index(i));
        if (slot.occupied && slot.piece.color == c) {
            pts += piece_pts(slot.piece.type);
        }
    }
    PieceType const types[] = {PieceType::Pawn,   PieceType::Lance, PieceType::Knight,
                               PieceType::Silver, PieceType::Gold,  PieceType::Bishop,
                               PieceType::Rook};
    for (PieceType t : types) {
        pts += board.hand_count(c, t) * piece_pts(t);
    }
    return pts;
}

} // namespace shogi
