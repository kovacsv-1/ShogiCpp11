#include "controller.hpp"
#include "file_dialog.hpp"
#include "key_repeat.hpp"

#include "../core/handicap.hpp"
#include "../core/learn.hpp"
#include "../core/rules.hpp"

#include <SDL2/SDL.h>
#include <cstdio>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <stdexcept>

namespace shogi {
namespace ui {
namespace {

int const kDragThresholdPx = 6;

std::mt19937& rng() {
    static std::mt19937 gen(static_cast<std::mt19937::result_type>(
        std::time(nullptr) ^ static_cast<std::time_t>(std::clock())));
    return gen;
}

} // namespace

InputController::InputController(Game& game)
    : open_menu_(false),
      game_(game),
      has_selection_(false),
      selected_(),
      has_drop_(false),
      drop_type_(PieceType::Pawn),
      asking_promote_(false),
      pending_move_(),
      highlights_(),
      status_("Sente to move. Drag a piece, or click then click."),
      rmb_down_(false),
      rmb_from_(),
      lmb_down_(false),
      dragging_(false),
      drag_x_(0),
      drag_y_(0),
      lmb_down_x_(0),
      lmb_down_y_(0),
      drag_piece_(),
      side_focus_(0),
      resign_armed_(false),
      new_armed_(false),
      claim_armed_(false),
      keyboard_board_(false),
      board_nav_(false),
      board_cursor_(5, 5),
      hand_nav_(HandNavFocus::None),
      hand_slot_(0),
      hard_mode_(false),
      furigoma_visible_(false),
      furigoma_pawns_(),
      vs_engine_(false),
      human_color_(Color::Sente),
      engine_color_(Color::Gote),
      engine_thinking_(false),
      engine_movetime_ms_(800),
      engine_(),
      last_setup_(),
      last_tick_ms_(SDL_GetTicks()) {}

void InputController::set_keyboard_board(bool on) {
    keyboard_board_ = on;
    if (!on) {
        board_nav_ = false;
        hand_nav_ = HandNavFocus::None;
    } else {
        board_nav_ = true;
        hand_nav_ = HandNavFocus::None;
    }
}

void InputController::set_hard_mode(bool on) {
    hard_mode_ = on;
    game_.set_hard_mode(on);
    recompute_highlights();
}

void InputController::roll_furigoma() {
    std::uniform_int_distribution<int> coin(0, 1);
    std::uniform_real_distribution<double> angle(0.0, 360.0);
    int promoted = 0;
    for (int i = 0; i < 5; ++i) {
        FurigomaPaint& p = furigoma_pawns_[static_cast<std::size_t>(i)];
        p.color = coin(rng()) == 0 ? Color::Sente : Color::Gote;
        p.promoted = coin(rng()) != 0;
        p.angle_deg = angle(rng());
        if (p.promoted) {
            ++promoted;
        }
    }
    Color first = Game::furigoma_starter(promoted, 5);
    game_.reset_starting(first);
    furigoma_visible_ = true;
    if (keyboard_board_) {
        // Side-menu focus while the overlay is up; Tab dismisses and moves to the board.
        board_nav_ = false;
        hand_nav_ = HandNavFocus::None;
        set_side_focus(0);
    }
    status_ = std::string("Furigoma: ") + (first == Color::Sente ? "Sente" : "Gote") +
              (keyboard_board_ ? " starts — Tab to continue."
                               : " starts — click the board.");
}

void InputController::dismiss_furigoma() {
    if (!furigoma_visible_) {
        return;
    }
    furigoma_visible_ = false;
    refresh_status();
}

void InputController::apply_setup_to_game(GameSetupConfig const& setup, bool allow_furigoma) {
    last_setup_ = setup;
    if (setup.use_handicap) {
        game_.reset_handicap(setup.to_handicap());
        furigoma_visible_ = false;
    } else if (allow_furigoma) {
        roll_furigoma();
    } else {
        game_.reset();
        furigoma_visible_ = false;
    }
    if (setup.use_clock) {
        game_.configure_clock(true, setup.main_minutes, setup.increment_seconds);
    } else {
        game_.clear_clock();
    }
    last_tick_ms_ = SDL_GetTicks();
}

std::string InputController::format_clock_ms(int ms) {
    if (ms < 0) {
        ms = 0;
    }
    int total_sec = ms / 1000;
    int m = total_sec / 60;
    int s = total_sec % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

void InputController::start_new_game(GameSetupConfig const& setup) {
    stop_engine();
    clear_confirm_arms();
    clear_selection();
    marks_.clear();
    apply_setup_to_game(setup, true);
    refresh_status();
}

void InputController::stop_engine() {
    engine_thinking_ = false;
    vs_engine_ = false;
    if (engine_) {
        engine_->stop_search();
        engine_->shutdown();
        engine_.reset();
    }
}

bool InputController::start_engine_game(EngineKind kind, GameSetupConfig const& setup,
                                        std::string const& exe_path) {
    stop_engine();
    clear_selection();
    marks_.clear();
    set_hard_mode(false);
    // Furigoma (unless handicap): decides who moves first; human stays Sente.
    apply_setup_to_game(setup, true);
    human_color_ = Color::Sente;
    engine_color_ = Color::Gote;
    vs_engine_ = true;
    engine_thinking_ = false;

    try {
        engine_ = make_engine(kind, exe_path);
        engine_->start();
    } catch (std::exception const& ex) {
        stop_engine();
        status_ = std::string("Engine failed: ") + ex.what();
        return false;
    }
    if (furigoma_visible_) {
        // roll_furigoma already set the status; note opponent.
        status_ += std::string("  |  vs ") + engine_->name() + " (you: Sente)";
        return true;
    }
    if (kind == EngineKind::RandomLegal) {
        status_ = std::string("vs ") + engine_->name() + " — you are Sente.";
    } else {
        status_ = std::string("vs ") + engine_->name() + " — you are Sente. Think ~"
                  + std::to_string(engine_movetime_ms_) + " ms.";
    }
    if (setup.use_handicap) {
        status_ += std::string(" | HC ") + handicap_label(setup.rank_diff);
    }
    refresh_status();
    return true;
}

bool InputController::human_to_move() const {
    if (!game_.can_accept_moves()) {
        return false;
    }
    if (furigoma_visible_) {
        return false;
    }
    if (!vs_engine_) {
        return true;
    }
    return game_.side_to_move() == human_color_ && !engine_thinking_;
}

void InputController::maybe_request_engine_move() {
    if (!vs_engine_ || !engine_ || engine_thinking_) {
        return;
    }
    if (!game_.can_accept_moves() || game_.side_to_move() != engine_color_) {
        return;
    }
    try {
        std::string sfen =
            learn::to_sfen(game_.board(), game_.side_to_move(),
                           static_cast<int>(game_.history().size()) + 1);
        engine_->go_sfen(sfen, engine_movetime_ms_);
        engine_thinking_ = true;
        status_ = "Engine thinking...";
    } catch (std::exception const& ex) {
        status_ = std::string("Engine error: ") + ex.what();
        stop_engine();
    }
}

void InputController::apply_engine_bestmove(std::string const& token) {
    engine_thinking_ = false;
    if (token.empty() || token == "resign" || token == "(none)") {
        game_.resign(); // engine side resigns — side_to_move is engine
        // resign() resigns the current side — good.
        status_ = game_.message().empty() ? "Engine resigns." : game_.message();
        clear_selection();
        return;
    }
    if (token == "win") {
        // Declaration — treat as engine win if supported; else ignore.
        status_ = "Engine claims win (ignored).";
        return;
    }
    Move m;
    if (!learn::parse_usi_move(token, game_.board(), game_.side_to_move(), m)) {
        status_ = "Engine move illegal: " + token;
        return;
    }
    if (!game_.try_play(m)) {
        status_ = game_.message().empty() ? ("Engine move failed: " + token) : game_.message();
        return;
    }
    clear_selection();
    refresh_status();
    status_ = std::string("Engine: ") + token + "  |  " + status_;
}

void InputController::tick() {
    std::uint32_t now = SDL_GetTicks();
    int dt = static_cast<int>(now - last_tick_ms_);
    last_tick_ms_ = now;
    if (dt > 0 && dt < 1000 && !furigoma_visible_) {
        GameStatus before = game_.status();
        game_.tick_clock(dt);
        if (game_.clock_enabled() && game_.status() == GameStatus::Playing) {
            refresh_status();
        }
        if (before == GameStatus::Playing && game_.status() != GameStatus::Playing) {
            clear_selection();
            refresh_status();
            if (vs_engine_ && engine_ && engine_thinking_) {
                engine_->stop_search();
                engine_thinking_ = false;
            }
        }
    }

    if (!vs_engine_ || !engine_) {
        return;
    }
    if (engine_thinking_) {
        std::string bm;
        if (engine_->poll_bestmove(bm)) {
            apply_engine_bestmove(bm);
        }
        return;
    }
    if (!furigoma_visible_) {
        maybe_request_engine_move();
    }
}

void InputController::move_board_cursor(int dfile, int drank) {
    if (asking_promote_) {
        Square keep_sq;
        Square promo_sq;
        BoardView::promote_squares(pending_move_.from, pending_move_.to, keep_sq, promo_sq);
        // Only the two promote choices are reachable.
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

    // Hands: LEFT/RIGHT move slot (no wrap). UP/DOWN enter/leave board (no wrap off ends).
    if (hand_nav_ == HandNavFocus::Gote) {
        if (dfile != 0) {
            int s = hand_slot_ - dfile; // LEFT (dfile>0) → lower index (visual left)
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
            // Down onto the board (rank 1 / top row).
            hand_nav_ = HandNavFocus::None;
            board_cursor_ = Square{board_cursor_.file, 1};
        }
        // drank < 0: already above board — no wrap
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
            // Up onto the board (rank 9 / bottom row).
            hand_nav_ = HandNavFocus::None;
            board_cursor_ = Square{board_cursor_.file, 9};
        }
        // drank > 0: already below board — no wrap
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
        // Off the top of the board → Gote hand (no wrap past hand).
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

void InputController::activate_board_cursor(BoardView const& view) {
    (void)view;
    clear_confirm_arms();
    if (furigoma_visible_) {
        // Keyboard-only: dismiss with Tab, not Enter/Space.
        if (!keyboard_board_) {
            dismiss_furigoma();
        }
        return;
    }
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

    if (vs_engine_ && !human_to_move()) {
        status_ = engine_thinking_ ? "Engine thinking..." : "Not your turn.";
        return;
    }

    if (hand_nav_ != HandNavFocus::None) {
        Color owner = (hand_nav_ == HandNavFocus::Gote) ? Color::Gote : Color::Sente;
        PieceType t = BoardView::hand_type_order()[static_cast<std::size_t>(hand_slot_)];
        on_hand_click(owner, t);
        return;
    }

    if (!game_.can_accept_moves()) {
        if (!game_.at_current_ply()) {
            status_ = game_.allow_review_fork()
                          ? "Cannot move here."
                          : "Not at current ply — press →, or enable takebacks in Menu.";
        }
        return;
    }

    auto const& slot = game_.board().at(board_cursor_);
    if (has_drop_ || has_selection_) {
        try_move_to(board_cursor_);
        return;
    }
    if (slot.occupied && slot.piece.color == game_.side_to_move()) {
        selected_ = board_cursor_;
        has_selection_ = true;
        has_drop_ = false;
        drag_piece_ = slot.piece;
        recompute_highlights();
        status_ = "Selected " + slot.piece.notation_token() + " on " + board_cursor_.to_string();
    }
}

bool InputController::consume_open_menu() {
    bool v = open_menu_;
    open_menu_ = false;
    return v;
}

void InputController::clear_selection() {
    has_selection_ = false;
    has_drop_ = false;
    asking_promote_ = false;
    dragging_ = false;
    lmb_down_ = false;
    highlights_.clear();
}

void InputController::clear_confirm_arms() {
    resign_armed_ = false;
    new_armed_ = false;
    claim_armed_ = false;
}

void InputController::set_side_focus(int index) {
    // Navigating (arrows / hover) must not clear confirm arms — only clicks / activates.
    int const n = side_button_count();
    if (index < 0) {
        index = 0;
    }
    if (index >= n) {
        index = n - 1;
    }
    side_focus_ = index;
}

int InputController::side_button_count() const {
    return game_.can_claim_jishogi() ? 6 : 5;
}

void InputController::recompute_highlights() {
    highlights_.clear();
    if (hard_mode_ || furigoma_visible_) {
        return;
    }
    auto legal = game_.legal_moves();
    if (has_drop_) {
        for (auto const& m : legal) {
            if (m.is_drop && m.drop_type == drop_type_) {
                highlights_.push_back(m.to);
            }
        }
        return;
    }
    if (!has_selection_) {
        return;
    }
    for (auto const& m : legal) {
        if (!m.is_drop && m.from == selected_) {
            if (std::find(highlights_.begin(), highlights_.end(), m.to) == highlights_.end()) {
                highlights_.push_back(m.to);
            }
        }
    }
}

void InputController::commit_move(Move m) {
    if (game_.try_play(m)) {
        clear_selection();
        if (game_.status() == GameStatus::Playing) {
            status_ = (game_.side_to_move() == Color::Sente ? "Sente" : "Gote");
            status_ += " to move.";
            if (Rules::in_check(game_.board(), game_.side_to_move())) {
                status_ += " Check!";
            }
        } else {
            status_ = game_.message();
        }
    } else {
        status_ = game_.message().empty() ? "Illegal move." : game_.message();
        clear_selection();
    }
}

void InputController::refresh_status() {
    if (!game_.can_claim_jishogi()) {
        claim_armed_ = false;
        if (side_focus_ >= 5) {
            side_focus_ = 4;
        }
    }
    if (!game_.at_current_ply()) {
        status_ = game_.message();
        return;
    }
    if (game_.status() != GameStatus::Playing) {
        status_ = game_.message();
        return;
    }
    status_ = (game_.side_to_move() == Color::Sente ? "Sente" : "Gote");
    status_ += " to move.";
    if (Rules::in_check(game_.board(), game_.side_to_move())) {
        status_ += " Check!";
    }
    if (game_.clock_enabled()) {
        status_ += "  |  S " + format_clock_ms(game_.clock_remaining_ms(Color::Sente));
        status_ += "  G " + format_clock_ms(game_.clock_remaining_ms(Color::Gote));
    }
    if (game_.handicap().enabled) {
        status_ += "  |  HC ";
        status_ += handicap_label(game_.handicap().rank_diff);
    }
    if (game_.can_redo()) {
        status_ += game_.allow_review_fork() ? "  [→ redo | takebacks on]" : "  [→ redo]";
    }
}

bool InputController::try_move_to(Square to) {
    if (furigoma_visible_) {
        return false;
    }
    if (vs_engine_ && !human_to_move()) {
        status_ = engine_thinking_ ? "Engine thinking..." : "Not your turn.";
        return false;
    }
    if (!game_.can_accept_moves()) {
        if (!game_.at_current_ply()) {
            status_ = game_.allow_review_fork()
                          ? "Cannot move here."
                          : "Not at current ply — press →, or enable takebacks in Menu.";
        }
        return false;
    }

    if (has_drop_) {
        Move m = Move::drop(drop_type_, to);
        auto legal = game_.legal_moves();
        auto it = std::find(legal.begin(), legal.end(), m);
        if (it != legal.end()) {
            commit_move(*it);
            return true;
        }
        if (hard_mode_) {
            game_.lose_by_illegal();
            status_ = game_.message();
            clear_selection();
            return true;
        }
        status_ = "Illegal drop.";
        clear_selection();
        return false;
    }

    if (!has_selection_) {
        return false;
    }

    if (to == selected_) {
        return false;
    }

    auto legal = game_.legal_moves();
    std::vector<Move> matches;
    std::copy_if(legal.begin(), legal.end(), std::back_inserter(matches), [&](Move const& m) {
        return !m.is_drop && m.from == selected_ && m.to == to;
    });
    if (matches.empty()) {
        auto const& dest = game_.board().at(to);
        if (dest.occupied && dest.piece.color == game_.side_to_move()) {
            // Drag onto own piece → reselect (not an illegal attempt).
            selected_ = to;
            has_selection_ = true;
            has_drop_ = false;
            drag_piece_ = dest.piece;
            recompute_highlights();
            status_ = "Selected " + dest.piece.notation_token() + " on " + to.to_string();
            return true;
        }
        if (hard_mode_) {
            game_.lose_by_illegal();
            status_ = game_.message();
            clear_selection();
            return true;
        }
        status_ = "Illegal move.";
        // Keep current selection; releasing over another piece must not select it.
        return false;
    }

    bool has_promo = std::any_of(matches.begin(), matches.end(), [](Move const& m) {
        return m.promotion == PromotionChoice::Promote;
    });
    bool has_decline = std::any_of(matches.begin(), matches.end(), [](Move const& m) {
        return m.promotion == PromotionChoice::Decline;
    });
    if (has_promo && has_decline) {
        pending_move_ = Move::board_move(selected_, to, PromotionChoice::None);
        asking_promote_ = true;
        dragging_ = false;
        lmb_down_ = false;
        status_ = "Click a piece: original or promoted.";
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

void InputController::on_hand_click(Color hand_owner, PieceType t) {
    if (furigoma_visible_) {
        return;
    }
    if (vs_engine_ && !human_to_move()) {
        status_ = engine_thinking_ ? "Engine thinking..." : "Not your turn.";
        return;
    }
    if (asking_promote_ || !game_.can_accept_moves()) {
        if (!game_.at_current_ply() && !game_.allow_review_fork()) {
            status_ = "Not at current ply — press →, or enable takebacks in Menu.";
            clear_selection();
        }
        return;
    }
    if (hand_owner != game_.side_to_move()) {
        status_ = "That is the opponent's hand.";
        return;
    }
    if (game_.board().hand_count(hand_owner, t) <= 0) {
        status_ = "No such piece in hand.";
        return;
    }
    has_selection_ = false;
    has_drop_ = true;
    drop_type_ = t;
    drag_piece_ = Piece{hand_owner, t, false};
    recompute_highlights();
    status_ = std::string("Dropping ") + piece_letter(t) + " — drag or click a square.";
}

void InputController::end_lmb_drag(BoardView const& view, int x, int y) {
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

    if (hit.kind == UiHitKind::Board) {
        if (has_drop_ || has_selection_) {
            if (was_dragging || hit.square != selected_ || has_drop_) {
                try_move_to(hit.square);
            }
            // else: click on selected piece without drag — keep selection
        }
        return;
    }

    // Released off-board while dragging a board piece: cancel pick-up visual, keep selection.
    if (was_dragging && has_selection_ && !has_drop_) {
        recompute_highlights();
    }
}

void InputController::activate_side_button(int index) {
    // Furigoma overlay: Load / New / Menu OK; Resign / Save / Claim wait for dismiss.
    if (furigoma_visible_ && index != 2 && index != 3 && index != 4) {
        status_ = keyboard_board_ ? "Furigoma — Tab to continue."
                                  : "Furigoma — click the board to continue.";
        return;
    }
    auto arm_hint = [&](char const* what) {
        return keyboard_board_ && !board_nav_
                   ? std::string(what) + " armed — Enter/Space again to confirm."
                   : std::string(what) + " armed — click again to confirm.";
    };
    switch (index) {
    case 0: { // Resign — first press arms (red); second confirms
        set_side_focus(0);
        if (!game_.at_current_ply()) {
            clear_confirm_arms();
            status_ = "Not at current ply — press → first.";
            break;
        }
        if (game_.status() != GameStatus::Playing) {
            clear_confirm_arms();
            status_ = game_.message().empty() ? "Game is over." : game_.message();
            break;
        }
        if (!resign_armed_) {
            clear_confirm_arms();
            resign_armed_ = true;
            status_ = arm_hint("Resign");
            break;
        }
        clear_confirm_arms();
        game_.resign();
        clear_selection();
        status_ = game_.message();
        break;
    }
    case 1: { // Save
        clear_confirm_arms();
        std::string path = make_timestamped_kifu_name();
        if (game_.save_kifu(path)) {
            status_ = "Saved to " + path;
        } else {
            status_ = "Save failed (" + path + ").";
        }
        break;
    }
    case 2: { // Load
        clear_confirm_arms();
        std::string path;
        if (!open_kifu_dialog(path)) {
            status_ = "Load cancelled.";
            break;
        }
        if (engine_ && engine_thinking_) {
            engine_->stop_search();
            engine_thinking_ = false;
        }
        if (game_.load_kifu(path)) {
            furigoma_visible_ = false;
            clear_selection();
            refresh_status();
            status_ = "Loaded " + path + (vs_engine_ ? "  [vs engine]" : "");
        } else {
            status_ = "Load failed (" + path + ").";
        }
        break;
    }
    case 3: { // New — confirm like Resign
        set_side_focus(3);
        if (!new_armed_) {
            clear_confirm_arms();
            new_armed_ = true;
            status_ = arm_hint("New game");
            break;
        }
        clear_confirm_arms();
        start_new_game(last_setup_);
        break;
    }
    case 4: // Menu
        clear_confirm_arms();
        open_menu_ = true;
        break;
    case 5: { // Claim impasse
        set_side_focus(5);
        if (!game_.can_claim_jishogi()) {
            clear_confirm_arms();
            status_ = "Impasse claim not available.";
            break;
        }
        if (!claim_armed_) {
            clear_confirm_arms();
            claim_armed_ = true;
            status_ = arm_hint("Claim impasse");
            break;
        }
        clear_confirm_arms();
        if (game_.claim_jishogi()) {
            clear_selection();
            status_ = game_.message();
        }
        break;
    }
    default:
        break;
    }
}

bool InputController::handle_event(SDL_Event const& e, BoardView const& view) {
    if (e.type == SDL_QUIT) {
        return false;
    }
    if (e.type == SDL_KEYDOWN) {
        SDL_Keycode sym = e.key.keysym.sym;
        bool const is_arrow = (sym == SDLK_LEFT || sym == SDLK_RIGHT || sym == SDLK_UP ||
                               sym == SDLK_DOWN);
        // Hold-to-step selection: accept SDL key-repeat for arrows only (throttled).
        if (e.key.repeat && !is_arrow) {
            return true;
        }
        if (is_arrow && !accept_nav_key(e.key.repeat != 0)) {
            return true;
        }
        if (!e.key.repeat && sym == SDLK_ESCAPE) {
            open_menu_ = true;
            return true;
        }
        if (!e.key.repeat && sym == SDLK_TAB) {
            if (keyboard_board_) {
                clear_confirm_arms();
                if (furigoma_visible_) {
                    dismiss_furigoma();
                    board_nav_ = true;
                    hand_nav_ = HandNavFocus::None;
                    board_cursor_ = Square{5, 5};
                    return true;
                }
                board_nav_ = !board_nav_;
                if (board_nav_) {
                    // Reset cursor / selection each time focus enters the board.
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
                    set_side_focus(0); // reset side-menu highlight
                }
            }
            return true;
        }
        if (keyboard_board_ && board_nav_) {
            // Visual: left = higher file, up = lower rank.
            if (sym == SDLK_LEFT) {
                move_board_cursor(1, 0);
                return true;
            }
            if (sym == SDLK_RIGHT) {
                move_board_cursor(-1, 0);
                return true;
            }
            if (sym == SDLK_UP) {
                move_board_cursor(0, -1);
                return true;
            }
            if (sym == SDLK_DOWN) {
                move_board_cursor(0, 1);
                return true;
            }
            if (!e.key.repeat && (sym == SDLK_RETURN || sym == SDLK_SPACE)) {
                activate_board_cursor(view);
                return true;
            }
        } else {
            if (sym == SDLK_LEFT) {
                if (game_.undo()) {
                    clear_selection();
                    refresh_status();
                    if (!game_.message().empty() && !game_.at_current_ply()) {
                        status_ = game_.message();
                    }
                }
                return true;
            }
            if (sym == SDLK_RIGHT) {
                if (game_.redo()) {
                    clear_selection();
                    refresh_status();
                    if (!game_.message().empty() && !game_.at_current_ply()) {
                        status_ = game_.message();
                    }
                }
                return true;
            }
            if (!asking_promote_) {
                if (sym == SDLK_UP) {
                    int const n = side_button_count();
                    set_side_focus((side_focus_ + n - 1) % n);
                    return true;
                }
                if (sym == SDLK_DOWN) {
                    int const n = side_button_count();
                    set_side_focus((side_focus_ + 1) % n);
                    return true;
                }
                if (!e.key.repeat && (sym == SDLK_RETURN || sym == SDLK_SPACE)) {
                    activate_side_button(side_focus_);
                    return true;
                }
            }
        }
    }

    if (keyboard_board_) {
        // Keyboard-only mode: ignore all mouse input.
        if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEWHEEL ||
            e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            return true;
        }
    }

    if (e.type == SDL_MOUSEWHEEL) {
        // GetMouseState is window-space; last motion/button coords are already logical.
        if (view.hit_history_panel(drag_x_, drag_y_)) {
            view.scroll_history(-e.wheel.y);
        }
        return true;
    }

    if (e.type == SDL_MOUSEMOTION) {
        drag_x_ = e.motion.x;
        drag_y_ = e.motion.y;
        if (lmb_down_ && (has_selection_ || has_drop_) && !asking_promote_) {
            int dx = drag_x_ - lmb_down_x_;
            int dy = drag_y_ - lmb_down_y_;
            if (dx * dx + dy * dy >= kDragThresholdPx * kDragThresholdPx) {
                dragging_ = true;
            }
        } else if (!lmb_down_) {
            UiHit hit = view.hit_test(e.motion.x, e.motion.y);
            if (hit.kind == UiHitKind::ButtonResign) {
                set_side_focus(0);
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonSave) {
                set_side_focus(1);
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonLoad) {
                set_side_focus(2);
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonNew) {
                set_side_focus(3);
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonMenu) {
                set_side_focus(4);
                board_nav_ = false;
            } else if (hit.kind == UiHitKind::ButtonClaimImpasse && game_.can_claim_jishogi()) {
                set_side_focus(5);
                board_nav_ = false;
            }
        }
        return true;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
        UiHit hit = view.hit_test(e.button.x, e.button.y);
        if (hit.kind == UiHitKind::Board) {
            rmb_down_ = true;
            rmb_from_ = hit.square;
        }
        return true;
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
        return true;
    }

    if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        end_lmb_drag(view, e.button.x, e.button.y);
        return true;
    }

    if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT) {
        return true;
    }

    marks_.clear();
    drag_x_ = e.button.x;
    drag_y_ = e.button.y;
    lmb_down_x_ = e.button.x;
    lmb_down_y_ = e.button.y;

    UiHit hit = view.hit_test(e.button.x, e.button.y);
    bool const same_confirm_btn =
        hit.kind == UiHitKind::ButtonResign || hit.kind == UiHitKind::ButtonNew ||
        hit.kind == UiHitKind::ButtonClaimImpasse;
    if (!same_confirm_btn) {
        clear_confirm_arms();
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
        return true;
    }

    switch (hit.kind) {
    case UiHitKind::Board: {
        if (furigoma_visible_) {
            if (!keyboard_board_) {
                dismiss_furigoma();
            }
            break;
        }
        if (keyboard_board_) {
            board_nav_ = true;
            board_cursor_ = hit.square;
        }
        if (!game_.can_accept_moves()) {
            if (!game_.at_current_ply()) {
                status_ = game_.allow_review_fork()
                              ? "Cannot move here."
                              : "Not at current ply — press →, or enable takebacks in Menu.";
                clear_selection();
            }
            break;
        }
        if (vs_engine_ && !human_to_move()) {
            status_ = engine_thinking_ ? "Engine thinking..." : "Not your turn.";
            break;
        }
        auto const& slot = game_.board().at(hit.square);
        if (slot.occupied && slot.piece.color == game_.side_to_move()) {
            selected_ = hit.square;
            has_selection_ = true;
            has_drop_ = false;
            drag_piece_ = slot.piece;
            recompute_highlights();
            lmb_down_ = true;
            dragging_ = false;
            status_ = "Selected " + slot.piece.notation_token() + " on " + hit.square.to_string();
        } else if (has_selection_ || has_drop_) {
            // Potential destination — resolve on mouse up (supports click-to-move).
            lmb_down_ = true;
            dragging_ = false;
        } else {
            clear_selection();
        }
        break;
    }
    case UiHitKind::HandSente:
        on_hand_click(Color::Sente, hit.hand_type);
        lmb_down_ = has_drop_;
        dragging_ = false;
        break;
    case UiHitKind::HandGote:
        on_hand_click(Color::Gote, hit.hand_type);
        lmb_down_ = has_drop_;
        dragging_ = false;
        break;
    case UiHitKind::ButtonResign:
        board_nav_ = false;
        set_side_focus(0);
        activate_side_button(0);
        break;
    case UiHitKind::ButtonSave:
        board_nav_ = false;
        set_side_focus(1);
        activate_side_button(1);
        break;
    case UiHitKind::ButtonLoad:
        board_nav_ = false;
        set_side_focus(2);
        activate_side_button(2);
        break;
    case UiHitKind::ButtonNew:
        board_nav_ = false;
        set_side_focus(3);
        activate_side_button(3);
        break;
    case UiHitKind::ButtonMenu:
        board_nav_ = false;
        set_side_focus(4);
        activate_side_button(4);
        break;
    case UiHitKind::ButtonClaimImpasse:
        board_nav_ = false;
        if (game_.can_claim_jishogi()) {
            set_side_focus(5);
            activate_side_button(5);
        } else {
            clear_confirm_arms();
        }
        break;
    default:
        break;
    }
    return true;
}

void InputController::render(SDL_Renderer* renderer, BoardView const& view) const {
    Square const* sel = has_selection_ ? &selected_ : nullptr;
    PieceType const* drop = has_drop_ ? &drop_type_ : nullptr;
    Square const* promo_at = nullptr;
    Square const* promo_from = nullptr;
    Piece promo_piece;
    Piece const* promo_ptr = nullptr;
    if (asking_promote_ && !pending_move_.is_drop) {
        promo_at = &pending_move_.to;
        promo_from = &pending_move_.from;
        if (game_.board().at(pending_move_.from).occupied) {
            promo_piece = game_.board().at(pending_move_.from).piece;
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
    bool const board_focus =
        keyboard_board_ && board_nav_ && !furigoma_visible_;
    bool on_board = board_focus && hand_nav_ == HandNavFocus::None;
    Square const* cursor = on_board ? &board_cursor_ : nullptr;
    int side_draw = board_focus ? -1 : side_focus_;
    bool cursor_green = asking_promote_ && board_focus;
    FurigomaPaint const* furi = furigoma_visible_ ? furigoma_pawns_.data() : nullptr;
    int furi_n = furigoma_visible_ ? 5 : 0;
    HandNavFocus hand_draw = board_focus ? hand_nav_ : HandNavFocus::None;
    bool const can_claim = game_.can_claim_jishogi();
    view.draw(renderer, game_, sel, highlights_, drop, promo_at, promo_from, promo_ptr, &marks_,
              &drag, status_, side_draw, cursor, cursor_green, furi, furi_n, game_.start_side(),
              hand_draw, hand_slot_, resign_armed_, keyboard_board_, new_armed_, can_claim,
              claim_armed_);
}

} // namespace ui
} // namespace shogi
