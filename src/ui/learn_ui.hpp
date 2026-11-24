#ifndef SHOGI_LEARN_UI_HPP
#define SHOGI_LEARN_UI_HPP

/**
 * @file learn_ui.hpp
 * @brief SDL UI for the tutorial (stage browser + level play).
 */

#include "../core/learn.hpp"
#include "board_marks.hpp"
#include "view.hpp"

#include <string>
#include <vector>

namespace shogi {
namespace ui {

/** Result of learn-screen events for the app loop. */
enum class LearnAction {
    None,
    QuitApp,
    BackToMenu
};

/** Stage browser + level play for Lishogi-style learn mode. */
class LearnUi {
public:
    LearnUi();

    void open();
    void set_keyboard_board(bool on);
    bool keyboard_board() const { return keyboard_board_; }

    LearnAction handle_event(SDL_Event const& e, BoardView const& view);
    void draw(SDL_Renderer* renderer, BoardView const& view, SpriteAssets& assets) const;

private:
    enum class Mode { Browser, Play };

    Mode mode_;
    learn::LearnSession session_;
    learn::LearnProgress progress_;
    int browser_focus_; // 0..n-1 stages, n = Back (arrows + hover)
    int side_focus_;    // 0..4 play side buttons (Retry..Menu)

    bool has_selection_;
    Square selected_;
    bool has_drop_;
    PieceType drop_type_;
    bool asking_promote_;
    Move pending_move_;
    std::vector<Square> highlights_;
    BoardMarks marks_;
    bool rmb_down_;
    Square rmb_from_;
    bool lmb_down_;
    bool dragging_;
    int drag_x_;
    int drag_y_;
    int lmb_down_x_;
    int lmb_down_y_;
    Piece drag_piece_;

    bool keyboard_board_;
    bool board_nav_;
    Square board_cursor_;
    HandNavFocus hand_nav_;
    int hand_slot_;

    void clear_selection();
    void recompute_highlights();
    void on_hand_click(PieceType t);
    void commit_move(Move m);
    bool try_move_to(Square to);
    void end_lmb_drag(BoardView const& view, int x, int y);
    LearnAction activate_side_button(int index);
    void move_board_cursor(int dfile, int drank);
    void activate_board_cursor();
    void draw_browser(SDL_Renderer* renderer, SpriteAssets& assets) const;
    void draw_play(SDL_Renderer* renderer, BoardView const& view, SpriteAssets& assets) const;
};

} // namespace ui
} // namespace shogi

#endif
