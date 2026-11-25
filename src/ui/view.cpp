#include "view.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace shogi {
namespace ui {

int text_px(TTF_Font* font, std::string const& text) {
    if (!font || text.empty()) {
        return 0;
    }
    int w = 0;
    int h = 0;
    if (TTF_SizeUTF8(font, text.c_str(), &w, &h) != 0) {
        return 0;
    }
    return w;
}

namespace {

void pop_utf8(std::string& s) {
    if (s.empty()) {
        return;
    }
    s.pop_back();
    while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) {
        s.pop_back();
    }
}

std::size_t utf8_span(std::string const& text, std::size_t at) {
    if (at >= text.size()) {
        return 0;
    }
    unsigned char c = static_cast<unsigned char>(text[at]);
    std::size_t n = 1;
    if ((c & 0xE0) == 0xC0) {
        n = 2;
    } else if ((c & 0xF0) == 0xE0) {
        n = 3;
    } else if ((c & 0xF8) == 0xF0) {
        n = 4;
    }
    if (at + n > text.size()) {
        return 1;
    }
    return n;
}

std::vector<std::string> wrap_text(TTF_Font* font, std::string const& text, int max_px) {
    std::vector<std::string> lines;
    std::string cur;
    for (std::size_t i = 0; i < text.size();) {
        std::string ch = text.substr(i, utf8_span(text, i));
        i += ch.size();
        std::string trial = cur + ch;
        if (cur.empty() || text_px(font, trial) <= max_px) {
            cur = trial;
            continue;
        }
        std::size_t sp = cur.rfind(' ');
        if (sp != std::string::npos && sp > 0) {
            lines.push_back(cur.substr(0, sp));
            cur = cur.substr(sp + 1);
            if (ch != " ") {
                cur += ch;
            }
        } else {
            lines.push_back(cur);
            cur = (ch == " ") ? std::string() : ch;
        }
    }
    if (!cur.empty()) {
        lines.push_back(cur);
    }
    return lines;
}

} // namespace

std::string clip_text(TTF_Font* font, std::string const& text, int max_px) {
    if (max_px <= 0) {
        return std::string();
    }
    if (text_px(font, text) <= max_px) {
        return text;
    }
    std::string cut = text;
    std::string const ell = "...";
    while (!cut.empty() && text_px(font, cut + ell) > max_px) {
        pop_utf8(cut);
    }
    return cut + ell;
}

BoardView::BoardView(SpriteAssets& assets)
    : assets_(assets), history_scroll_(0), history_scroll_ply_(static_cast<std::size_t>(-1)) {}

SDL_Rect BoardView::history_panel_rect() const {
    int const x = board_origin_x + 9 * cell_size + 36;
    // Below Menu + gap + Claim Impasse row.
    SDL_Rect claim = claim_impasse_btn();
    int const y = claim.y + claim.h + 10;
    int const w = window_w - x - 16;
    int const h = sente_hand_y - y - 8;
    return SDL_Rect{x, y, w, std::max(0, h)};
}

bool BoardView::hit_history_panel(int x, int y) const {
    SDL_Rect r = history_panel_rect();
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

void BoardView::scroll_history(int wheel_y) const {
    // SDL wheel: positive y = scroll away / up content → show older (increase scroll)
    history_scroll_ += wheel_y;
    if (history_scroll_ < 0) {
        history_scroll_ = 0;
    }
}

SDL_Rect BoardView::board_square_rect(Square s) const {
    int col = 9 - s.file;
    int row = s.rank - 1;
    return SDL_Rect{board_origin_x + col * cell_size, board_origin_y + row * cell_size, cell_size,
                    cell_size};
}

SDL_Rect BoardView::side_btn(int index) const {
    return SDL_Rect{board_origin_x + 9 * cell_size + 36, board_origin_y + index * 52, 160, 44};
}

SDL_Rect BoardView::claim_impasse_btn() const {
    // Gap under Menu (index 4) before the claim / blank spacer row.
    int const gap = 22;
    return SDL_Rect{board_origin_x + 9 * cell_size + 36, board_origin_y + 5 * 52 + gap, 160, 44};
}


void BoardView::draw_text(SDL_Renderer* renderer, int x, int y, std::string const& text,
                          SDL_Color color, bool small) const {
    TTF_Font* font = small ? assets_.font_small() : assets_.font();
    if (!font) {
        return;
    }
    SurfacePtr surf(TTF_RenderUTF8_Blended(font, text.c_str(), color));
    if (!surf) {
        return;
    }
    TexturePtr tex(SDL_CreateTextureFromSurface(renderer, surf.get()));
    if (!tex) {
        return;
    }
    SDL_Rect dst{x, y, surf->w, surf->h};
    SDL_RenderCopy(renderer, tex.get(), nullptr, &dst);
}

void BoardView::draw_button(SDL_Renderer* renderer, SDL_Rect rect, char const* label,
                            bool active, bool focused, bool danger) const {
    // Inactive (+ focused) matches locked "Play against engine" chrome.
    if (!active) {
        if (focused) {
            SDL_SetRenderDrawColor(renderer, 48, 52, 40, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 20, 22, 28, 255);
        }
    } else if (danger) {
        SDL_SetRenderDrawColor(renderer, focused ? 180 : 150, focused ? 50 : 40, focused ? 45 : 40,
                               255);
    } else if (focused) {
        SDL_SetRenderDrawColor(renderer, 70, 90, 50, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 70, 110, 70, 255);
    }
    SDL_RenderFillRect(renderer, &rect);
    if (!active) {
        if (focused) {
            SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 45, 45, 50, 255);
        }
    } else if (danger) {
        SDL_SetRenderDrawColor(renderer, focused ? 255 : 220, focused ? 120 : 80, focused ? 120 : 80,
                               255);
    } else if (focused) {
        SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    }
    SDL_RenderDrawRect(renderer, &rect);
    if (focused || danger) {
        SDL_Rect r2{rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2};
        SDL_RenderDrawRect(renderer, &r2);
    }
    SDL_Color label_col =
        !active ? (focused ? SDL_Color{140, 140, 120, 255} : SDL_Color{70, 70, 75, 255})
                : (danger ? SDL_Color{255, 220, 220, 255}
                          : (focused ? SDL_Color{220, 180, 60, 255} : SDL_Color{230, 230, 230, 255}));
    draw_text(renderer, rect.x + 12, rect.y + 10, label, label_col, true);
}

void BoardView::draw_board_grid(SDL_Renderer* renderer) const {
    for (int r = 0; r < 9; ++r) {
        for (int f = 0; f < 9; ++f) {
            Square s{9 - f, r + 1};
            SDL_Rect rect = board_square_rect(s);
            bool dark = ((f + r) % 2) == 0;
            if (dark) {
                SDL_SetRenderDrawColor(renderer, 196, 160, 96, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 222, 190, 130, 255);
            }
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 80, 50, 20, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    }

    // Axis labels inside edge squares: ranks (a–i) top-left of left file,
    // files (9–1) bottom-right of bottom rank.
    SDL_Color label{50, 35, 20, 255};
    TTF_Font* font = assets_.font_small();
    for (int rank = 1; rank <= 9; ++rank) {
        SDL_Rect rect = board_square_rect(Square{9, rank});
        char ch = static_cast<char>('a' + rank - 1);
        draw_text(renderer, rect.x + 3, rect.y + 1, std::string(1, ch), label, true);
    }
    for (int file = 1; file <= 9; ++file) {
        SDL_Rect rect = board_square_rect(Square{file, 9});
        std::string text = std::to_string(file);
        int tw = 0;
        int th = 0;
        if (font) {
            TTF_SizeUTF8(font, text.c_str(), &tw, &th);
        }
        draw_text(renderer, rect.x + rect.w - tw - 3, rect.y + rect.h - th - 1, text, label,
                  true);
    }
}

void BoardView::draw_pieces(SDL_Renderer* renderer, Board const& board,
                            Square const* hide_square) const {
    SDL_Texture* sheet = assets_.sheet();
    if (!sheet) {
        return;
    }
    for (int i = 0; i < 81; ++i) {
        Square s = Square::from_index(i);
        if (hide_square && s == *hide_square) {
            continue;
        }
        auto const& slot = board.at(s);
        if (!slot.occupied) {
            continue;
        }
        SDL_Rect src = assets_.piece_rect(slot.piece);
        SDL_Rect cell = board_square_rect(s);
        cell.x += 3;
        cell.y += 2;
        cell.w -= 6;
        cell.h -= 4;
        SDL_Rect dst = assets_.fit_in(cell);
        SDL_RenderCopy(renderer, sheet, &src, &dst);
    }
}

std::array<PieceType, 7> const& BoardView::hand_type_order() {
    static std::array<PieceType, 7> const order = {
        {PieceType::Rook, PieceType::Bishop, PieceType::Gold, PieceType::Silver, PieceType::Knight,
         PieceType::Lance, PieceType::Pawn}};
    return order;
}

SDL_Rect BoardView::hand_slot_rect(HandNavFocus which, int slot) const {
    if (slot < 0) {
        slot = 0;
    }
    if (slot > 6) {
        slot = 6;
    }
    int y0 = (which == HandNavFocus::Gote) ? gote_hand_y : sente_hand_y;
    return SDL_Rect{20 + slot * hand_gap, y0 + 26, hand_cell, hand_cell};
}

void BoardView::draw_hand_cursor(SDL_Renderer* renderer, HandNavFocus hand_nav,
                                 int hand_slot) const {
    if (hand_nav == HandNavFocus::None) {
        return;
    }
    SDL_Rect r = hand_slot_rect(hand_nav, hand_slot);
    SDL_SetRenderDrawColor(renderer, 120, 200, 255, 255);
    SDL_RenderDrawRect(renderer, &r);
    SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
    SDL_RenderDrawRect(renderer, &r2);
}

void BoardView::draw_hand(SDL_Renderer* renderer, Color c, Hand const& hand, int y0,
                          bool name_lit) const {
    SDL_Color col{220, 220, 220, 255};
    SDL_Color accent{220, 180, 60, 255};
    std::string title = (c == Color::Sente) ? "Sente" : "Gote";
    draw_text(renderer, 20, y0, title, name_lit ? accent : col, false);

    std::array<PieceType, 7> const& order = hand_type_order();
    SDL_Texture* sheet = assets_.sheet();
    for (std::size_t i = 0; i < order.size(); ++i) {
        PieceType t = order[i];
        auto it = hand.find(t);
        int n = (it == hand.end()) ? 0 : it->second;
        int x = 20 + static_cast<int>(i) * hand_gap;
        SDL_Rect box{x, y0 + 26, hand_cell, hand_cell};
        if (n > 0) {
            Piece p{c, t, false};
            if (sheet) {
                SDL_Rect src = assets_.piece_rect(p);
                SDL_Rect dst = assets_.fit_in(box);
                SDL_RenderCopy(renderer, sheet, &src, &dst);
            }
            // Count to the right of the cell so it does not cover the piece.
            draw_text(renderer, x + hand_cell + 2, y0 + 26 + hand_cell - 18, "x" + std::to_string(n),
                      col, true);
        } else {
            SDL_SetRenderDrawColor(renderer, 45, 50, 60, 255);
            SDL_RenderDrawRect(renderer, &box);
        }
    }
}

void BoardView::draw_move_history(SDL_Renderer* renderer, Game const& game) const {
    SDL_Rect panel = history_panel_rect();
    if (panel.w <= 0 || panel.h <= 0) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 32, 36, 44, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 70, 70, 80, 255);
    SDL_RenderDrawRect(renderer, &panel);

    auto const& hist = game.history();
    int const n = static_cast<int>(hist.size());
    int const rows = (n + 1) / 2;
    int const visible = std::max(1, (panel.h - 8) / history_row_h);

    // Keep highlighted ply row in view when the review cursor moves.
    std::size_t const ply = game.view_ply();
    if (ply != history_scroll_ply_) {
        history_scroll_ply_ = ply;
        int hl_move = static_cast<int>(ply) - 1;
        if (hl_move >= 0) {
            int hl_row = hl_move / 2;
            if (hl_row < history_scroll_) {
                history_scroll_ = hl_row;
            } else if (hl_row >= history_scroll_ + visible) {
                history_scroll_ = hl_row - visible + 1;
            }
        } else {
            history_scroll_ = 0;
        }
    }

    int max_scroll = std::max(0, rows - visible);
    if (history_scroll_ > max_scroll) {
        history_scroll_ = max_scroll;
    }
    if (history_scroll_ < 0) {
        history_scroll_ = 0;
    }

    SDL_Color text{200, 200, 200, 255};
    SDL_Color accent{220, 180, 60, 255};
    SDL_Color dim{120, 120, 130, 255};

    int const col_w = (panel.w - 16) / 2;
    int const hl_idx = static_cast<int>(ply) - 1;

    SDL_RenderSetClipRect(renderer, &panel);
    for (int row = history_scroll_; row < rows && row < history_scroll_ + visible; ++row) {
        int y = panel.y + 4 + (row - history_scroll_) * history_row_h;
        int i0 = row * 2;
        int i1 = i0 + 1;

        auto draw_cell = [&](int move_i, int x) {
            if (move_i >= n) {
                return;
            }
            bool lit = (move_i == hl_idx);
            if (lit) {
                SDL_Rect bg{x - 2, y - 1, col_w, history_row_h};
                SDL_SetRenderDrawColor(renderer, 55, 60, 40, 255);
                SDL_RenderFillRect(renderer, &bg);
            }
            std::string label;
            if (move_i % 2 == 0) {
                label = std::to_string(move_i / 2 + 1) + ". " + hist[static_cast<std::size_t>(move_i)].notation;
            } else {
                label = hist[static_cast<std::size_t>(move_i)].notation;
            }
            draw_text(renderer, x, y, label, lit ? accent : text, true);
        };

        draw_cell(i0, panel.x + 8);
        draw_cell(i1, panel.x + 8 + col_w);
    }
    SDL_RenderSetClipRect(renderer, nullptr);

    if (rows == 0) {
        draw_text(renderer, panel.x + 8, panel.y + 8, "No moves yet", dim, true);
    } else if (max_scroll > 0) {
        // Tiny scroll hint on the right edge.
        draw_text(renderer, panel.x + panel.w - 14, panel.y + 2, "^", dim, true);
        draw_text(renderer, panel.x + panel.w - 14, panel.y + panel.h - 18, "v", dim, true);
    }
}

void BoardView::promote_squares(Square from, Square to, Square& keep_sq, Square& promote_sq) {
    (void)from;
    // Destination always shows the unpromoted piece. The promoted option is one square
    // up or down — toward the farther edge (away from whichever side the piece is nearer).
    keep_sq = to;
    int dist_top = to.rank - 1;     // rank a
    int dist_bottom = 9 - to.rank;  // rank i
    if (dist_top <= dist_bottom) {
        // Closer to (or tied with) the top → open downward.
        promote_sq = Square{to.file, to.rank + 1};
    } else {
        // Closer to the bottom → open upward.
        promote_sq = Square{to.file, to.rank - 1};
    }
    if (!promote_sq.valid()) {
        // Safety fallback (should not trigger with the distance rule).
        promote_sq = dist_top <= dist_bottom ? Square{to.file, to.rank - 1}
                                             : Square{to.file, to.rank + 1};
        if (!promote_sq.valid()) {
            promote_sq = to;
        }
    }
}

void BoardView::draw_promote_choice_square(SDL_Renderer* renderer, Square s,
                                           Piece const& shown) const {
    SDL_Rect cell = board_square_rect(s);
    SDL_SetRenderDrawColor(renderer, 40, 90, 180, 255);
    SDL_RenderFillRect(renderer, &cell);
    SDL_SetRenderDrawColor(renderer, 180, 210, 255, 255);
    SDL_RenderDrawRect(renderer, &cell);

    SDL_Texture* sheet = assets_.sheet();
    if (!sheet) {
        return;
    }
    SDL_Rect inner{cell.x + 3, cell.y + 2, cell.w - 6, cell.h - 4};
    SDL_Rect src = assets_.piece_rect(shown);
    SDL_Rect dst = assets_.fit_in(inner);
    SDL_RenderCopy(renderer, sheet, &src, &dst);
}

void BoardView::draw_promote_picker(SDL_Renderer* renderer, Square from, Square to,
                                    Piece const& piece) const {
    Square keep_sq;
    Square promote_sq;
    promote_squares(from, to, keep_sq, promote_sq);

    Piece keep = piece;
    keep.promoted = false;
    Piece promo = piece;
    promo.promoted = true;

    // Draw "further" square first if it overlaps visually — both are full cells.
    // Prefer drawing keep then promote so promote stays readable when squares differ.
    draw_promote_choice_square(renderer, keep_sq, keep);
    draw_promote_choice_square(renderer, promote_sq, promo);
}

PromotePick BoardView::hit_promote(int x, int y, Square from, Square to) const {
    Square keep_sq;
    Square promote_sq;
    promote_squares(from, to, keep_sq, promote_sq);

    auto in_sq = [&](Square s) {
        SDL_Rect cell = board_square_rect(s);
        return x >= cell.x && x < cell.x + cell.w && y >= cell.y && y < cell.y + cell.h;
    };

    if (in_sq(promote_sq)) {
        return PromotePick::Promote;
    }
    if (in_sq(keep_sq)) {
        return PromotePick::Keep;
    }
    return PromotePick::None;
}

void BoardView::draw_marks(SDL_Renderer* renderer, BoardMarks const& marks) const {
    SDL_Texture* red = assets_.mark_red();
    SDL_Texture* arrow = assets_.mark_arrow();
    SDL_Texture* line = assets_.mark_line();
    SDL_Texture* lineend = assets_.mark_lineend();

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (Square s : marks.squares()) {
        SDL_Rect r = board_square_rect(s);
        if (red) {
            SDL_RenderCopy(renderer, red, nullptr, &r);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 110);
            SDL_RenderFillRect(renderer, &r);
        }
    }

    for (BoardArrow const& a : marks.arrows()) {
        SDL_Rect rf = board_square_rect(a.from);
        SDL_Rect rt = board_square_rect(a.to);
        float x0 = static_cast<float>(rf.x + rf.w / 2);
        float y0 = static_cast<float>(rf.y + rf.h / 2);
        float x1 = static_cast<float>(rt.x + rt.w / 2);
        float y1 = static_cast<float>(rt.y + rt.h / 2);
        float dx = x1 - x0;
        float dy = y1 - y0;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) {
            continue;
        }

        // SDL_RenderCopyEx angle: degrees clockwise; sprites point "up" (0°).
        double angle = std::atan2(static_cast<double>(dx), static_cast<double>(-dy)) * 180.0 /
                       3.14159265358979323846;

        int thickness = std::max(10, cell_size / 5);
        int head = std::max(22, cell_size / 3);
        int tail = std::max(18, cell_size / 4);

        // Shorten shaft so it meets the arrowhead / lineend.
        float usable = len - static_cast<float>(head) * 0.55f - static_cast<float>(tail) * 0.35f;
        if (usable < 8.0f) {
            usable = len * 0.4f;
        }
        float ux = dx / len;
        float uy = dy / len;
        float sx = x0 + ux * (static_cast<float>(tail) * 0.35f);
        float sy = y0 + uy * (static_cast<float>(tail) * 0.35f);
        float ex = sx + ux * usable;
        float ey = sy + uy * usable;
        float mx = (sx + ex) * 0.5f;
        float my = (sy + ey) * 0.5f;

        if (line) {
            SDL_Rect shaft{static_cast<int>(std::round(mx - thickness / 2.0f)),
                           static_cast<int>(std::round(my - usable / 2.0f)), thickness,
                           static_cast<int>(std::round(usable))};
            SDL_Point center{shaft.w / 2, shaft.h / 2};
            SDL_RenderCopyEx(renderer, line, nullptr, &shaft, angle, &center, SDL_FLIP_NONE);
        }

        if (lineend) {
            SDL_Rect endr{static_cast<int>(std::round(x0 - thickness / 2.0f)),
                          static_cast<int>(std::round(y0 - tail / 2.0f)), thickness, tail};
            SDL_Point center{endr.w / 2, endr.h / 2};
            SDL_RenderCopyEx(renderer, lineend, nullptr, &endr, angle, &center, SDL_FLIP_NONE);
        }

        if (arrow) {
            SDL_Rect ar{static_cast<int>(std::round(x1 - head / 2.0f)),
                        static_cast<int>(std::round(y1 - head / 2.0f)), head, head};
            SDL_Point center{ar.w / 2, ar.h / 2};
            SDL_RenderCopyEx(renderer, arrow, nullptr, &ar, angle, &center, SDL_FLIP_NONE);
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void BoardView::draw_furigoma(SDL_Renderer* renderer, FurigomaPaint const* pawns, int count,
                              Color starter, bool keyboard_dismiss) const {
    if (!pawns || count <= 0) {
        return;
    }
    int board_w = 9 * cell_size;
    int board_h = 9 * cell_size;
    SDL_Rect overlay{board_origin_x, board_origin_y, board_w, board_h};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 12, 14, 20, 150);
    SDL_RenderFillRect(renderer, &overlay);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    int const gap = 14;
    int piece_box = cell_size - 8;
    int total_w = count * piece_box + (count - 1) * gap;
    int x0 = board_origin_x + (board_w - total_w) / 2;
    int y0 = board_origin_y + (board_h - piece_box) / 2 - 24;

    SDL_Texture* sheet = assets_.sheet();
    for (int i = 0; i < count; ++i) {
        FurigomaPaint const& p = pawns[i];
        Piece piece{p.color, PieceType::Pawn, p.promoted};
        SDL_Rect src = assets_.piece_rect(piece);
        SDL_Rect dst{x0 + i * (piece_box + gap), y0, piece_box, piece_box};
        dst = assets_.fit_in(dst);
        if (sheet) {
            SDL_Point center{dst.w / 2, dst.h / 2};
            SDL_RenderCopyEx(renderer, sheet, &src, &dst, p.angle_deg, &center, SDL_FLIP_NONE);
        }
    }

    std::string line = std::string("Furigoma — ") +
                       (starter == Color::Sente ? "Sente" : "Gote") + " starts";
    std::string hint =
        keyboard_dismiss ? "Press Tab to continue" : "Click the board to continue";
    int tw = 0;
    int th = 0;
    if (assets_.font_small()) {
        TTF_SizeUTF8(assets_.font_small(), line.c_str(), &tw, &th);
    }
    int tx = board_origin_x + (board_w - tw) / 2;
    int ty = y0 + piece_box + 18;
    draw_text(renderer, tx, ty, line, SDL_Color{240, 220, 120, 255}, true);
    if (assets_.font_tiny()) {
        TTF_SizeUTF8(assets_.font_tiny(), hint.c_str(), &tw, &th);
    }
    tx = board_origin_x + (board_w - tw) / 2;
    draw_text(renderer, tx, ty + 22, hint, SDL_Color{180, 180, 170, 255}, true);
}

void BoardView::draw_drag_piece(SDL_Renderer* renderer, DragPaint const& drag) const {
    if (!drag.active || !assets_.sheet()) {
        return;
    }
    SDL_Rect src = assets_.piece_rect(drag.piece);
    int w = cell_size - 6;
    int h = cell_size - 4;
    SDL_Rect bounds{drag.x - w / 2, drag.y - h / 2, w, h};
    SDL_Rect dst = assets_.fit_in(bounds);
    SDL_SetTextureAlphaMod(assets_.sheet(), 230);
    SDL_RenderCopy(renderer, assets_.sheet(), &src, &dst);
    SDL_SetTextureAlphaMod(assets_.sheet(), 255);
}

void BoardView::draw(SDL_Renderer* renderer, Game const& game, Square const* selected,
                     std::vector<Square> const& highlights, PieceType const* selected_drop,
                     Square const* promote_at, Square const* promote_from,
                     Piece const* promote_piece, BoardMarks const* marks, DragPaint const* drag,
                     std::string const& status_line, int side_focus,
                     Square const* board_cursor, bool cursor_green,
                     FurigomaPaint const* furigoma, int furigoma_count,
                     Color furigoma_starter, HandNavFocus hand_nav, int hand_slot,
                     bool resign_armed, bool furigoma_tab_dismiss, bool new_armed,
                     bool claim_available, bool claim_armed) const {
    SDL_SetRenderDrawColor(renderer, 28, 32, 40, 255);
    SDL_RenderClear(renderer);

    bool const playing = (game.status() == GameStatus::Playing);
    bool const true_draw = (game.status() == GameStatus::DrawRepetition ||
                            game.status() == GameStatus::JishogiDraw);
    bool gote_lit = playing ? (game.side_to_move() == Color::Gote)
                            : (!true_draw && game.winner() == Color::Gote);
    bool sente_lit = playing ? (game.side_to_move() == Color::Sente)
                             : (!true_draw && game.winner() == Color::Sente);
    draw_hand(renderer, Color::Gote, game.board().hand(Color::Gote), gote_hand_y, gote_lit);
    if (hand_nav == HandNavFocus::Gote) {
        draw_hand_cursor(renderer, hand_nav, hand_slot);
    }
    draw_board_grid(renderer);

    for (Square h : highlights) {
        SDL_Rect r = board_square_rect(h);
        SDL_SetRenderDrawColor(renderer, 80, 160, 80, 120);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Layer: loser's king — independent of selection.
    if (game.status() == GameStatus::Checkmate || game.status() == GameStatus::Resignation ||
        game.status() == GameStatus::IllegalMove || game.status() == GameStatus::JishogiWin ||
        game.status() == GameStatus::KingCaptured || game.status() == GameStatus::TimeForfeit) {
        Square loser_king = Rules::find_king(game.board(), opposite(game.winner()));
        if (loser_king.valid()) {
            SDL_Rect r = board_square_rect(loser_king);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 200, 40, 40, 130);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
            SDL_RenderDrawRect(renderer, &r);
            SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
            SDL_RenderDrawRect(renderer, &r2);
        }
    }

    // Layer: selection — stacked on top; does not clear the loser-king mark.
    if (selected) {
        SDL_Rect r = board_square_rect(*selected);
        SDL_SetRenderDrawColor(renderer, 220, 180, 40, 255);
        SDL_RenderDrawRect(renderer, &r);
        SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
        SDL_RenderDrawRect(renderer, &r2);
    }

    Square const* hide = promote_from;
    Square drag_hide;
    if (drag && drag->active && selected) {
        drag_hide = *selected;
        hide = &drag_hide;
    }
    draw_pieces(renderer, game.board(), hide);

    if (marks) {
        draw_marks(renderer, *marks);
    }

    if (promote_at && promote_from && promote_piece) {
        draw_promote_picker(renderer, *promote_from, *promote_at, *promote_piece);
    }

    // Keyboard cursor on top (after promote blue edges so the focus color stays visible).
    if (board_cursor && board_cursor->valid()) {
        SDL_Rect r = board_square_rect(*board_cursor);
        if (cursor_green) {
            SDL_SetRenderDrawColor(renderer, 255, 170, 40, 255); // amber ≠ promote blue
        } else {
            SDL_SetRenderDrawColor(renderer, 120, 200, 255, 255);
        }
        SDL_RenderDrawRect(renderer, &r);
        SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
        SDL_RenderDrawRect(renderer, &r2);
        if (cursor_green) {
            SDL_Rect r3{r.x + 2, r.y + 2, r.w - 4, r.h - 4};
            SDL_RenderDrawRect(renderer, &r3);
        }
    }

    draw_hand(renderer, Color::Sente, game.board().hand(Color::Sente), sente_hand_y, sente_lit);
    if (hand_nav == HandNavFocus::Sente) {
        draw_hand_cursor(renderer, hand_nav, hand_slot);
    }

    if (drag && drag->active) {
        draw_drag_piece(renderer, *drag);
    }

    // Armed resign/new stay red even if focus moved away (clears on click/activate/Tab).
    bool const furi_up = (furigoma != nullptr && furigoma_count > 0);
    // Furigoma: Resign/Save inactive but still hover/focus like locked Engine.
    draw_button(renderer, resign_btn(), "Resign", !furi_up, side_focus == 0,
                resign_armed && !furi_up);
    draw_button(renderer, save_btn(), "Save", !furi_up, side_focus == 1);
    draw_button(renderer, load_btn(), "Load", true, side_focus == 2);
    draw_button(renderer, new_btn(), "New", true, side_focus == 3, new_armed);
    draw_button(renderer, menu_btn(), "Menu", true, side_focus == 4);
    SDL_Rect claim_r = claim_impasse_btn();
    if (claim_available) {
        draw_button(renderer, claim_r, "Claim Impasse", true, side_focus == 5, claim_armed);
    } else {
        // Same idea as locked Koma: blank footprint, no label.
        SDL_SetRenderDrawColor(renderer, 28, 32, 40, 255);
        SDL_RenderFillRect(renderer, &claim_r);
    }

    draw_move_history(renderer, game);

    SDL_Color st{210, 210, 210, 255};
    // Status bar — reserved bottom strip
    SDL_Rect status_bg{0, status_y - 6, window_w, window_h - (status_y - 6)};
    SDL_SetRenderDrawColor(renderer, 20, 24, 30, 255);
    SDL_RenderFillRect(renderer, &status_bg);
    std::string line = status_line;
    if (game.status() != GameStatus::Playing && !game.message().empty()) {
        line = game.message() + "  |  " + status_line;
    }
    if (selected_drop) {
        line = std::string("Drop: ") + piece_letter(*selected_drop) + "  |  " + line;
    }
    draw_text(renderer, 20, status_y, line, st, false);

    if (furigoma && furigoma_count > 0) {
        draw_furigoma(renderer, furigoma, furigoma_count, furigoma_starter, furigoma_tab_dismiss);
    }

    SDL_RenderPresent(renderer);
}

void BoardView::draw_learn(SDL_Renderer* renderer, learn::LearnSession const& session,
                           Square const* selected, std::vector<Square> const& highlights,
                           PieceType const* selected_drop, Square const* promote_at,
                           Square const* promote_from, Piece const* promote_piece,
                           BoardMarks const* marks, DragPaint const* drag,
                           int side_focus, Square const* board_cursor,
                           bool cursor_green, HandNavFocus hand_nav, int hand_slot) const {
    SDL_SetRenderDrawColor(renderer, 28, 32, 40, 255);
    SDL_RenderClear(renderer);

    Board const& board = session.board();
    bool gote_lit = session.side_to_move() == Color::Gote;
    bool sente_lit = session.side_to_move() == Color::Sente;
    draw_hand(renderer, Color::Gote, board.hand(Color::Gote), gote_hand_y, gote_lit);
    if (hand_nav == HandNavFocus::Gote) {
        draw_hand_cursor(renderer, hand_nav, hand_slot);
    }
    draw_board_grid(renderer);

    for (Square h : highlights) {
        SDL_Rect r = board_square_rect(h);
        SDL_SetRenderDrawColor(renderer, 80, 160, 80, 120);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    if (selected) {
        SDL_Rect r = board_square_rect(*selected);
        SDL_SetRenderDrawColor(renderer, 220, 180, 40, 255);
        SDL_RenderDrawRect(renderer, &r);
        SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
        SDL_RenderDrawRect(renderer, &r2);
    }

    Square const* hide = promote_from;
    Square drag_hide;
    if (drag && drag->active && selected) {
        drag_hide = *selected;
        hide = &drag_hide;
    }
    draw_pieces(renderer, board, hide);

    Square keep_sq;
    Square promo_sq;
    bool hiding_promo = false;
    if (promote_at && promote_from) {
        promote_squares(*promote_from, *promote_at, keep_sq, promo_sq);
        hiding_promo = true;
    }

    // Obstacle stars on top of placeholder pawns
    for (Square ob : session.remaining_obstacles()) {
        if (hiding_promo && (ob == keep_sq || ob == promo_sq)) {
            continue;
        }
        SDL_Rect r = board_square_rect(ob);
        int cx = r.x + r.w / 2;
        int cy = r.y + r.h / 2;
        SDL_SetRenderDrawColor(renderer, 255, 210, 40, 220);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (int dy = -10; dy <= 10; ++dy) {
            for (int dx = -10; dx <= 10; ++dx) {
                int ax = dx < 0 ? -dx : dx;
                int ay = dy < 0 ? -dy : dy;
                if (ax + ay <= 10 || dx * dx + dy * dy <= 28) {
                    SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
                }
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    if (marks) {
        draw_marks(renderer, *marks);
    }

    if (promote_at && promote_from && promote_piece) {
        draw_promote_picker(renderer, *promote_from, *promote_at, *promote_piece);
    }

    if (board_cursor && board_cursor->valid()) {
        SDL_Rect r = board_square_rect(*board_cursor);
        if (cursor_green) {
            SDL_SetRenderDrawColor(renderer, 255, 170, 40, 255); // amber ≠ promote blue
        } else {
            SDL_SetRenderDrawColor(renderer, 120, 200, 255, 255);
        }
        SDL_RenderDrawRect(renderer, &r);
        SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
        SDL_RenderDrawRect(renderer, &r2);
        if (cursor_green) {
            SDL_Rect r3{r.x + 2, r.y + 2, r.w - 4, r.h - 4};
            SDL_RenderDrawRect(renderer, &r3);
        }
    }

    draw_hand(renderer, Color::Sente, board.hand(Color::Sente), sente_hand_y, sente_lit);
    if (hand_nav == HandNavFocus::Sente) {
        draw_hand_cursor(renderer, hand_nav, hand_slot);
    }

    if (drag && drag->active) {
        draw_drag_piece(renderer, *drag);
    }

    bool next_ok = session.result() == learn::LevelResult::Success;
    draw_button(renderer, resign_btn(), "Retry", true, side_focus == 0);
    draw_button(renderer, save_btn(), "Next", next_ok, side_focus == 1);
    draw_button(renderer, load_btn(), "Prev", true, side_focus == 2);
    draw_button(renderer, new_btn(), "Stages", true, side_focus == 3);
    draw_button(renderer, menu_btn(), "Menu", true, side_focus == 4);

    SDL_Color st{210, 210, 210, 255};
    learn::LearnStage const& stg = session.stage();
    learn::LearnLevel const& lv = session.level();
    int const panel_x = board_origin_x + 9 * cell_size + 36;
    int const panel_w = window_w - panel_x - 16;
    int text_y = board_origin_y + 5 * 52 + 8;

    std::string header = std::string(stg.title) + "  " +
                         std::to_string(session.level_index() + 1) + "/" +
                         std::to_string(session.level_count());
    header = clip_text(assets_.font(), header, panel_w);
    draw_text(renderer, panel_x, text_y, header, st, false);
    text_y += 32;

    // Goal beside the buttons. Width is measured, not guessed, so a line
    // cannot run past the right edge of the window.
    SDL_Color goal_col{200, 200, 190, 255};
    int const line_h = 22;
    int const text_bottom = status_y - 70;
    std::vector<std::string> lines = wrap_text(assets_.font_small(), lv.goal, panel_w);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (text_y + line_h > text_bottom) {
            draw_text(renderer, panel_x, text_y, "...", goal_col, true);
            text_y += line_h;
            break;
        }
        std::string line = lines[i];
        bool const last_fit = (i + 1 < lines.size() && text_y + 2 * line_h > text_bottom);
        if (last_fit) {
            line = clip_text(assets_.font_small(), line + " ...", panel_w);
        }
        draw_text(renderer, panel_x, text_y, line, goal_col, true);
        text_y += line_h;
        if (last_fit) {
            break;
        }
    }

    text_y += 8;
    std::string score_line;
    if (session.result() == learn::LevelResult::Success) {
        score_line = "score: " + std::to_string(session.last_score()) + "/3";
        score_line = clip_text(assets_.font(), score_line, panel_w);
        draw_text(renderer, panel_x, text_y, score_line, SDL_Color{220, 180, 60, 255}, false);
    } else {
        score_line = "Target moves: " + std::to_string(lv.nb_moves);
        if (session.player_moves() > 0) {
            score_line += "  (" + std::to_string(session.player_moves()) + " used)";
        }
        score_line = clip_text(assets_.font_small(), score_line, panel_w);
        draw_text(renderer, panel_x, text_y, score_line, SDL_Color{160, 160, 160, 255}, true);
    }
    text_y += 28;

    if (session.result() == learn::LevelResult::Failure) {
        draw_text(renderer, panel_x, text_y,
                  clip_text(assets_.font_small(), "Failed — press R to retry", panel_w),
                  SDL_Color{220, 100, 100, 255}, true);
    } else if (session.result() == learn::LevelResult::Success) {
        draw_text(renderer, panel_x, text_y, "Success!", SDL_Color{120, 200, 120, 255}, true);
    }

    SDL_Rect status_bg{0, status_y - 6, window_w, window_h - (status_y - 6)};
    SDL_SetRenderDrawColor(renderer, 20, 24, 30, 255);
    SDL_RenderFillRect(renderer, &status_bg);
    if (selected_drop) {
        std::string drop_line = std::string("Drop: ") + piece_letter(*selected_drop);
        draw_text(renderer, 20, status_y, drop_line, st, false);
    }

    SDL_RenderPresent(renderer);
}

UiHit BoardView::hit_test(int x, int y) const {
    UiHit hit;

    auto in_rect = [&](SDL_Rect r) {
        return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
    };

    if (in_rect(resign_btn())) {
        hit.kind = UiHitKind::ButtonResign;
        return hit;
    }
    if (in_rect(save_btn())) {
        hit.kind = UiHitKind::ButtonSave;
        return hit;
    }
    if (in_rect(load_btn())) {
        hit.kind = UiHitKind::ButtonLoad;
        return hit;
    }
    if (in_rect(new_btn())) {
        hit.kind = UiHitKind::ButtonNew;
        return hit;
    }
    if (in_rect(menu_btn())) {
        hit.kind = UiHitKind::ButtonMenu;
        return hit;
    }
    if (in_rect(claim_impasse_btn())) {
        hit.kind = UiHitKind::ButtonClaimImpasse;
        return hit;
    }

    if (x >= board_origin_x && x < board_origin_x + 9 * cell_size && y >= board_origin_y &&
        y < board_origin_y + 9 * cell_size) {
        int col = (x - board_origin_x) / cell_size;
        int row = (y - board_origin_y) / cell_size;
        hit.kind = UiHitKind::Board;
        hit.square = Square{9 - col, row + 1};
        return hit;
    }

    auto hand_hit = [&](int y0, UiHitKind kind) {
        std::array<PieceType, 7> const& order = hand_type_order();
        int hy = y0 + 26;
        for (std::size_t i = 0; i < order.size(); ++i) {
            int hx = 20 + static_cast<int>(i) * hand_gap;
            SDL_Rect r{hx, hy, hand_cell, hand_cell};
            if (in_rect(r)) {
                hit.kind = kind;
                hit.hand_type = order[i];
                return true;
            }
        }
        return false;
    };

    if (hand_hit(gote_hand_y, UiHitKind::HandGote)) {
        return hit;
    }
    if (hand_hit(sente_hand_y, UiHitKind::HandSente)) {
        return hit;
    }

    return hit;
}

} // namespace ui
} // namespace shogi
