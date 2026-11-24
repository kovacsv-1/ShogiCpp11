#ifndef SHOGI_MENU_HPP
#define SHOGI_MENU_HPP

/**
 * @file menu.hpp
 * @brief Main menu: piece sets, options, engine/setup popups.
 */

#include "../core/engine.hpp"
#include "../core/learn.hpp"
#include "assets.hpp"
#include "game_setup.hpp"

#include <string>
#include <vector>

namespace shogi {
namespace ui {

/** Result of menu interaction for the app loop. */
enum class MenuAction {
    None,
    ContinueGame,
    NewGame,
    Learn,
    PlayEngine,
    Quit
};

/**
 * Piece-set chooser and game options.
 * Clock / handicap toggles; New game / Engine may open a setup dialog first.
 * Setup keyboard: ↑/↓ by row; ←/→ within Sente|Gote and Start|Cancel;
 * Enter/Space confirms (uwate like piece-set selection); Esc closes.
 */
class PieceSetMenu {
public:
    PieceSetMenu();

    bool init(SDL_Renderer* renderer, SpriteAssets& assets, std::string const& initial_path);

    /** When true, show Continue (resume paused game). */
    void set_session_active(bool active) { session_active_ = active; }
    bool session_active() const { return session_active_; }

    void set_allow_review_fork(bool on) { allow_review_fork_ = on; }
    bool allow_review_fork() const { return allow_review_fork_; }

    void set_keyboard_board(bool on) { keyboard_board_ = on; }
    bool keyboard_board() const { return keyboard_board_ && keyboard_board_unlocked_; }

    void set_hard_mode(bool on) { hard_mode_ = on; }
    bool hard_mode() const { return hard_mode_ && hard_mode_available(); }

    void set_use_clock(bool on) { use_clock_ = on; }
    bool use_clock() const { return use_clock_; }
    void set_use_handicap(bool on) { use_handicap_ = on; }
    bool use_handicap() const { return use_handicap_; }

    /** Kind chosen in the engine popup (valid after PlayEngine). */
    EngineKind chosen_engine() const { return chosen_engine_; }

    /** Confirmed setup from the last NewGame / PlayEngine dialog. */
    GameSetupConfig const& game_setup() const { return setup_; }

    MenuAction handle_event(SDL_Event const& e, SpriteAssets& assets, SDL_Renderer* renderer);
    void draw(SDL_Renderer* renderer, SpriteAssets const& assets) const;

    int selected_index() const { return selected_; }
    std::string const& selected_path() const;

private:
    enum class NavFocus {
        Style,
        UseClock,
        UseHandicap,
        Takebacks,
        HardMode,
        KeyboardOnly,
        Bottom
    };
    enum class TextSize { Normal, Small, Tiny };
    enum class BottomId { Continue, NewGame, Learn, Engine, Quit };
    enum class SetupFocus {
        MainSlider,
        IncSlider,
        StrongerSente,
        StrongerGote,
        RankSlider,
        Start,
        Cancel
    };

    struct BottomBtn {
        BottomId id;
        SDL_Rect rect;
        char const* label;
        bool primary;
        bool enabled;
    };

    std::vector<PieceSetInfo> all_sets_;
    std::vector<PieceSetInfo> sets_;
    int selected_;
    int style_focus_;
    NavFocus nav_focus_;
    int bottom_focus_;
    std::vector<SDL_Keycode> konami_history_;
    bool koma_unlocked_;
    bool engine_session_unlocked_;
    bool keyboard_board_unlocked_;
    bool keyboard_board_;
    bool hard_mode_konami_unlocked_;
    bool hard_mode_;
    bool use_clock_;
    bool use_handicap_;
    learn::LearnProgress learn_progress_;
    mutable int unlock_flash_frames_;
    mutable int engine_flash_frames_;
    bool session_active_;
    bool allow_review_fork_;
    bool engine_picker_open_;
    int engine_picker_focus_;
    EngineKind chosen_engine_;

    bool setup_open_;
    bool setup_for_engine_;
    SetupFocus setup_focus_;
    GameSetupConfig setup_;
    bool setup_dragging_;

    void rebuild_visible();
    bool is_selectable(int index) const;
    bool is_focusable(int index) const;
    int first_selectable() const;
    int last_selectable() const;
    int first_focusable() const;
    int last_focusable() const;
    int next_focusable(int from, int dir) const;
    int next_selectable(int from, int dir) const;
    bool feed_konami(SDL_Keycode key);
    bool engine_unlocked() const;
    bool blank_unlocked() const;
    bool hard_mode_available() const;
    void refresh_learn_progress();
    SDL_Rect list_item_rect(int index) const;
    /** 0=clock, 1=handicap, 2=takebacks, 3=hard, 4=keyboard (row chrome; bottom buttons unchanged). */
    SDL_Rect option_row_rect(int index) const;
    /** Small square inside @ref option_row_rect (label + Tiny subtitle below). */
    SDL_Rect option_checkbox_rect(int index) const;

    int layout_bottom_buttons(SpriteAssets const& assets, BottomBtn* out, int max_out) const;
    static int measure_text(SpriteAssets const& assets, char const* text, TextSize size);
    static int measure_text_h(SpriteAssets const& assets, TextSize size);
    void draw_text(SDL_Renderer* renderer, SpriteAssets const& assets, int x, int y,
                   std::string const& text, SDL_Color color, TextSize size) const;
    void draw_option_row(SDL_Renderer* renderer, SpriteAssets const& assets, int index,
                         bool focused, bool checked, bool locked, char const* label,
                         char const* subtitle, SDL_Color accent, SDL_Color text,
                         SDL_Color dim) const;
    bool select_index(int index, SpriteAssets& assets, SDL_Renderer* renderer);

    MenuAction activate_focused(SpriteAssets& assets, SDL_Renderer* renderer);
    MenuAction activate_bottom(BottomBtn const& b);
    void nav_vertical(int dir);
    void nav_horizontal(int dir, SpriteAssets const& assets);

    void open_engine_picker();
    void close_engine_picker();
    MenuAction confirm_engine_picker();
    void engine_picker_rects(SDL_Rect& panel, SDL_Rect& usi, SDL_Rect& random,
                             SDL_Rect& cancel) const;
    MenuAction handle_engine_picker_event(SDL_Event const& e);
    void draw_engine_picker(SDL_Renderer* renderer, SpriteAssets const& assets) const;

    void open_setup(bool for_engine);
    void close_setup();
    MenuAction confirm_setup();
    void setup_layout(SDL_Rect& panel, SDL_Rect& main_sl, SDL_Rect& inc_sl, SDL_Rect& sente_btn,
                      SDL_Rect& gote_btn, SDL_Rect& rank_sl, SDL_Rect& start_btn,
                      SDL_Rect& cancel_btn) const;
    int slider_value_at(SDL_Rect const& track, int x, int vmin, int vmax) const;
    void apply_slider_drag(int x);
    MenuAction handle_setup_event(SDL_Event const& e);
    void draw_setup(SDL_Renderer* renderer, SpriteAssets const& assets) const;
    void draw_slider(SDL_Renderer* renderer, SpriteAssets const& assets, SDL_Rect track, int value,
                     int vmin, int vmax, bool focused, char const* label) const;
};

} // namespace ui
} // namespace shogi

#endif
