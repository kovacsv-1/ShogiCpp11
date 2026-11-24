#ifndef SHOGI_CONTROLLER_HPP
#define SHOGI_CONTROLLER_HPP

/**
 * @file controller.hpp
 * @brief In-game input: mouse/keyboard, promotion, furigoma, vs-engine tick.
 */

#include "../core/engine.hpp"
#include "../core/game.hpp"
#include "board_marks.hpp"
#include "game_setup.hpp"
#include "view.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace shogi {
namespace ui {

/**
 * Drives a live Game: selection, drops, side buttons, optional Engine.
 * Owns the opponent engine through @c std::unique_ptr.
 */
class InputController {
public:
    explicit InputController(Game& game);

    /** @return false = quit app; sets open_menu if Menu was clicked / Esc. */
    bool handle_event(SDL_Event const& e, BoardView const& view);
    void render(SDL_Renderer* renderer, BoardView const& view) const;

    /** Poll engine bestmove / kick off engine think. Call once per frame. */
    void tick();

    std::string const& status() const { return status_; }
    bool consume_open_menu();
    void clear_selection();

    void set_keyboard_board(bool on);
    bool keyboard_board() const { return keyboard_board_; }

    void set_hard_mode(bool on);
    bool hard_mode() const { return hard_mode_; }

    /** Reset board; uses @p setup for clock/handicap (furigoma if no handicap). */
    void start_new_game(GameSetupConfig const& setup = GameSetupConfig());

    /**
     * Human (always Sente) vs chosen Engine (Gote).
     * Furigoma decides who moves first when there is no handicap.
     * @return false if engine failed to start (status_ explains).
     */
    bool start_engine_game(EngineKind kind = EngineKind::FairyStockfish,
                           GameSetupConfig const& setup = GameSetupConfig(),
                           std::string const& exe_path = std::string());

    bool vs_engine() const { return vs_engine_; }
    void stop_engine();

private:
    bool open_menu_;
    Game& game_;
    bool has_selection_;
    Square selected_;
    bool has_drop_;
    PieceType drop_type_;
    bool asking_promote_;
    Move pending_move_;
    std::vector<Square> highlights_;
    std::string status_;

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
    int side_focus_; // 0..4 Resign..Menu; 5 = Claim impasse when available
    /** First press arms (red); second confirms. Cleared by click elsewhere / other activate / Tab. */
    bool resign_armed_;
    bool new_armed_;
    bool claim_armed_;

    bool keyboard_board_;
    bool board_nav_; // Tab: focus on board/hands vs side buttons
    Square board_cursor_;
    HandNavFocus hand_nav_; // Board focus uses HandNavFocus::None
    int hand_slot_;         // 0..6 when hand_nav_ is Gote/Sente
    bool hard_mode_;

    bool furigoma_visible_;
    std::array<FurigomaPaint, 5> furigoma_pawns_;

    bool vs_engine_;
    Color human_color_;
    Color engine_color_;
    bool engine_thinking_;
    int engine_movetime_ms_;
    std::unique_ptr<Engine> engine_;
    GameSetupConfig last_setup_;
    std::uint32_t last_tick_ms_;

    void recompute_highlights();
    void refresh_status();
    void apply_setup_to_game(GameSetupConfig const& setup, bool allow_furigoma);
    static std::string format_clock_ms(int ms);
    void on_hand_click(Color hand_owner, PieceType t);
    void commit_move(Move m);
    bool try_move_to(Square to);
    void end_lmb_drag(BoardView const& view, int x, int y);
    void activate_side_button(int index);
    void move_board_cursor(int dfile, int drank);
    void activate_board_cursor(BoardView const& view);
    void roll_furigoma();
    void dismiss_furigoma();
    void clear_confirm_arms();
    void set_side_focus(int index);
    int side_button_count() const;

    bool human_to_move() const;
    void maybe_request_engine_move();
    void apply_engine_bestmove(std::string const& token);
};

} // namespace ui
} // namespace shogi

#endif
