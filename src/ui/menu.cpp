#include "menu.hpp"
#include "key_repeat.hpp"
#include "view.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace shogi {
namespace ui {
namespace {

SDL_Keycode const kKonami[] = {
    SDLK_UP, SDLK_UP, SDLK_DOWN, SDLK_DOWN,
    SDLK_LEFT, SDLK_RIGHT, SDLK_LEFT, SDLK_RIGHT,
    SDLK_b, SDLK_a
};
int const kKonamiLen = static_cast<int>(sizeof(kKonami) / sizeof(kKonami[0]));

} // namespace

PieceSetMenu::PieceSetMenu()
    : selected_(0),
      style_focus_(0),
      nav_focus_(NavFocus::Style),
      bottom_focus_(0),
      koma_unlocked_(false),
      engine_session_unlocked_(false),
      keyboard_board_unlocked_(false),
      keyboard_board_(false),
      hard_mode_konami_unlocked_(false),
      hard_mode_(false),
      use_clock_(false),
      use_handicap_(false),
      unlock_flash_frames_(0),
      engine_flash_frames_(0),
      session_active_(false),
      allow_review_fork_(false),
      engine_picker_open_(false),
      engine_picker_focus_(0),
      chosen_engine_(EngineKind::FairyStockfish),
      setup_open_(false),
      setup_for_engine_(false),
      setup_focus_(SetupFocus::Start),
      setup_(),
      setup_dragging_(false) {}

void PieceSetMenu::rebuild_visible() {
    // Keep full ordered list; locked Koma stays as a blank slot (not a button).
    sets_ = all_sets_;
}

bool PieceSetMenu::is_selectable(int index) const {
    if (index < 0 || index >= static_cast<int>(sets_.size())) {
        return false;
    }
    PieceSetInfo const& info = sets_[static_cast<std::size_t>(index)];
    if (info.secret && !koma_unlocked_) {
        return false;
    }
    if (info.learn_gated && !blank_unlocked()) {
        return false;
    }
    return true;
}

bool PieceSetMenu::is_focusable(int index) const {
    if (index < 0 || index >= static_cast<int>(sets_.size())) {
        return false;
    }
    PieceSetInfo const& info = sets_[static_cast<std::size_t>(index)];
    // Hidden Koma spacer is not a focus target; locked Blank is (same as other locks).
    if (info.secret && !koma_unlocked_) {
        return false;
    }
    return true;
}

int PieceSetMenu::first_selectable() const {
    for (int i = 0; i < static_cast<int>(sets_.size()); ++i) {
        if (is_selectable(i)) {
            return i;
        }
    }
    return 0;
}

int PieceSetMenu::last_selectable() const {
    for (int i = static_cast<int>(sets_.size()) - 1; i >= 0; --i) {
        if (is_selectable(i)) {
            return i;
        }
    }
    return 0;
}

int PieceSetMenu::first_focusable() const {
    for (int i = 0; i < static_cast<int>(sets_.size()); ++i) {
        if (is_focusable(i)) {
            return i;
        }
    }
    return 0;
}

int PieceSetMenu::last_focusable() const {
    for (int i = static_cast<int>(sets_.size()) - 1; i >= 0; --i) {
        if (is_focusable(i)) {
            return i;
        }
    }
    return 0;
}

int PieceSetMenu::next_focusable(int from, int dir) const {
    int n = static_cast<int>(sets_.size());
    int i = from;
    for (int step = 0; step < n; ++step) {
        i += dir;
        if (i < 0 || i >= n) {
            return -1;
        }
        if (is_focusable(i)) {
            return i;
        }
    }
    return -1;
}

int PieceSetMenu::next_selectable(int from, int dir) const {
    int n = static_cast<int>(sets_.size());
    int i = from;
    for (int step = 0; step < n; ++step) {
        i += dir;
        if (i < 0 || i >= n) {
            return -1; // left the style list
        }
        if (is_selectable(i)) {
            return i;
        }
    }
    return -1;
}

bool PieceSetMenu::init(SDL_Renderer* renderer, SpriteAssets& assets, std::string const& initial_path) {
    all_sets_ = SpriteAssets::discover_piece_sets();
    rebuild_visible();
    refresh_learn_progress();
    if (sets_.empty()) {
        return false;
    }

    selected_ = first_selectable();
    for (std::size_t i = 0; i < sets_.size(); ++i) {
        if (sets_[i].path == initial_path && is_selectable(static_cast<int>(i))) {
            selected_ = static_cast<int>(i);
            break;
        }
    }
    style_focus_ = selected_;
    return select_index(selected_, assets, renderer);
}

void PieceSetMenu::refresh_learn_progress() {
    learn_progress_.load();
}

bool PieceSetMenu::engine_unlocked() const {
    return engine_session_unlocked_ || learn_progress_.all_perfect();
}

bool PieceSetMenu::blank_unlocked() const {
    return learn_progress_.all_cleared();
}

bool PieceSetMenu::hard_mode_available() const {
    if (hard_mode_konami_unlocked_) {
        return true;
    }
    return learn_progress_.all_cleared() && learn_progress_.average_best() >= 2.0;
}

std::string const& PieceSetMenu::selected_path() const {
    return sets_[static_cast<std::size_t>(selected_)].path;
}

namespace {
int const kStyleListX = 40;
int const kStyleListW = 320;
} // namespace

SDL_Rect PieceSetMenu::list_item_rect(int index) const {
    return SDL_Rect{kStyleListX, 110 + index * 52, kStyleListW, 46};
}

SDL_Rect PieceSetMenu::option_row_rect(int index) const {
    // 0=clock, 1=handicap, 2=takebacks, 3=hard, 4=keyboard
    int const row_h = 48;
    int const gap = 6;
    int const bottom_y = BoardView::window_h - 70;
    int const rows = 5;
    int y = bottom_y - 12 - (rows - index) * (row_h + gap);
    return SDL_Rect{kStyleListX, y, kStyleListW, row_h};
}

SDL_Rect PieceSetMenu::option_checkbox_rect(int index) const {
    SDL_Rect row = option_row_rect(index);
    int const sz = 18;
    return SDL_Rect{row.x + 12, row.y + 6, sz, sz};
}

int PieceSetMenu::measure_text(SpriteAssets const& assets, char const* text, TextSize size) {
    TTF_Font* font = assets.font();
    if (size == TextSize::Small) {
        font = assets.font_small();
    } else if (size == TextSize::Tiny) {
        font = assets.font_tiny();
    }
    if (!font || !text) {
        return static_cast<int>(std::strlen(text ? text : "")) * 10;
    }
    int w = 0;
    int h = 0;
    if (TTF_SizeUTF8(font, text, &w, &h) != 0) {
        return static_cast<int>(std::strlen(text)) * 10;
    }
    return w;
}

int PieceSetMenu::measure_text_h(SpriteAssets const& assets, TextSize size) {
    TTF_Font* font = assets.font();
    if (size == TextSize::Small) {
        font = assets.font_small();
    } else if (size == TextSize::Tiny) {
        font = assets.font_tiny();
    }
    if (!font) {
        return 20;
    }
    int w = 0;
    int h = 0;
    if (TTF_SizeUTF8(font, "Ag", &w, &h) != 0) {
        return 20;
    }
    return h;
}

int PieceSetMenu::layout_bottom_buttons(SpriteAssets const& assets, BottomBtn* out,
                                       int max_out) const {
    struct Spec {
        BottomId id;
        char const* label;
        bool primary;
        bool enabled;
        bool include;
    };

    Spec specs[] = {
        {BottomId::Continue, "Continue", true, true, session_active_},
        {BottomId::NewGame, "New game", true, true, true},
        {BottomId::Learn, "Tutorial", true, true, true},
        {BottomId::Engine, "Play against engine", true, engine_unlocked(), true},
        {BottomId::Quit, "Quit", false, true, true},
    };

    int const pad_x = 22;
    int const gap = 14;
    int const btn_h = 48;
    int const y = BoardView::window_h - 70;
    int x = 40;
    int n = 0;

    for (std::size_t i = 0; i < sizeof(specs) / sizeof(specs[0]); ++i) {
        if (!specs[i].include || n >= max_out) {
            continue;
        }
        int tw = measure_text(assets, specs[i].label, TextSize::Normal);
        int bw = tw + pad_x * 2;
        if (bw < 96) {
            bw = 96;
        }
        out[n].id = specs[i].id;
        out[n].label = specs[i].label;
        out[n].primary = specs[i].primary;
        out[n].enabled = specs[i].enabled;
        out[n].rect = SDL_Rect{x, y, bw, btn_h};
        x += bw + gap;
        ++n;
    }
    return n;
}

void PieceSetMenu::draw_text(SDL_Renderer* renderer, SpriteAssets const& assets, int x, int y,
                             std::string const& text, SDL_Color color, TextSize size) const {
    TTF_Font* font = assets.font();
    if (size == TextSize::Small) {
        font = assets.font_small();
    } else if (size == TextSize::Tiny) {
        font = assets.font_tiny();
    }
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

bool PieceSetMenu::select_index(int index, SpriteAssets& assets, SDL_Renderer* renderer) {
    if (!is_selectable(index)) {
        return false;
    }
    if (!assets.load_piece_set(renderer, sets_[static_cast<std::size_t>(index)].path)) {
        return false;
    }
    selected_ = index;
    return true;
}

void PieceSetMenu::draw_option_row(SDL_Renderer* renderer, SpriteAssets const& assets, int index,
                                   bool focused, bool checked, bool locked, char const* label,
                                   char const* subtitle, SDL_Color accent, SDL_Color text,
                                   SDL_Color dim) const {
    // Locked + focused uses the same chrome as Engine/Blank locks (gold ring, muted fill).
    SDL_Rect fr = option_row_rect(index);
    if (focused) {
        SDL_SetRenderDrawColor(renderer, locked ? 48 : 70, locked ? 52 : 90, locked ? 40 : 50, 255);
    } else if (locked) {
        SDL_SetRenderDrawColor(renderer, 20, 22, 28, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 36, 40, 50, 255);
    }
    SDL_RenderFillRect(renderer, &fr);
    if (focused) {
        SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
    } else if (locked) {
        SDL_SetRenderDrawColor(renderer, 45, 45, 50, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
    }
    SDL_RenderDrawRect(renderer, &fr);
    if (focused) {
        SDL_Rect fr2{fr.x + 1, fr.y + 1, fr.w - 2, fr.h - 2};
        SDL_RenderDrawRect(renderer, &fr2);
    }

    SDL_Rect cb = option_checkbox_rect(index);
    SDL_SetRenderDrawColor(renderer, 28, 32, 40, 255);
    SDL_RenderFillRect(renderer, &cb);
    SDL_SetRenderDrawColor(renderer, focused ? 220 : (locked ? 70 : 180),
                           focused ? 180 : (locked ? 70 : 180), focused ? 60 : (locked ? 75 : 180),
                           255);
    SDL_RenderDrawRect(renderer, &cb);
    if (checked && !locked) {
        SDL_Rect mark{cb.x + 4, cb.y + 4, cb.w - 8, cb.h - 8};
        SDL_SetRenderDrawColor(renderer, 80, 180, 100, 255);
        SDL_RenderFillRect(renderer, &mark);
    }

    SDL_Color locked_col{70, 70, 75, 255};
    SDL_Color label_col = locked ? (focused ? SDL_Color{140, 140, 120, 255} : locked_col)
                                 : ((checked || focused) ? accent : text);
    int const label_h = measure_text_h(assets, TextSize::Small);
    draw_text(renderer, assets, cb.x + cb.w + 12, cb.y + (cb.h - label_h) / 2, label, label_col,
              TextSize::Small);
    int sub_w = measure_text(assets, subtitle, TextSize::Tiny);
    int const sub_y = cb.y + cb.h + 6; // parenthetical hint under the compact checkbox
    draw_text(renderer, assets, fr.x + (fr.w - sub_w) / 2, sub_y, subtitle,
              locked ? (focused ? SDL_Color{90, 90, 85, 255} : SDL_Color{55, 55, 60, 255}) : dim,
              TextSize::Tiny);
}

bool PieceSetMenu::feed_konami(SDL_Keycode key) {
    // Sliding window: unlock if the last N keys equal the code.
    // Handles overlaps like ↑↑↑↓↓… (extra ups before a valid suffix).
    konami_history_.push_back(key);
    if (static_cast<int>(konami_history_.size()) > kKonamiLen) {
        konami_history_.erase(konami_history_.begin());
    }
    if (static_cast<int>(konami_history_.size()) < kKonamiLen) {
        return false;
    }

    bool match = true;
    for (int i = 0; i < kKonamiLen; ++i) {
        if (konami_history_[static_cast<std::size_t>(i)] != kKonami[i]) {
            match = false;
            break;
        }
    }
    if (!match) {
        return false;
    }

    konami_history_.clear();
    bool changed = false;
    if (!koma_unlocked_) {
        koma_unlocked_ = true;
        rebuild_visible();
        unlock_flash_frames_ = 180;
        changed = true;
    }
    // Engine unlock via Konami is session-only (not written to disk).
    if (!engine_session_unlocked_) {
        engine_session_unlocked_ = true;
        changed = true;
    }
    if (!keyboard_board_unlocked_) {
        keyboard_board_unlocked_ = true;
        changed = true;
    }
    if (!hard_mode_konami_unlocked_) {
        hard_mode_konami_unlocked_ = true;
        changed = true;
    }
    return changed;
}

MenuAction PieceSetMenu::activate_bottom(BottomBtn const& b) {
    switch (b.id) {
    case BottomId::Continue:
        return MenuAction::ContinueGame;
    case BottomId::NewGame:
        if (use_clock_ || use_handicap_) {
            open_setup(false);
            return MenuAction::None;
        }
        setup_ = GameSetupConfig();
        setup_.use_clock = false;
        setup_.use_handicap = false;
        return MenuAction::NewGame;
    case BottomId::Learn:
        return MenuAction::Learn;
    case BottomId::Engine:
        if (!b.enabled) {
            return MenuAction::None;
        }
        if (use_clock_ || use_handicap_) {
            open_setup(true);
            return MenuAction::None;
        }
        open_engine_picker();
        return MenuAction::None;
    case BottomId::Quit:
        return MenuAction::Quit;
    }
    return MenuAction::None;
}

MenuAction PieceSetMenu::activate_focused(SpriteAssets& assets, SDL_Renderer* renderer) {
    if (nav_focus_ == NavFocus::Style) {
        select_index(style_focus_, assets, renderer);
        return MenuAction::None;
    }
    if (nav_focus_ == NavFocus::UseClock) {
        use_clock_ = !use_clock_;
        return MenuAction::None;
    }
    if (nav_focus_ == NavFocus::UseHandicap) {
        use_handicap_ = !use_handicap_;
        return MenuAction::None;
    }
    if (nav_focus_ == NavFocus::Takebacks) {
        allow_review_fork_ = !allow_review_fork_;
        return MenuAction::None;
    }
    if (nav_focus_ == NavFocus::KeyboardOnly) {
        if (keyboard_board_unlocked_) {
            keyboard_board_ = !keyboard_board_;
        }
        return MenuAction::None;
    }
    if (nav_focus_ == NavFocus::HardMode) {
        if (hard_mode_available()) {
            hard_mode_ = !hard_mode_;
        }
        return MenuAction::None;
    }
    BottomBtn bottoms[8];
    int bn = layout_bottom_buttons(assets, bottoms, 8);
    if (bottom_focus_ >= 0 && bottom_focus_ < bn) {
        return activate_bottom(bottoms[bottom_focus_]);
    }
    return MenuAction::None;
}

void PieceSetMenu::nav_vertical(int dir) {
    // dir: +1 = down, -1 = up — moves highlight only; piece set changes on confirm.
    if (nav_focus_ == NavFocus::Style) {
        int next = next_focusable(style_focus_, dir);
        if (next >= 0) {
            style_focus_ = next;
            return;
        }
        if (dir > 0) {
            nav_focus_ = NavFocus::UseClock;
        } else {
            nav_focus_ = NavFocus::Bottom;
            bottom_focus_ = 0;
        }
        return;
    }

    NavFocus const order[] = {NavFocus::UseClock,   NavFocus::UseHandicap, NavFocus::Takebacks,
                              NavFocus::HardMode,   NavFocus::KeyboardOnly};
    int const n = 5;
    int idx = -1;
    for (int i = 0; i < n; ++i) {
        if (nav_focus_ == order[i]) {
            idx = i;
            break;
        }
    }
    if (idx >= 0) {
        int next = idx + dir;
        if (next < 0) {
            nav_focus_ = NavFocus::Style;
            style_focus_ = last_focusable();
        } else if (next >= n) {
            nav_focus_ = NavFocus::Bottom;
            bottom_focus_ = 0;
        } else {
            nav_focus_ = order[next];
        }
        return;
    }

    // Bottom row
    if (dir > 0) {
        nav_focus_ = NavFocus::Style;
        style_focus_ = first_focusable();
    } else {
        nav_focus_ = NavFocus::KeyboardOnly;
    }
}

void PieceSetMenu::nav_horizontal(int dir, SpriteAssets const& assets) {
    if (nav_focus_ != NavFocus::Bottom) {
        return;
    }
    BottomBtn bottoms[8];
    int bn = layout_bottom_buttons(assets, bottoms, 8);
    if (bn <= 0) {
        return;
    }
    bottom_focus_ = (bottom_focus_ + dir) % bn;
    if (bottom_focus_ < 0) {
        bottom_focus_ += bn;
    }
}

MenuAction PieceSetMenu::handle_event(SDL_Event const& e, SpriteAssets& assets,
                                      SDL_Renderer* renderer) {
    if (e.type == SDL_QUIT) {
        return MenuAction::Quit;
    }

    if (setup_open_) {
        return handle_setup_event(e);
    }
    if (engine_picker_open_) {
        return handle_engine_picker_event(e);
    }

    if (keyboard_board()) {
        if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEWHEEL ||
            e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            return MenuAction::None;
        }
    }

    auto in_rect = [](SDL_Rect r, int x, int y) {
        return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
    };

    if (e.type == SDL_MOUSEMOTION) {
        // Hover moves the same highlight as arrow keys (does not confirm a piece set).
        int mx = e.motion.x;
        int my = e.motion.y;
        for (int i = 0; i < static_cast<int>(sets_.size()); ++i) {
            if (!is_focusable(i)) {
                continue;
            }
            if (in_rect(list_item_rect(i), mx, my)) {
                nav_focus_ = NavFocus::Style;
                style_focus_ = i;
                return MenuAction::None;
            }
        }
        if (in_rect(option_row_rect(0), mx, my)) {
            nav_focus_ = NavFocus::UseClock;
            return MenuAction::None;
        }
        if (in_rect(option_row_rect(1), mx, my)) {
            nav_focus_ = NavFocus::UseHandicap;
            return MenuAction::None;
        }
        if (in_rect(option_row_rect(2), mx, my)) {
            nav_focus_ = NavFocus::Takebacks;
            return MenuAction::None;
        }
        if (in_rect(option_row_rect(3), mx, my)) {
            nav_focus_ = NavFocus::HardMode;
            return MenuAction::None;
        }
        if (in_rect(option_row_rect(4), mx, my)) {
            nav_focus_ = NavFocus::KeyboardOnly;
            return MenuAction::None;
        }
        BottomBtn bottoms[8];
        int bn = layout_bottom_buttons(assets, bottoms, 8);
        for (int i = 0; i < bn; ++i) {
            // Hover/focus locked Engine the same way as other locked rows.
            if (in_rect(bottoms[i].rect, mx, my)) {
                nav_focus_ = NavFocus::Bottom;
                bottom_focus_ = i;
                break;
            }
        }
        return MenuAction::None;
    }

    if (e.type == SDL_KEYDOWN) {
        SDL_Keycode sym = e.key.keysym.sym;
        bool const is_arrow = (sym == SDLK_LEFT || sym == SDLK_RIGHT || sym == SDLK_UP ||
                               sym == SDLK_DOWN);
        if (e.key.repeat && !is_arrow) {
            return MenuAction::None;
        }
        if (is_arrow && !accept_nav_key(e.key.repeat != 0)) {
            return MenuAction::None;
        }

        // Konami only on the initial keydown (not auto-repeat).
        if (!e.key.repeat) {
            bool just_unlocked = feed_konami(sym);
            if (just_unlocked) {
                for (std::size_t i = 0; i < sets_.size(); ++i) {
                    if (sets_[i].secret) {
                        style_focus_ = static_cast<int>(i);
                        select_index(style_focus_, assets, renderer);
                        nav_focus_ = NavFocus::Style;
                        break;
                    }
                }
                return MenuAction::None;
            }
        }

        // Outside Konami: arrows (incl. hold-repeat) + Enter/Space.
        if (sym == SDLK_UP) {
            nav_vertical(-1);
        } else if (sym == SDLK_DOWN) {
            nav_vertical(1);
        } else if (sym == SDLK_LEFT) {
            nav_horizontal(-1, assets);
        } else if (sym == SDLK_RIGHT) {
            nav_horizontal(1, assets);
        } else if (!e.key.repeat && (sym == SDLK_RETURN || sym == SDLK_SPACE)) {
            return activate_focused(assets, renderer);
        }
        return MenuAction::None;
    }

    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT) {
        return MenuAction::None;
    }

    int x = e.button.x;
    int y = e.button.y;

    for (int i = 0; i < static_cast<int>(sets_.size()); ++i) {
        if (!is_focusable(i)) {
            continue;
        }
        if (in_rect(list_item_rect(i), x, y)) {
            style_focus_ = i;
            nav_focus_ = NavFocus::Style;
            if (is_selectable(i)) {
                select_index(i, assets, renderer);
            }
            return MenuAction::None;
        }
    }

    if (in_rect(option_row_rect(0), x, y)) {
        use_clock_ = !use_clock_;
        nav_focus_ = NavFocus::UseClock;
        return MenuAction::None;
    }
    if (in_rect(option_row_rect(1), x, y)) {
        use_handicap_ = !use_handicap_;
        nav_focus_ = NavFocus::UseHandicap;
        return MenuAction::None;
    }
    if (in_rect(option_row_rect(2), x, y)) {
        allow_review_fork_ = !allow_review_fork_;
        nav_focus_ = NavFocus::Takebacks;
        return MenuAction::None;
    }
    if (in_rect(option_row_rect(3), x, y)) {
        nav_focus_ = NavFocus::HardMode;
        if (hard_mode_available()) {
            hard_mode_ = !hard_mode_;
        }
        return MenuAction::None;
    }
    if (in_rect(option_row_rect(4), x, y)) {
        nav_focus_ = NavFocus::KeyboardOnly;
        if (keyboard_board_unlocked_) {
            keyboard_board_ = !keyboard_board_;
        }
        return MenuAction::None;
    }

    BottomBtn bottoms[8];
    int bn = layout_bottom_buttons(assets, bottoms, 8);
    for (int i = 0; i < bn; ++i) {
        if (!in_rect(bottoms[i].rect, x, y)) {
            continue;
        }
        nav_focus_ = NavFocus::Bottom;
        bottom_focus_ = i;
        return activate_bottom(bottoms[i]);
    }
    return MenuAction::None;
}

void PieceSetMenu::draw(SDL_Renderer* renderer, SpriteAssets const& assets) const {
    if (unlock_flash_frames_ > 0) {
        --unlock_flash_frames_;
    }

    SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
    SDL_RenderClear(renderer);

    SDL_Color title{230, 220, 180, 255};
    SDL_Color text{210, 210, 210, 255};
    SDL_Color dim{140, 140, 140, 255};
    SDL_Color accent{220, 180, 60, 255};

    draw_text(renderer, assets, 40, 28,
              session_active_ ? "Shogi — menu" : "Shogi — piece set", title, TextSize::Normal);
    if (session_active_) {
        draw_text(renderer, assets, 40, 62,
                  "Continue the game, start a new one, or change piece set.", dim,
                  TextSize::Small);
    }

    for (int i = 0; i < static_cast<int>(sets_.size()); ++i) {
        SDL_Rect r = list_item_rect(i);
        PieceSetInfo const& info = sets_[static_cast<std::size_t>(i)];
        bool secret_locked = info.secret && !koma_unlocked_;
        bool learn_locked = info.learn_gated && !blank_unlocked();

        if (secret_locked) {
            // Blank spacer: same footprint as a row, no border/label — only background.
            // Slightly different shade so the gap feels intentional but unexplained.
            SDL_SetRenderDrawColor(renderer, 28, 32, 40, 255);
            SDL_RenderFillRect(renderer, &r);
            continue;
        }

        bool sel = (i == selected_);
        bool focused = (nav_focus_ == NavFocus::Style && i == style_focus_);
        bool secret = info.secret;

        if (learn_locked) {
            // Same locked-focus chrome as Keyboard only / Engine.
            if (focused) {
                SDL_SetRenderDrawColor(renderer, 48, 52, 40, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 20, 22, 28, 255);
            }
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, focused ? 220 : 45, focused ? 180 : 45,
                                   focused ? 60 : 50, 255);
            SDL_RenderDrawRect(renderer, &r);
            if (focused) {
                SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
                SDL_RenderDrawRect(renderer, &r2);
            }
            draw_text(renderer, assets, r.x + 8, r.y + 14,
                      "Blank (unlocked by completing the tutorial)",
                      focused ? SDL_Color{140, 140, 120, 255} : SDL_Color{70, 70, 75, 255},
                      TextSize::Tiny);
            continue;
        }

        if (focused) {
            SDL_SetRenderDrawColor(renderer, secret ? 90 : 70, secret ? 60 : 90, secret ? 110 : 50,
                                   255);
        } else if (sel) {
            SDL_SetRenderDrawColor(renderer, 48, 52, 62, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 36, 40, 50, 255);
        }
        SDL_RenderFillRect(renderer, &r);
        if (focused) {
            SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
        } else if (sel) {
            SDL_SetRenderDrawColor(renderer, 120, 120, 100, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
        }
        SDL_RenderDrawRect(renderer, &r);
        if (focused) {
            SDL_Rect r2{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
            SDL_RenderDrawRect(renderer, &r2);
        }

        draw_text(renderer, assets, r.x + 12, r.y + 12, info.label,
                  (focused || sel) ? accent : text, TextSize::Small);
    }

    if (unlock_flash_frames_ > 0 && !sets_.empty()) {
        SDL_Rect last = list_item_rect(static_cast<int>(sets_.size()) - 1);
        draw_text(renderer, assets, last.x, last.y + last.h + 8, "Koma unlocked!",
                  SDL_Color{255, 120, 220, 255}, TextSize::Normal);
    }

    int const panel_x = 400;
    int const panel_y = 100;
    int const panel_w = BoardView::window_w - panel_x - 40;
    int const panel_h = BoardView::window_h - panel_y - 100;
    SDL_Rect panel{panel_x, panel_y, panel_w, panel_h};
    SDL_SetRenderDrawColor(renderer, 36, 40, 50, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
    SDL_RenderDrawRect(renderer, &panel);

    draw_text(renderer, assets, panel.x + 20, panel.y + 14, "Preview: " + assets.current_label(),
              accent, TextSize::Normal);

    char const* row_labels[4] = {
        "1 — Sente unpromoted",
        "2 — Sente promoted",
        "3 — Gote unpromoted",
        "4 — Gote promoted",
    };

    int preview_x = panel.x + 20;
    int const label_h = 20;
    int const between_gap = 8;
    int piece_h = std::min(90, assets.cell_h() > 0 ? assets.cell_h() : 90);
    int max_row_w = panel.w - 40;
    if (assets.cell_w() > 0 && assets.cell_h() > 0) {
        int native_w = assets.cell_w();
        int native_h = assets.cell_h();
        int row_w = 8 * native_w + 7 * 6;
        if (row_w > max_row_w) {
            float s = static_cast<float>(max_row_w) / row_w;
            piece_h = std::max(48, static_cast<int>(native_h * s));
        } else {
            piece_h = native_h;
        }
    }

    // Each block: label, then pieces. Gaps keep labels clear of the row above.
    int preview_y = panel.y + 48;
    int row_gap = label_h + between_gap; // after pieces until next pieces (= label + gap)
    int pieces_origin = preview_y + label_h;
    for (int row = 0; row < 4; ++row) {
        int label_y = preview_y + row * (piece_h + row_gap);
        draw_text(renderer, assets, preview_x, label_y, row_labels[row], dim, TextSize::Small);
    }
    int used_h = assets.draw_preview(renderer, preview_x, pieces_origin, piece_h, row_gap);
    int preview_block_h = label_h + used_h;

    if (assets.sheet()) {
        int tw = 0;
        int th = 0;
        SDL_QueryTexture(assets.sheet(), nullptr, nullptr, &tw, &th);
        int top = preview_y + preview_block_h + 28;
        int max_w = panel.w - 40;
        int max_h = panel.y + panel.h - top - 20;
        if (max_h > 40) {
            float scale = std::min(1.0f, std::min(static_cast<float>(max_w) / tw,
                                                    static_cast<float>(max_h) / th));
            int dw = static_cast<int>(tw * scale);
            int dh = static_cast<int>(th * scale);
            SDL_Rect dst{panel.x + 20, top, dw, dh};
            draw_text(renderer, assets, dst.x, dst.y - 22, "Full sheet", dim, TextSize::Small);
            SDL_RenderCopy(renderer, assets.sheet(), nullptr, &dst);
        }
    }

    draw_option_row(renderer, assets, 0, nav_focus_ == NavFocus::UseClock, use_clock_, false,
                    "Use chess clock", "(Main time + increment on new games)", accent, text, dim);
    draw_option_row(renderer, assets, 1, nav_focus_ == NavFocus::UseHandicap, use_handicap_, false,
                    "Use handicap", "(Komaochi: stronger side + rank difference)", accent, text,
                    dim);
    draw_option_row(renderer, assets, 2, nav_focus_ == NavFocus::Takebacks, allow_review_fork_,
                    false, "Allow takebacks", "(Allow continuation from earlier boardstate)",
                    accent, text, dim);
    bool hard_unlocked = hard_mode_available();
    draw_option_row(renderer, assets, 3, nav_focus_ == NavFocus::HardMode, hard_mode_ && hard_unlocked,
                    !hard_unlocked, "Hard mode",
                    hard_unlocked ? "(professional standards)"
                                  : "(achieve an average of 2/3 in the tutorial)",
                    accent, text, dim);
    draw_option_row(renderer, assets, 4, nav_focus_ == NavFocus::KeyboardOnly, keyboard_board_,
                    !keyboard_board_unlocked_, "Keyboard only",
                    keyboard_board_unlocked_
                        ? "(Tab board, arrows, Enter/Space — mouse off)"
                        : "(Unlock with secret code)",
                    accent, text, dim);

    int const btn_label_h = measure_text_h(assets, TextSize::Normal);
    BottomBtn bottoms[8];
    int bn = layout_bottom_buttons(assets, bottoms, 8);
    int focus_i = bottom_focus_;
    if (focus_i >= bn) {
        focus_i = std::max(0, bn - 1);
    }
    for (int i = 0; i < bn; ++i) {
        BottomBtn const& b = bottoms[i];
        bool focused = (nav_focus_ == NavFocus::Bottom && i == focus_i);
        bool locked = !b.enabled;
        if (focused) {
            SDL_SetRenderDrawColor(renderer, locked ? 48 : (b.primary ? 70 : 60),
                                   locked ? 52 : (b.primary ? 110 : 90),
                                   locked ? 40 : (b.primary ? 50 : 60), 255);
        } else if (locked) {
            SDL_SetRenderDrawColor(renderer, 20, 22, 28, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, b.primary ? 60 : 50, b.primary ? 100 : 70,
                                   b.primary ? 70 : 60, 255);
        }
        SDL_RenderFillRect(renderer, &b.rect);
        if (focused) {
            SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
        } else if (locked) {
            SDL_SetRenderDrawColor(renderer, 45, 45, 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 180, 200, 180, 255);
        }
        SDL_RenderDrawRect(renderer, &b.rect);
        if (focused) {
            SDL_Rect r2{b.rect.x + 1, b.rect.y + 1, b.rect.w - 2, b.rect.h - 2};
            SDL_RenderDrawRect(renderer, &r2);
        }

        int tw = measure_text(assets, b.label, TextSize::Normal);
        int tx = b.rect.x + (b.rect.w - tw) / 2;
        int ty = b.rect.y + (b.rect.h - btn_label_h) / 2;
        SDL_Color label_col =
            locked ? (focused ? SDL_Color{140, 140, 120, 255} : SDL_Color{70, 70, 75, 255})
                   : (focused ? accent : text);
        draw_text(renderer, assets, tx, ty, b.label, label_col, TextSize::Normal);
    }

    if (engine_flash_frames_ > 0) {
        --engine_flash_frames_;
        SDL_Rect quit_r{0, 0, 0, 0};
        for (int i = 0; i < bn; ++i) {
            if (bottoms[i].id == BottomId::Quit) {
                quit_r = bottoms[i].rect;
                break;
            }
        }
        if (quit_r.w > 0) {
            int const flash_h = measure_text_h(assets, TextSize::Small);
            draw_text(renderer, assets, quit_r.x + quit_r.w + 16,
                      quit_r.y + (quit_r.h - flash_h) / 2, "placeholder…",
                      SDL_Color{180, 200, 255, 255}, TextSize::Small);
        }
    }

    if (engine_picker_open_) {
        draw_engine_picker(renderer, assets);
    }
    if (setup_open_) {
        draw_setup(renderer, assets);
    }

    SDL_RenderPresent(renderer);
}

void PieceSetMenu::open_engine_picker() {
    engine_picker_open_ = true;
    engine_picker_focus_ = 0;
}

void PieceSetMenu::close_engine_picker() {
    engine_picker_open_ = false;
}

MenuAction PieceSetMenu::confirm_engine_picker() {
    if (engine_picker_focus_ == 0) {
        chosen_engine_ = EngineKind::FairyStockfish;
    } else if (engine_picker_focus_ == 1) {
        chosen_engine_ = EngineKind::RandomLegal;
    } else {
        close_engine_picker();
        return MenuAction::None;
    }
    close_engine_picker();
    return MenuAction::PlayEngine;
}

void PieceSetMenu::engine_picker_rects(SDL_Rect& panel, SDL_Rect& usi, SDL_Rect& random,
                                       SDL_Rect& cancel) const {
    int const pw = 420;
    int const ph = 290;
    panel = SDL_Rect{(BoardView::window_w - pw) / 2, (BoardView::window_h - ph) / 2, pw, ph};
    int const btn_w = pw - 48;
    int const btn_h = 52;
    int const x = panel.x + 24;
    usi = SDL_Rect{x, panel.y + 72, btn_w, btn_h};
    random = SDL_Rect{x, panel.y + 136, btn_w, btn_h};
    cancel = SDL_Rect{x, panel.y + 210, btn_w, 44};
}

MenuAction PieceSetMenu::handle_engine_picker_event(SDL_Event const& e) {
    auto in_rect = [](SDL_Rect r, int x, int y) {
        return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
    };

    SDL_Rect panel, usi, random, cancel;
    engine_picker_rects(panel, usi, random, cancel);

    if (e.type == SDL_KEYDOWN) {
        SDL_Keycode sym = e.key.keysym.sym;
        bool const is_arrow = (sym == SDLK_UP || sym == SDLK_DOWN);
        if (e.key.repeat && !is_arrow) {
            return MenuAction::None;
        }
        if (is_arrow && !accept_nav_key(e.key.repeat != 0)) {
            return MenuAction::None;
        }
        if (!e.key.repeat && sym == SDLK_ESCAPE) {
            close_engine_picker();
            return MenuAction::None;
        }
        if (sym == SDLK_UP) {
            engine_picker_focus_ = (engine_picker_focus_ + 2) % 3;
            return MenuAction::None;
        }
        if (sym == SDLK_DOWN) {
            engine_picker_focus_ = (engine_picker_focus_ + 1) % 3;
            return MenuAction::None;
        }
        if (!e.key.repeat && (sym == SDLK_RETURN || sym == SDLK_SPACE)) {
            return confirm_engine_picker();
        }
        return MenuAction::None;
    }

    // Keyboard-only mode: ignore mouse while picker is open too.
    if (keyboard_board()) {
        return MenuAction::None;
    }

    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x;
        int my = e.motion.y;
        if (in_rect(usi, mx, my)) {
            engine_picker_focus_ = 0;
        } else if (in_rect(random, mx, my)) {
            engine_picker_focus_ = 1;
        } else if (in_rect(cancel, mx, my)) {
            engine_picker_focus_ = 2;
        }
        return MenuAction::None;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int x = e.button.x;
        int y = e.button.y;
        if (in_rect(usi, x, y)) {
            engine_picker_focus_ = 0;
            return confirm_engine_picker();
        }
        if (in_rect(random, x, y)) {
            engine_picker_focus_ = 1;
            return confirm_engine_picker();
        }
        if (in_rect(cancel, x, y) || !in_rect(panel, x, y)) {
            close_engine_picker();
            return MenuAction::None;
        }
    }
    return MenuAction::None;
}

void PieceSetMenu::draw_engine_picker(SDL_Renderer* renderer, SpriteAssets const& assets) const {
    SDL_Rect dim{0, 0, BoardView::window_w, BoardView::window_h};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(renderer, &dim);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_Rect panel, usi, random, cancel;
    engine_picker_rects(panel, usi, random, cancel);

    SDL_SetRenderDrawColor(renderer, 32, 36, 46, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
    SDL_RenderDrawRect(renderer, &panel);

    SDL_Color title{230, 220, 180, 255};
    SDL_Color text{210, 210, 210, 255};
    SDL_Color accent{220, 180, 60, 255};
    SDL_Color dim_c{140, 140, 140, 255};

    draw_text(renderer, assets, panel.x + 24, panel.y + 20, "Choose opponent", title,
              TextSize::Normal);

    struct Row {
        SDL_Rect rect;
        char const* label;
        char const* sub;
        int focus_i;
    };
    Row rows[] = {
        {usi, "Fairy-Stockfish", "Real USI engine", 0},
        {random, "Random legal move", "Joke / practice opponent", 1},
        {cancel, "Cancel", "", 2},
    };

    for (std::size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        Row const& row = rows[i];
        bool focused = (engine_picker_focus_ == row.focus_i);
        bool is_cancel = (row.focus_i == 2);
        if (focused) {
            SDL_SetRenderDrawColor(renderer, is_cancel ? 60 : 70, is_cancel ? 70 : 100,
                                   is_cancel ? 60 : 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 44, 48, 58, 255);
        }
        SDL_RenderFillRect(renderer, &row.rect);
        SDL_SetRenderDrawColor(renderer, focused ? 220 : 120, focused ? 180 : 130,
                               focused ? 60 : 120, 255);
        SDL_RenderDrawRect(renderer, &row.rect);

        int th = measure_text_h(assets, TextSize::Normal);
        int ty = row.rect.y + (row.sub[0] ? 6 : (row.rect.h - th) / 2);
        draw_text(renderer, assets, row.rect.x + 14, ty, row.label, focused ? accent : text,
                  TextSize::Normal);
        if (row.sub[0]) {
            draw_text(renderer, assets, row.rect.x + 14, ty + th + 2, row.sub, dim_c,
                      TextSize::Tiny);
        }
    }
}

void PieceSetMenu::open_setup(bool for_engine) {
    setup_open_ = true;
    setup_for_engine_ = for_engine;
    setup_dragging_ = false;
    setup_ = GameSetupConfig();
    setup_.use_clock = use_clock_;
    setup_.use_handicap = use_handicap_;
    if (setup_.use_clock) {
        setup_focus_ = SetupFocus::MainSlider;
    } else if (setup_.use_handicap) {
        setup_focus_ = (setup_.stronger == Color::Sente) ? SetupFocus::StrongerSente
                                                         : SetupFocus::StrongerGote;
    } else {
        setup_focus_ = SetupFocus::Start;
    }
}

void PieceSetMenu::close_setup() {
    setup_open_ = false;
    setup_dragging_ = false;
}

MenuAction PieceSetMenu::confirm_setup() {
    if (!setup_.clock_valid()) {
        return MenuAction::None;
    }
    close_setup();
    if (setup_for_engine_) {
        open_engine_picker();
        return MenuAction::None;
    }
    return MenuAction::NewGame;
}

void PieceSetMenu::setup_layout(SDL_Rect& panel, SDL_Rect& main_sl, SDL_Rect& inc_sl,
                                SDL_Rect& sente_btn, SDL_Rect& gote_btn, SDL_Rect& rank_sl,
                                SDL_Rect& start_btn, SDL_Rect& cancel_btn) const {
    // Labels are drawn above controls (track.y - 18); reserve that space so they
    // never overlap the Esc line, the previous control, or each other.
    int const pw = 480;
    int const label_space = 22;
    int const track_h = 28;
    int const btn_h = 40;
    int const after_block = 16;
    int const header_h = 48;
    int const footer_h = 72;

    int body = 0;
    if (setup_.use_clock) {
        body += label_space + track_h + after_block; // main
        body += label_space + track_h + after_block; // increment
        if (!setup_.clock_valid()) {
            body += 18;
        }
    }
    if (setup_.use_handicap) {
        body += label_space + btn_h + after_block;   // stronger pair
        body += label_space + track_h + after_block; // rank
    }

    int const ph = header_h + body + footer_h;
    panel = SDL_Rect{(BoardView::window_w - pw) / 2, (BoardView::window_h - ph) / 2, pw, ph};
    int y = panel.y + header_h;
    int const x = panel.x + 24;
    int const tw = pw - 48;

    if (setup_.use_clock) {
        main_sl = SDL_Rect{x, y + label_space, tw, track_h};
        y = main_sl.y + track_h + after_block;
        inc_sl = SDL_Rect{x, y + label_space, tw, track_h};
        y = inc_sl.y + track_h + after_block;
        if (!setup_.clock_valid()) {
            y += 18;
        }
    } else {
        main_sl = SDL_Rect{0, 0, 0, 0};
        inc_sl = SDL_Rect{0, 0, 0, 0};
    }
    if (setup_.use_handicap) {
        int const by = y + label_space;
        sente_btn = SDL_Rect{x, by, (tw - 12) / 2, btn_h};
        gote_btn = SDL_Rect{x + (tw - 12) / 2 + 12, by, (tw - 12) / 2, btn_h};
        y = by + btn_h + after_block;
        rank_sl = SDL_Rect{x, y + label_space, tw, track_h};
        y = rank_sl.y + track_h + after_block;
    } else {
        sente_btn = SDL_Rect{0, 0, 0, 0};
        gote_btn = SDL_Rect{0, 0, 0, 0};
        rank_sl = SDL_Rect{0, 0, 0, 0};
    }
    start_btn = SDL_Rect{x, panel.y + ph - 56, (tw - 12) / 2, 40};
    cancel_btn = SDL_Rect{x + (tw - 12) / 2 + 12, panel.y + ph - 56, (tw - 12) / 2, 40};
}

int PieceSetMenu::slider_value_at(SDL_Rect const& track, int x, int vmin, int vmax) const {
    if (track.w <= 0 || vmax <= vmin) {
        return vmin;
    }
    double t = static_cast<double>(x - track.x) / static_cast<double>(track.w);
    if (t < 0) {
        t = 0;
    }
    if (t > 1) {
        t = 1;
    }
    return vmin + static_cast<int>(t * (vmax - vmin) + 0.5);
}

void PieceSetMenu::apply_slider_drag(int x) {
    SDL_Rect panel, main_sl, inc_sl, sente_btn, gote_btn, rank_sl, start_btn, cancel_btn;
    setup_layout(panel, main_sl, inc_sl, sente_btn, gote_btn, rank_sl, start_btn, cancel_btn);
    if (setup_focus_ == SetupFocus::MainSlider && setup_.use_clock) {
        setup_.main_minutes = slider_value_at(main_sl, x, 0, 180);
    } else if (setup_focus_ == SetupFocus::IncSlider && setup_.use_clock) {
        setup_.increment_seconds = slider_value_at(inc_sl, x, 0, 180);
    } else if (setup_focus_ == SetupFocus::RankSlider && setup_.use_handicap) {
        setup_.rank_diff = slider_value_at(rank_sl, x, 1, 10);
    }
}

MenuAction PieceSetMenu::handle_setup_event(SDL_Event const& e) {
    auto in_rect = [](SDL_Rect r, int x, int y) {
        return r.w > 0 && x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
    };
    SDL_Rect panel, main_sl, inc_sl, sente_btn, gote_btn, rank_sl, start_btn, cancel_btn;
    setup_layout(panel, main_sl, inc_sl, sente_btn, gote_btn, rank_sl, start_btn, cancel_btn);

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        setup_dragging_ = false;
        return MenuAction::None;
    }
    if (e.type == SDL_MOUSEMOTION && setup_dragging_) {
        apply_slider_drag(e.motion.x);
        return MenuAction::None;
    }

    if (e.type == SDL_KEYDOWN) {
        SDL_Keycode sym = e.key.keysym.sym;
        bool arrow = (sym == SDLK_LEFT || sym == SDLK_RIGHT || sym == SDLK_UP || sym == SDLK_DOWN);
        if (e.key.repeat && !arrow) {
            return MenuAction::None;
        }
        if (arrow && !accept_nav_key(e.key.repeat != 0)) {
            return MenuAction::None;
        }
        if (!e.key.repeat && sym == SDLK_ESCAPE) {
            close_setup();
            return MenuAction::None;
        }

        // Vertical nav moves by row; Sente|Gote and Start|Cancel are horizontal pairs.
        enum class SetupRow { Main, Inc, Stronger, Rank, Actions };
        auto row_of = [](SetupFocus f) -> SetupRow {
            if (f == SetupFocus::MainSlider) {
                return SetupRow::Main;
            }
            if (f == SetupFocus::IncSlider) {
                return SetupRow::Inc;
            }
            if (f == SetupFocus::StrongerSente || f == SetupFocus::StrongerGote) {
                return SetupRow::Stronger;
            }
            if (f == SetupFocus::RankSlider) {
                return SetupRow::Rank;
            }
            return SetupRow::Actions;
        };
        auto focus_for_row = [&](SetupRow row) -> SetupFocus {
            if (row == SetupRow::Main) {
                return SetupFocus::MainSlider;
            }
            if (row == SetupRow::Inc) {
                return SetupFocus::IncSlider;
            }
            if (row == SetupRow::Stronger) {
                if (setup_focus_ == SetupFocus::StrongerSente ||
                    setup_focus_ == SetupFocus::StrongerGote) {
                    return setup_focus_;
                }
                return (setup_.stronger == Color::Sente) ? SetupFocus::StrongerSente
                                                         : SetupFocus::StrongerGote;
            }
            if (row == SetupRow::Rank) {
                return SetupFocus::RankSlider;
            }
            if (setup_focus_ == SetupFocus::Start || setup_focus_ == SetupFocus::Cancel) {
                return setup_focus_;
            }
            return SetupFocus::Start;
        };

        auto cycle = [&](int dir) {
            SetupRow rows[6];
            int n = 0;
            if (setup_.use_clock) {
                rows[n++] = SetupRow::Main;
                rows[n++] = SetupRow::Inc;
            }
            if (setup_.use_handicap) {
                rows[n++] = SetupRow::Stronger;
                rows[n++] = SetupRow::Rank;
            }
            rows[n++] = SetupRow::Actions;
            SetupRow cur = row_of(setup_focus_);
            int idx = 0;
            for (int i = 0; i < n; ++i) {
                if (rows[i] == cur) {
                    idx = i;
                    break;
                }
            }
            idx = (idx + dir + n) % n;
            setup_focus_ = focus_for_row(rows[idx]);
        };

        if (sym == SDLK_UP) {
            cycle(-1);
            return MenuAction::None;
        }
        if (sym == SDLK_DOWN) {
            cycle(1);
            return MenuAction::None;
        }
        if (sym == SDLK_LEFT || sym == SDLK_RIGHT) {
            int d = (sym == SDLK_RIGHT) ? 1 : -1;
            if (setup_focus_ == SetupFocus::MainSlider) {
                setup_.main_minutes =
                    std::max(0, std::min(180, setup_.main_minutes + d));
            } else if (setup_focus_ == SetupFocus::IncSlider) {
                setup_.increment_seconds =
                    std::max(0, std::min(180, setup_.increment_seconds + d));
            } else if (setup_focus_ == SetupFocus::RankSlider) {
                setup_.rank_diff = std::max(1, std::min(10, setup_.rank_diff + d));
            } else if (setup_focus_ == SetupFocus::StrongerSente ||
                       setup_focus_ == SetupFocus::StrongerGote) {
                // Style-like: arrows only move the highlight; Enter/Space confirms.
                setup_focus_ = (setup_focus_ == SetupFocus::StrongerSente)
                                   ? SetupFocus::StrongerGote
                                   : SetupFocus::StrongerSente;
            } else if (setup_focus_ == SetupFocus::Start || setup_focus_ == SetupFocus::Cancel) {
                setup_focus_ = (setup_focus_ == SetupFocus::Start) ? SetupFocus::Cancel
                                                                  : SetupFocus::Start;
            }
            return MenuAction::None;
        }
        if (!e.key.repeat && (sym == SDLK_RETURN || sym == SDLK_SPACE)) {
            if (setup_focus_ == SetupFocus::Cancel) {
                close_setup();
                return MenuAction::None;
            }
            if (setup_focus_ == SetupFocus::StrongerSente) {
                setup_.stronger = Color::Sente;
                return MenuAction::None;
            }
            if (setup_focus_ == SetupFocus::StrongerGote) {
                setup_.stronger = Color::Gote;
                return MenuAction::None;
            }
            return confirm_setup();
        }
        return MenuAction::None;
    }

    if (keyboard_board()) {
        return MenuAction::None;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int x = e.button.x;
        int y = e.button.y;
        if (setup_.use_clock && in_rect(main_sl, x, y)) {
            setup_focus_ = SetupFocus::MainSlider;
            setup_dragging_ = true;
            apply_slider_drag(x);
            return MenuAction::None;
        }
        if (setup_.use_clock && in_rect(inc_sl, x, y)) {
            setup_focus_ = SetupFocus::IncSlider;
            setup_dragging_ = true;
            apply_slider_drag(x);
            return MenuAction::None;
        }
        if (setup_.use_handicap && in_rect(sente_btn, x, y)) {
            setup_.stronger = Color::Sente;
            setup_focus_ = SetupFocus::StrongerSente;
            return MenuAction::None;
        }
        if (setup_.use_handicap && in_rect(gote_btn, x, y)) {
            setup_.stronger = Color::Gote;
            setup_focus_ = SetupFocus::StrongerGote;
            return MenuAction::None;
        }
        if (setup_.use_handicap && in_rect(rank_sl, x, y)) {
            setup_focus_ = SetupFocus::RankSlider;
            setup_dragging_ = true;
            apply_slider_drag(x);
            return MenuAction::None;
        }
        if (in_rect(start_btn, x, y)) {
            setup_focus_ = SetupFocus::Start;
            return confirm_setup();
        }
        if (in_rect(cancel_btn, x, y) || !in_rect(panel, x, y)) {
            close_setup();
            return MenuAction::None;
        }
    }
    return MenuAction::None;
}

void PieceSetMenu::draw_slider(SDL_Renderer* renderer, SpriteAssets const& assets, SDL_Rect track,
                               int value, int vmin, int vmax, bool focused,
                               char const* label) const {
    SDL_Color text{210, 210, 210, 255};
    SDL_Color accent{220, 180, 60, 255};
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s: %d", label, value);
    draw_text(renderer, assets, track.x, track.y - 18, buf, focused ? accent : text,
              TextSize::Small);
    SDL_SetRenderDrawColor(renderer, 50, 54, 64, 255);
    SDL_RenderFillRect(renderer, &track);
    SDL_SetRenderDrawColor(renderer, focused ? 220 : 120, focused ? 180 : 130, focused ? 60 : 120,
                           255);
    SDL_RenderDrawRect(renderer, &track);
    double t = (vmax > vmin) ? static_cast<double>(value - vmin) / (vmax - vmin) : 0;
    int kx = track.x + static_cast<int>(t * track.w);
    SDL_Rect knob{kx - 6, track.y - 4, 12, track.h + 8};
    SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
    SDL_RenderFillRect(renderer, &knob);
}

void PieceSetMenu::draw_setup(SDL_Renderer* renderer, SpriteAssets const& assets) const {
    SDL_Rect dim{0, 0, BoardView::window_w, BoardView::window_h};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(renderer, &dim);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_Rect panel, main_sl, inc_sl, sente_btn, gote_btn, rank_sl, start_btn, cancel_btn;
    setup_layout(panel, main_sl, inc_sl, sente_btn, gote_btn, rank_sl, start_btn, cancel_btn);

    SDL_SetRenderDrawColor(renderer, 32, 36, 46, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
    SDL_RenderDrawRect(renderer, &panel);

    SDL_Color title{230, 220, 180, 255};
    SDL_Color text{210, 210, 210, 255};
    SDL_Color accent{220, 180, 60, 255};
    SDL_Color dim_c{140, 140, 140, 255};
    SDL_Color bad{220, 100, 100, 255};

    draw_text(renderer, assets, panel.x + 24, panel.y + 16,
              setup_for_engine_ ? "Game setup (vs engine)" : "Game setup (new game)", title,
              TextSize::Normal);

    if (setup_.use_clock) {
        draw_slider(renderer, assets, main_sl, setup_.main_minutes, 0, 180,
                    setup_focus_ == SetupFocus::MainSlider, "Main time (min)");
        draw_slider(renderer, assets, inc_sl, setup_.increment_seconds, 0, 180,
                    setup_focus_ == SetupFocus::IncSlider, "Increment (sec)");
        if (!setup_.clock_valid()) {
            draw_text(renderer, assets, panel.x + 24, inc_sl.y + 32,
                      "Cannot start with 0 min and 0 sec increment", bad, TextSize::Tiny);
        }
    }
    if (setup_.use_handicap) {
        // Match piece-set style list: focus highlight ≠ confirmed selection.
        auto draw_btn = [&](SDL_Rect r, char const* lab, bool selected, bool focused) {
            if (focused) {
                SDL_SetRenderDrawColor(renderer, 70, 100, 50, 255);
            } else if (selected) {
                SDL_SetRenderDrawColor(renderer, 48, 52, 62, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 44, 48, 58, 255);
            }
            SDL_RenderFillRect(renderer, &r);
            if (focused) {
                SDL_SetRenderDrawColor(renderer, 220, 180, 60, 255);
            } else if (selected) {
                SDL_SetRenderDrawColor(renderer, 120, 120, 100, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
            }
            SDL_RenderDrawRect(renderer, &r);
            if (focused) {
                SDL_Rect inner{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
                SDL_RenderDrawRect(renderer, &inner);
            }
            int tw = measure_text(assets, lab, TextSize::Small);
            draw_text(renderer, assets, r.x + (r.w - tw) / 2, r.y + 10, lab,
                      (focused || selected) ? accent : text, TextSize::Small);
        };
        draw_text(renderer, assets, sente_btn.x, sente_btn.y - 18, "Stronger player (uwate)", text,
                  TextSize::Small);
        draw_btn(sente_btn, "Sente stronger", setup_.stronger == Color::Sente,
                 setup_focus_ == SetupFocus::StrongerSente);
        draw_btn(gote_btn, "Gote stronger", setup_.stronger == Color::Gote,
                 setup_focus_ == SetupFocus::StrongerGote);
        char rank_lab[64];
        std::snprintf(rank_lab, sizeof(rank_lab), "Rank diff (%s)",
                      handicap_label(setup_.rank_diff));
        draw_slider(renderer, assets, rank_sl, setup_.rank_diff, 1, 10,
                    setup_focus_ == SetupFocus::RankSlider, rank_lab);
    }

    auto draw_action = [&](SDL_Rect r, char const* lab, bool focused, bool enabled) {
        if (focused) {
            SDL_SetRenderDrawColor(renderer, enabled ? 70 : 50, enabled ? 100 : 50,
                                   enabled ? 50 : 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 44, 48, 58, 255);
        }
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, focused ? 220 : 120, focused ? 180 : 130,
                               focused ? 60 : 120, 255);
        SDL_RenderDrawRect(renderer, &r);
        int tw = measure_text(assets, lab, TextSize::Normal);
        draw_text(renderer, assets, r.x + (r.w - tw) / 2, r.y + 8, lab,
                  enabled ? (focused ? accent : text) : dim_c, TextSize::Normal);
    };
    draw_action(start_btn, "Start", setup_focus_ == SetupFocus::Start, setup_.clock_valid());
    draw_action(cancel_btn, "Cancel", setup_focus_ == SetupFocus::Cancel, true);
}

} // namespace ui
} // namespace shogi

