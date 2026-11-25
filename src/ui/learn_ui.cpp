#include "learn_ui.hpp"
#include "key_repeat.hpp"

#include "../core/rules.hpp"

#include <algorithm>
#include <cstdio>

namespace shogi {
namespace ui {
namespace {

int const kStageBtnX = 40;
int const kStageBtnW = 780;
int const kStageScoreX = kStageBtnX + kStageBtnW + 20;

SDL_Rect stage_row_rect(int index) {
    return SDL_Rect{kStageBtnX, 100 + index * 42, kStageBtnW, 38};
}

SDL_Rect browser_back_rect() {
    return SDL_Rect{40, BoardView::window_h - 70, 160, 48};
}

} // namespace

LearnUi::LearnUi()
    : mode_(Mode::Browser),
      browser_focus_(0),
      side_focus_(0),
      has_selection_(false),
      selected_(),
      has_drop_(false),
      drop_type_(PieceType::Pawn),
      asking_promote_(false),
      rmb_down_(false),
      rmb_from_(),
      lmb_down_(false),
      dragging_(false),
      drag_x_(0),
      drag_y_(0),
      lmb_down_x_(0),
      lmb_down_y_(0),
      drag_piece_(),
      keyboard_board_(false),
      board_nav_(false),
      board_cursor_(5, 5),
      hand_nav_(HandNavFocus::None),
      hand_slot_(0) {}

void LearnUi::open() {
    mode_ = Mode::Browser;
    browser_focus_ = 0;
    side_focus_ = 0;
    board_nav_ = false;
    progress_.load();
    marks_.clear();
    rmb_down_ = false;
    clear_selection();
}

void LearnUi::set_keyboard_board(bool on) {
    keyboard_board_ = on;
    if (!on) {
        board_nav_ = false;
        hand_nav_ = HandNavFocus::None;
    } else if (mode_ == Mode::Play) {
        board_nav_ = true;
        hand_nav_ = HandNavFocus::None;
    }
}

void LearnUi::move_board_cursor(int dfile, int drank) {
    if (asking_promote_) {
        Square keep_sq;
        Square promo_sq;
        BoardView::promote_squares(pending_move_.from, pending_move_.to, keep_sq, promo_sq);
        if (board_cursor_ == keep_sq) {
            board_cursor_ = promo_sq;
        } else {
            board_cursor_ = keep_sq;
        }
        hand_nav_ = HandNavFocus::None;
        (void)dfile;
        (void)drank;
        return;
    }

    if (hand_nav_ == HandNavFocus::Gote) {
        if (dfile != 0) {
            int s = hand_slot_ - dfile;
            if (s < 0) {
                s = 0;
            }
            if (s > 6) {
                s = 6;
            }
            hand_slot_ = s;
            return;
        }
        if (drank > 0) {
            hand_nav_ = HandNavFocus::None;
            board_cursor_ = Square{board_cursor_.file, 1};
        }
        return;
    }
    if (hand_nav_ == HandNavFocus::Sente) {
        if (dfile != 0) {
            int s = hand_slot_ - dfile;
            if (s < 0) {
                s = 0;
            }
            if (s > 6) {
                s = 6;
            }
            hand_slot_ = s;
            return;
        }
        if (drank < 0) {
            hand_nav_ = HandNavFocus::None;
            board_cursor_ = Square{board_cursor_.file, 9};
        }
        return;
    }

    int f = board_cursor_.file + dfile;
    int r = board_cursor_.rank + drank;
    if (f < 1) {
        f = 1;
    }
    if (f > 9) {
        f = 9;
    }
    if (r < 1) {
        hand_nav_ = HandNavFocus::Gote;
        board_cursor_ = Square{board_cursor_.file, 1};
        return;
    }
    if (r > 9) {
        hand_nav_ = HandNavFocus::Sente;
        board_cursor_ = Square{board_cursor_.file, 9};
        return;
    }
    board_cursor_ = Square{f, r};
}

void LearnUi::activate_board_cursor() {
    if (asking_promote_) {
        Square keep_sq;
        Square promo_sq;
        BoardView::promote_squares(pending_move_.from, pending_move_.to, keep_sq, promo_sq);
        if (board_cursor_ == promo_sq) {
            pending_move_.promotion = PromotionChoice::Promote;
            asking_promote_ = false;
            commit_move(pending_move_);
        } else if (board_cursor_ == keep_sq) {
            pending_move_.promotion = PromotionChoice::Decline;
            asking_promote_ = false;
            commit_move(pending_move_);
        } else {
            board_cursor_ = keep_sq;
        }
        return;
    }

    if (hand_nav_ != HandNavFocus::None) {
        bool const gote_turn = session_.side_to_move() == Color::Gote;
        bool const own_hand = (hand_nav_ == HandNavFocus::Gote) == gote_turn;
        if (own_hand) {
            PieceType t = BoardView::hand_type_order()[static_cast<std::size_t>(hand_slot_)];
            on_hand_click(t);
        }
        return;
    }

    if (session_.result() != learn::LevelResult::Playing) {
        return;
    }
    auto const& slot = session_.board().at(board_cursor_);
    if (has_drop_ || has_selection_) {
        try_move_to(board_cursor_);
        return;
    }
    if (slot.occupied && slot.piece.color == session_.side_to_move()) {
        has_selection_ = true;
        selected_ = board_cursor_;
        has_drop_ = false;
        drag_piece_ = slot.piece;
        recompute_highlights();
    }
}

void LearnUi::clear_selection() {
    has_selection_ = false;
    has_drop_ = false;
    asking_promote_ = false;
    dragging_ = false;
    lmb_down_ = false;
    highlights_.clear();
}

void LearnUi::recompute_highlights() {
    highlights_.clear();
    if (session_.result() != learn::LevelResult::Playing) {
        return;
    }
    auto legal = Rules::generate_legal(session_.board(), session_.side_to_move());
    if (has_drop_) {
        for (Move const& m : legal) {
            if (m.is_drop && m.drop_type == drop_type_) {
                highlights_.push_back(m.to);
            }
        }
        return;
    }
    if (!has_selection_) {
        return;
    }
    for (Move const& m : legal) {
        if (!m.is_drop && m.from == selected_) {
            if (std::find(highlights_.begin(), highlights_.end(), m.to) == highlights_.end()) {
                highlights_.push_back(m.to);
            }
        }
    }
}

void LearnUi::commit_move(Move m) {
    if (session_.try_player_move(m)) {
        if (session_.result() == learn::LevelResult::Success) {
            progress_.record(session_.stage_index(), session_.level_index(),
                             session_.last_score());
        }
        clear_selection();
    }
}

void LearnUi::on_hand_click(PieceType t) {
    if (session_.result() != learn::LevelResult::Playing) {
        return;
    }
    if (session_.board().hand_count(session_.side_to_move(), t) <= 0) {
        return;
    }
    has_selection_ = false;
    has_drop_ = true;
    drop_type_ = t;
    asking_promote_ = false;
    drag_piece_ = Piece{session_.side_to_move(), t, false};
    recompute_highlights();
}

bool LearnUi::try_move_to(Square to) {
    if (asking_promote_ || session_.result() != learn::LevelResult::Playing) {
        return false;
    }

    auto legal = Rules::generate_legal(session_.board(), session_.side_to_move());

    if (has_drop_) {
        Move m = Move::drop(drop_type_, to);
        auto it = std::find(legal.begin(), legal.end(), m);
        if (it != legal.end()) {
            commit_move(*it);
            return true;
        }
        clear_selection();
        return false;
    }

    if (!has_selection_) {
        return false;
    }
    if (to == selected_) {
        return false;
    }

    std::vector<Move> matches;
    for (Move const& m : legal) {
        if (!m.is_drop && m.from == selected_ && m.to == to) {
            matches.push_back(m);
        }
    }
    if (matches.empty()) {
        // Keep current selection; releasing over another piece must not select it.
        return false;
    }

    bool has_promo = false;
    bool has_decline = false;
    for (Move const& m : matches) {
        if (m.promotion == PromotionChoice::Promote) {
            has_promo = true;
        }
        if (m.promotion == PromotionChoice::Decline) {
            has_decline = true;
        }
    }
    if (has_promo && has_decline) {
        pending_move_ = Move::board_move(selected_, to, PromotionChoice::None);
        asking_promote_ = true;
        dragging_ = false;
        lmb_down_ = false;
        if (keyboard_board_) {
            Square keep_sq;
            Square promo_sq;
            BoardView::promote_squares(pending_move_.from, pending_move_.to, keep_sq, promo_sq);
            board_cursor_ = keep_sq;
            board_nav_ = true;
        }
        return true;
    }
    commit_move(matches.front());
    return true;
}

void LearnUi::end_lmb_drag(BoardView const& view, int x, int y) {
    if (!lmb_down_) {
        return;
    }
    UiHit hit = view.hit_test(x, y);
    bool was_dragging = dragging_;
    dragging_ = false;
    lmb_down_ = false;

    if (asking_promote_) {
        return;
    }
    if (hit.kind == UiHitKind::Board && (has_drop_ || has_selection_)) {
        if (was_dragging || hit.square != selected_ || has_drop_) {
            try_move_to(hit.square);
        }
    }
}

LearnAction LearnUi::activate_side_button(int index) {
    switch (index) {
    case 0: // Retry
        session_.restart_level();
        marks_.clear();
        clear_selection();
        if (keyboard_board_) {
            board_nav_ = true;
        }
        return LearnAction::None;
    case 1: // Next
        if (session_.result() == learn::LevelResult::Success) {
            if (!session_.next_level()) {
                mode_ = Mode::Browser;
                board_nav_ = false;
            } else if (keyboard_board_) {
                board_nav_ = true;
            }
            marks_.clear();
            clear_selection();
        }
        return LearnAction::None;
    case 2: // Prev
        session_.prev_level();
        marks_.clear();
        clear_selection();
        if (keyboard_board_) {
            board_nav_ = true;
        }
        return LearnAction::None;
    case 3: // Stages
        mode_ = Mode::Browser;
        board_nav_ = false;
        marks_.clear();
        clear_selection();
        return LearnAction::None;
    case 4: // Menu
        return LearnAction::BackToMenu;
    default:
        return LearnAction::None;
    }
}

LearnAction LearnUi::handle_event(SDL_Event const& e, BoardView const& view) {
    if (e.type == SDL_QUIT) {
        return LearnAction::QuitApp;
    }

    if (mode_ == Mode::Browser) {
        if (keyboard_board_) {
            if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEWHEEL ||
                e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                return LearnAction::None;
            }
        }

        auto in_rect = [](SDL_Rect r, int x, int y) {
            return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
        };
        int n = session_.stage_count();
        int const back_i = n; // focus index for Back after last stage

        if (e.type == SDL_MOUSEMOTION) {
            int mx = e.motion.x;
            int my = e.motion.y;
            if (in_rect(browser_back_rect(), mx, my)) {
                browser_focus_ = back_i;
            } else {
                for (int i = 0; i < n; ++i) {
                    if (in_rect(stage_row_rect(i), mx, my)) {
                        browser_focus_ = i;
                        break;
                    }
                }
            }
            return LearnAction::None;
        }

        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode sym = e.key.keysym.sym;
            bool const is_arrow = (sym == SDLK_UP || sym == SDLK_DOWN);
            if (e.key.repeat && !is_arrow) {
                return LearnAction::None;
            }
            if (is_arrow && !accept_nav_key(e.key.repeat != 0)) {
                return LearnAction::None;
            }
            if (!e.key.repeat && sym == SDLK_ESCAPE) {
                return LearnAction::BackToMenu;
            }
            int items = n + 1; // stages + Back
            if (browser_focus_ < 0 || browser_focus_ >= items) {
                browser_focus_ = 0;
            }
            if (sym == SDLK_UP) {
                browser_focus_ = (browser_focus_ + items - 1) % items;
            } else if (sym == SDLK_DOWN) {
                browser_focus_ = (browser_focus_ + 1) % items;
            } else if (!e.key.repeat && (sym == SDLK_RETURN || sym == SDLK_SPACE)) {
                if (browser_focus_ == back_i) {
                    return LearnAction::BackToMenu;
                }
                session_.start_stage(browser_focus_, 0);
                mode_ = Mode::Play;
                side_focus_ = 0;
                board_nav_ = keyboard_board_;
                board_cursor_ = Square{5, 5};
                marks_.clear();
                clear_selection();
            }
            return LearnAction::None;
        }

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            if (in_rect(browser_back_rect(), e.button.x, e.button.y)) {
                browser_focus_ = back_i;
                return LearnAction::BackToMenu;
            }
            for (int i = 0; i < n; ++i) {
                if (in_rect(stage_row_rect(i), e.button.x, e.button.y)) {
                    browser_focus_ = i;
                    session_.start_stage(i, 0);
                    mode_ = Mode::Play;
                    side_focus_ = 0;
                    board_nav_ = keyboard_board_;
                    board_cursor_ = Square{5, 5};
                    marks_.clear();
                    clear_selection();
                    return LearnAction::None;
                }
            }
        }
        return LearnAction::None;
    }

    // Play mode
    if (keyboard_board_) {
        if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEWHEEL ||
            e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            return LearnAction::None;
        }
    }

    if (e.type == SDL_KEYDOWN) {
        SDL_Keycode sym = e.key.keysym.sym;
        bool const is_arrow = (sym == SDLK_LEFT || sym == SDLK_RIGHT || sym == SDLK_UP ||
                               sym == SDLK_DOWN);
        if (e.key.repeat && !is_arrow) {
            return LearnAction::None;
        }
        if (is_arrow && !accept_nav_key(e.key.repeat != 0)) {
            return LearnAction::None;
        }
        if (!e.key.repeat && sym == SDLK_ESCAPE) {
            mode_ = Mode::Browser;
            board_nav_ = false;
            clear_selection();
            return LearnAction::None;
        }
        if (!e.key.repeat && sym == SDLK_TAB) {
            if (keyboard_board_) {
                board_nav_ = !board_nav_;
                if (board_nav_) {
                    hand_nav_ = HandNavFocus::None;
                    if (asking_promote_) {
                        Square keep_sq;
                        Square promo_sq;
                        BoardView::promote_squares(pending_move_.from, pending_move_.to, keep_sq,
                                                   promo_sq);
                        board_cursor_ = keep_sq; // arrival square
                    } else {
                        clear_selection();
                        board_cursor_ = Square{5, 5};
                    }
                } else {
                    hand_nav_ = HandNavFocus::None;
                    side_focus_ = 0;
                }
            }
            return LearnAction::None;
        }
        if (keyboard_board_ && board_nav_) {
            if (sym == SDLK_LEFT) {
                move_board_cursor(1, 0);
                return LearnAction::None;
            }
            if (sym == SDLK_RIGHT) {
                move_board_cursor(-1, 0);
                return LearnAction::None;
            }
            if (sym == SDLK_UP) {
                move_board_cursor(0, -1);
                return LearnAction::None;
            }
            if (sym == SDLK_DOWN) {
                move_board_cursor(0, 1);
                return LearnAction::None;
            }
            if (!e.key.repeat && (sym == SDLK_RETURN || sym == SDLK_SPACE)) {
                activate_board_cursor();
                return LearnAction::None;
            }
            return LearnAction::None;
        }
        if (asking_promote_) {
            return LearnAction::None;
        }
        if (sym == SDLK_UP) {
            side_focus_ = (side_focus_ + 4) % 5;
            return LearnAction::None;
        }
        if (sym == SDLK_DOWN) {
            side_focus_ = (side_focus_ + 1) % 5;
            return LearnAction::None;
        }
        if (!e.key.repeat && (sym == SDLK_RETURN || sym == SDLK_SPACE)) {
            return activate_side_button(side_focus_);
        }
    }

    if (e.type == SDL_MOUSEMOTION) {
        drag_x_ = e.motion.x;
        drag_y_ = e.motion.y;
        if (lmb_down_ && (has_selection_ || has_drop_) && !asking_promote_) {
            int dx = drag_x_ - lmb_down_x_;
            int dy = drag_y_ - lmb_down_y_;
            if (dx * dx + dy * dy >= 36) {
                dragging_ = true;
            }
        } else if (!lmb_down_) {
            UiHit hit = view.hit_test(e.motion.x, e.motion.y);
            if (hit.kind == UiHitKind::ButtonResign) {
                side_focus_ = 0;
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonSave) {
                side_focus_ = 1;
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonLoad) {
                side_focus_ = 2;
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonNew) {
                side_focus_ = 3;
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonMenu) {
                side_focus_ = 4;
                board_nav_ = false;
            }
        }
        return LearnAction::None;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        UiHit hit = view.hit_test(e.button.x, e.button.y);
        if (hit.kind == UiHitKind::Board) {
            rmb_down_ = true;
            rmb_from_ = hit.square;
        }
        return LearnAction::None;
    }

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT) {
        if (rmb_down_) {
            UiHit hit = view.hit_test(e.button.x, e.button.y);
            if (hit.kind == UiHitKind::Board) {
                if (hit.square == rmb_from_) {
                    marks_.toggle_square(hit.square);
                } else {
                    marks_.toggle_arrow(rmb_from_, hit.square);
                }
            }
            rmb_down_ = false;
        }
        return LearnAction::None;
    }

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        end_lmb_drag(view, e.button.x, e.button.y);
        return LearnAction::None;
    }

    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT) {
        return LearnAction::None;
    }

    marks_.clear();
    drag_x_ = e.button.x;
    drag_y_ = e.button.y;
    lmb_down_x_ = e.button.x;
    lmb_down_y_ = e.button.y;

    UiHit hit = view.hit_test(e.button.x, e.button.y);

    // Remap side buttons: Resign->Retry, Save->Next, Load->Prev, New->Stages, Menu->Menu
    if (hit.kind == UiHitKind::ButtonResign) {
        board_nav_ = false;
        side_focus_ = 0;
        return activate_side_button(0);
    }
    if (hit.kind == UiHitKind::ButtonSave) {
        board_nav_ = false;
        side_focus_ = 1;
        return activate_side_button(1);
    }
    if (hit.kind == UiHitKind::ButtonLoad) {
        board_nav_ = false;
        side_focus_ = 2;
        return activate_side_button(2);
    }
    if (hit.kind == UiHitKind::ButtonNew) {
        board_nav_ = false;
        side_focus_ = 3;
        return activate_side_button(3);
    }
    if (hit.kind == UiHitKind::ButtonMenu) {
        board_nav_ = false;
        side_focus_ = 4;
        return activate_side_button(4);
    }

    if (asking_promote_) {
        PromotePick pick =
            view.hit_promote(e.button.x, e.button.y, pending_move_.from, pending_move_.to);
        if (pick == PromotePick::Promote) {
            pending_move_.promotion = PromotionChoice::Promote;
            asking_promote_ = false;
            commit_move(pending_move_);
        } else if (pick == PromotePick::Keep) {
            pending_move_.promotion = PromotionChoice::Decline;
            asking_promote_ = false;
            commit_move(pending_move_);
        }
        return LearnAction::None;
    }

    if (hit.kind == UiHitKind::Board) {
        if (keyboard_board_) {
            board_nav_ = true;
            board_cursor_ = hit.square;
        }
        if (session_.result() == learn::LevelResult::Playing) {
            PieceSlot const& slot = session_.board().at(hit.square);
            if (slot.occupied && slot.piece.color == session_.side_to_move()) {
                has_selection_ = true;
                selected_ = hit.square;
                has_drop_ = false;
                drag_piece_ = slot.piece;
                recompute_highlights();
                lmb_down_ = true;
                dragging_ = false;
            } else if (has_selection_ || has_drop_) {
                lmb_down_ = true;
                dragging_ = false;
            } else {
                clear_selection();
            }
        }
    } else if (hit.kind == UiHitKind::HandSente || hit.kind == UiHitKind::HandGote) {
        bool const gote_hand = hit.kind == UiHitKind::HandGote;
        if (gote_hand == (session_.side_to_move() == Color::Gote)) {
            on_hand_click(hit.hand_type);
        }
        lmb_down_ = has_drop_;
        dragging_ = false;
    }

    return LearnAction::None;
}

void LearnUi::draw_browser(SDL_Renderer* renderer, SpriteAssets& assets) const {
    SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
    SDL_RenderClear(renderer);

    auto draw_text = [&](int x, int y, std::string const& t, SDL_Color c, bool small) {
        TTF_Font* font = small ? assets.font_small() : assets.font();
        if (!font) {
            return;
        }
        SurfacePtr surf(TTF_RenderUTF8_Blended(font, t.c_str(), c));
        if (!surf) {
            return;
        }
        TexturePtr tex(SDL_CreateTextureFromSurface(renderer, surf.get()));
        if (!tex) {
            return;
        }
        SDL_Rect dst{x, y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex.get(), nullptr, &dst);
    };

    SDL_Color title{230, 220, 180, 255};
    SDL_Color text{210, 210, 210, 255};
    SDL_Color dim{140, 140, 140, 255};
    SDL_Color accent{220, 180, 60, 255};

    draw_text(40, 28, "Learn — Lishogi tutorial", title, false);
    draw_text(40, 62,
              "Levels taken from the open source lishogi.org source at "
              "https://github.com/WandererXII/lishogi",
              dim, true);

    auto const& stages = learn::all_stages();
    int const back_i = static_cast<int>(stages.size());
    for (int i = 0; i < static_cast<int>(stages.size()); ++i) {
        SDL_Rect r = stage_row_rect(i);
        bool sel = (i == browser_focus_);
        if (sel) {
            SDL_SetRenderDrawColor(renderer, 70, 90, 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 36, 40, 50, 255);
        }
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, sel ? 220 : 90, sel ? 180 : 90, sel ? 60 : 90, 255);
        SDL_RenderDrawRect(renderer, &r);
        if (sel) {
            SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
            SDL_RenderDrawRect(renderer, &r2);
        }

        char buf[320];
        std::snprintf(buf, sizeof(buf), "%2d. %s — %s  (%d levels)", i + 1,
                      stages[static_cast<std::size_t>(i)].title,
                      stages[static_cast<std::size_t>(i)].subtitle,
                      stages[static_cast<std::size_t>(i)].level_count);
        draw_text(r.x + 16, r.y + 8, buf, sel ? accent : text, true);

        int hs = progress_.stage_score(i);
        int mx = progress_.stage_max(i);
        char score_buf[64];
        std::snprintf(score_buf, sizeof(score_buf), "high score: %d/%d", hs, mx);
        SDL_Color score_col = (hs > 0) ? accent : dim;
        draw_text(kStageScoreX, r.y + 8, score_buf, score_col, true);
    }

    SDL_Rect back = browser_back_rect();
    bool back_lit = (browser_focus_ == back_i);
    if (back_lit) {
        SDL_SetRenderDrawColor(renderer, 70, 90, 50, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 50, 70, 60, 255);
    }
    SDL_RenderFillRect(renderer, &back);
    SDL_SetRenderDrawColor(renderer, back_lit ? 220 : 180, back_lit ? 180 : 200,
                           back_lit ? 60 : 180, 255);
    SDL_RenderDrawRect(renderer, &back);
    if (back_lit) {
        SDL_Rect r2{back.x + 1, back.y + 1, back.w - 2, back.h - 2};
        SDL_RenderDrawRect(renderer, &r2);
    }
    draw_text(back.x + 40, back.y + 12, "Back", back_lit ? accent : text, false);

    SDL_RenderPresent(renderer);
}

void LearnUi::draw_play(SDL_Renderer* renderer, BoardView const& view, SpriteAssets& assets) const {
    (void)assets;
    // Reuse BoardView layout by temporarily drawing through a fake Game-like path.
    // Implement parallel draw here for learn-specific overlays.

    SDL_SetRenderDrawColor(renderer, 28, 32, 40, 255);
    SDL_RenderClear(renderer);

    // Delegate core board drawing via hit-compatible geometry: call view helpers by
    // constructing a lightweight status draw — BoardView::draw needs Game.
    // So we duplicate the essential paint using public hit_test geometry + a small bridge.

    // Use a static helper Game? Better: add BoardView::draw_board_state — done via friendship-free
    // approach: copy draw calls through a local lambda using view's public API only.
    // BoardView doesn't expose draw_board_grid. Extend view with draw_learn.

    Square const* promo_at = nullptr;
    Square const* promo_from = nullptr;
    Piece promo_piece;
    Piece const* promo_ptr = nullptr;
    if (asking_promote_ && !pending_move_.is_drop) {
        promo_at = &pending_move_.to;
        promo_from = &pending_move_.from;
        if (session_.board().at(pending_move_.from).occupied) {
            promo_piece = session_.board().at(pending_move_.from).piece;
            promo_ptr = &promo_piece;
        }
    }
    DragPaint drag;
    if (dragging_ && (has_selection_ || has_drop_) && !asking_promote_) {
        drag.active = true;
        drag.piece = drag_piece_;
        drag.x = drag_x_;
        drag.y = drag_y_;
    }
    bool on_board = keyboard_board_ && board_nav_ && hand_nav_ == HandNavFocus::None;
    Square const* cursor = on_board ? &board_cursor_ : nullptr;
    int side_draw = (keyboard_board_ && board_nav_) ? -1 : side_focus_;
    bool cursor_green = asking_promote_ && board_nav_;
    HandNavFocus hand_draw =
        (keyboard_board_ && board_nav_) ? hand_nav_ : HandNavFocus::None;
    view.draw_learn(renderer, session_, has_selection_ ? &selected_ : nullptr, highlights_,
                    has_drop_ ? &drop_type_ : nullptr, promo_at, promo_from, promo_ptr, &marks_,
                    &drag, side_draw, cursor, cursor_green, hand_draw, hand_slot_);
}

void LearnUi::draw(SDL_Renderer* renderer, BoardView const& view, SpriteAssets& assets) const {
    if (mode_ == Mode::Browser) {
        draw_browser(renderer, assets);
    } else {
        draw_play(renderer, view, assets);
    }
}

} // namespace ui
} // namespace shogi
