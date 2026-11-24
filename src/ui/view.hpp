#ifndef SHOGI_VIEW_HPP
#define SHOGI_VIEW_HPP

/**
 * @file view.hpp
 * @brief Board layout, hit-testing, and SDL drawing for game / learn screens.
 */

#include "../core/game.hpp"
#include "../core/learn.hpp"
#include "assets.hpp"
#include "board_marks.hpp"

#include <array>
#include <string>
#include <vector>

namespace shogi {
namespace ui {

/** What a pointer hit resolved to. */
enum class UiHitKind {
    None,
    Board,
    HandSente,
    HandGote,
    ButtonResign,
    ButtonSave,
    ButtonLoad,
    ButtonNew,
    ButtonMenu,
    ButtonClaimImpasse
};

/** Click result on an in-square promotion picker. */
enum class PromotePick {
    None,
    Keep,    // unpromoted (upper third)
    Promote  // promoted (lower third)
};

struct UiHit {
    UiHitKind kind;
    Square square;
    PieceType hand_type;

    UiHit() : kind(UiHitKind::None), square(), hand_type(PieceType::Pawn) {}
};

/** Floating piece under the cursor while dragging. */
struct DragPaint {
    bool active;
    Piece piece;
    int x;
    int y;

    DragPaint() : active(false), piece(), x(0), y(0) {}
};

/** One of five furigoma pawns drawn over the board until dismissed. */
struct FurigomaPaint {
    Color color;
    bool promoted;
    double angle_deg;

    FurigomaPaint() : color(Color::Sente), promoted(false), angle_deg(0.0) {}
};

/** Keyboard focus on a hand strip (above = Gote, below = Sente). */
enum class HandNavFocus { None, Gote, Sente };

/**
 * Geometry and rendering for the logical 1280×960 playfield
 * (hands, board, side buttons, status, history).
 * The OS window may be resized/fullscreen; drawing stays in these coordinates
 * via @c SDL_RenderSetLogicalSize (SDL remaps mouse events).
 */
class BoardView {
public:
    // Logical layout (1280×960): Gote hand | board | Sente hand | status
    static int const window_w = 1280;
    static int const window_h = 960;
    static int const cell_size = 78;
    static int const board_origin_x = 100;
    static int const hand_area_h = 86;
    static int const board_origin_y = hand_area_h + 12;          // 98
    static int const sente_hand_y = board_origin_y + 9 * cell_size + 10; // 810
    static int const status_y = window_h - 34;                   // 926
    static int const gote_hand_y = 6;

    explicit BoardView(SpriteAssets& assets);

    void draw(SDL_Renderer* renderer, Game const& game,
              Square const* selected,
              std::vector<Square> const& highlights,
              PieceType const* selected_drop,
              Square const* promote_at,
              Square const* promote_from,
              Piece const* promote_piece,
              BoardMarks const* marks,
              DragPaint const* drag,
              std::string const& status_line,
              int side_focus = -1,
              Square const* board_cursor = nullptr,
              bool cursor_green = false,
              FurigomaPaint const* furigoma = nullptr,
              int furigoma_count = 0,
              Color furigoma_starter = Color::Sente,
              HandNavFocus hand_nav = HandNavFocus::None,
              int hand_slot = 0,
              bool resign_armed = false,
              bool furigoma_tab_dismiss = false,
              bool new_armed = false,
              bool claim_available = false,
              bool claim_armed = false) const;

    void draw_learn(SDL_Renderer* renderer, learn::LearnSession const& session,
                    Square const* selected, std::vector<Square> const& highlights,
                    PieceType const* selected_drop, Square const* promote_at,
                    Square const* promote_from, Piece const* promote_piece,
                    BoardMarks const* marks, DragPaint const* drag,
                    int side_focus = -1,
                    Square const* board_cursor = nullptr,
                    bool cursor_green = false,
                    HandNavFocus hand_nav = HandNavFocus::None,
                    int hand_slot = 0) const;

    /** Hand strip piece order (R B G S N L P). */
    static std::array<PieceType, 7> const& hand_type_order();

    /** Pixel rect of hand slot `slot` (0..6) for Gote or Sente strip. */
    SDL_Rect hand_slot_rect(HandNavFocus which, int slot) const;

    UiHit hit_test(int x, int y) const;

    /** True if (x,y) is over the move-history panel (game view). */
    bool hit_history_panel(int x, int y) const;

    /** Scroll move list by wheel ticks (negative = up / older). */
    void scroll_history(int wheel_y) const;

    /**
     * Two-square picker along the move: Keep on one square, Promote on the other.
     * @param x    Pointer x in window coordinates.
     * @param y    Pointer y in window coordinates.
     * @param from Origin of the pending move.
     * @param to   Destination of the pending move.
     */
    PromotePick hit_promote(int x, int y, Square from, Square to) const;

    /** Fills keep_sq / promote_sq for the two-square promotion UI. */
    static void promote_squares(Square from, Square to, Square& keep_sq, Square& promote_sq);

    /**
     * Overlay: five tossed pawns + who starts.
     * @param renderer SDL renderer.
     * @param pawns Toss results (@p count entries).
     * @param count Number of pawns (usually 5).
     * @param starter Side that won the toss.
     * @param keyboard_dismiss If true, hint says Tab; else click the board.
     */
    void draw_furigoma(SDL_Renderer* renderer, FurigomaPaint const* pawns, int count,
                       Color starter, bool keyboard_dismiss = false) const;

private:
    SpriteAssets& assets_;
    mutable int history_scroll_;
    mutable std::size_t history_scroll_ply_;

    void draw_board_grid(SDL_Renderer* renderer) const;
    void draw_pieces(SDL_Renderer* renderer, Board const& board,
                     Square const* hide_square = nullptr) const;
    void draw_hand(SDL_Renderer* renderer, Color c, Hand const& hand, int y0,
                   bool name_lit) const;
    void draw_hand_cursor(SDL_Renderer* renderer, HandNavFocus hand_nav, int hand_slot) const;
    /**
     * Side-panel button. Inactive uses locked “Play against engine” chrome (incl. hover).
     * @param danger Red “armed” confirm chrome (Resign / New / Claim impasse).
     */
    void draw_button(SDL_Renderer* renderer, SDL_Rect rect, char const* label, bool active,
                     bool focused = false, bool danger = false) const;
    void draw_text(SDL_Renderer* renderer, int x, int y, std::string const& text,
                   SDL_Color color, bool small) const;
    void draw_move_history(SDL_Renderer* renderer, Game const& game) const;
    void draw_promote_picker(SDL_Renderer* renderer, Square from, Square to,
                             Piece const& piece) const;
    void draw_promote_choice_square(SDL_Renderer* renderer, Square s, Piece const& shown) const;
    void draw_marks(SDL_Renderer* renderer, BoardMarks const& marks) const;
    void draw_drag_piece(SDL_Renderer* renderer, DragPaint const& drag) const;

    SDL_Rect board_square_rect(Square s) const;
    SDL_Rect history_panel_rect() const;
    SDL_Rect side_btn(int index) const;
    SDL_Rect resign_btn() const { return side_btn(0); }
    SDL_Rect save_btn() const { return side_btn(1); }
    SDL_Rect load_btn() const { return side_btn(2); }
    SDL_Rect new_btn() const { return side_btn(3); }
    SDL_Rect menu_btn() const { return side_btn(4); }
    /** Below Menu with a gap; blank koma-style spacer when claim not available. */
    SDL_Rect claim_impasse_btn() const;

    static int const hand_cell = 52;
    static int const hand_gap = 78; // room for piece + xN count
    static int const history_row_h = 22;
};

} // namespace ui
} // namespace shogi

#endif
