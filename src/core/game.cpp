#include "game.hpp"

#include "notation.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace shogi {

Game::Game()
    : allow_review_fork_(false),
      hard_mode_(false),
      clock_enabled_(false),
      clock_main_ms_(0),
      clock_inc_ms_(0),
      clock_sente_ms_(0),
      clock_gote_ms_(0),
      handicap_() {
    bind_default_move_gen();
    reset();
}

void Game::bind_default_move_gen() {
    if (hard_mode_) {
        move_gen_ = [](Board const& b, Color c) {
            return Rules::generate_pseudolegal(b, c);
        };
    } else {
        move_gen_ = [](Board const& b, Color c) {
            return Rules::generate_legal(b, c);
        };
    }
}

void Game::set_hard_mode(bool on) {
    hard_mode_ = on;
    bind_default_move_gen();
}

void Game::set_move_generator(MoveGenerator gen) {
    if (gen) {
        move_gen_ = std::move(gen);
    } else {
        bind_default_move_gen();
    }
}

bool Game::can_accept_moves() const {
    // Earlier ply than history tip: only with fork option
    if (view_ply_ < history_.size()) {
        return allow_review_fork_;
    }
    // Tip of history (redo may still hold undone moves — playing discards them)
    if (resigned_ || status_ != GameStatus::Playing) {
        return false;
    }
    return true;
}

void Game::truncate_future() {
    if (view_ply_ < history_.size()) {
        history_.resize(view_ply_);
    }
    redo_.clear();
    resigned_ = false;
}

void Game::reset() {
    reset_starting(Color::Sente);
}

void Game::reset_starting(Color first) {
    board_ = Board::starting_position();
    start_side_ = first;
    side_ = first;
    status_ = GameStatus::Playing;
    winner_ = Color::Sente;
    history_.clear();
    redo_.clear();
    view_ply_ = 0;
    repetition_.clear();
    message_.clear();
    resigned_ = false;
    handicap_ = HandicapSettings();
    // allow_review_fork_ / hard_mode_ / clock prefs kept unless caller clears
    record_position();
}

void Game::reset_handicap(HandicapSettings const& hc) {
    handicap_ = hc;
    Color first = Color::Sente;
    board_ = make_handicap_position(hc, first);
    start_side_ = first;
    side_ = first;
    status_ = GameStatus::Playing;
    winner_ = Color::Sente;
    history_.clear();
    redo_.clear();
    view_ply_ = 0;
    repetition_.clear();
    message_.clear();
    resigned_ = false;
    record_position();
}

void Game::configure_clock(bool enabled, int main_minutes, int increment_seconds) {
    if (main_minutes < 0) {
        main_minutes = 0;
    }
    if (main_minutes > 180) {
        main_minutes = 180;
    }
    if (increment_seconds < 0) {
        increment_seconds = 0;
    }
    if (increment_seconds > 180) {
        increment_seconds = 180;
    }
    clock_enabled_ = enabled;
    clock_main_ms_ = main_minutes * 60 * 1000;
    clock_inc_ms_ = increment_seconds * 1000;
    clock_sente_ms_ = clock_main_ms_;
    clock_gote_ms_ = clock_main_ms_;
}

void Game::clear_clock() {
    clock_enabled_ = false;
    clock_main_ms_ = 0;
    clock_inc_ms_ = 0;
    clock_sente_ms_ = 0;
    clock_gote_ms_ = 0;
}

void Game::flag_timeout(Color loser) {
    set_result(GameStatus::TimeForfeit, opposite(loser),
               loser == Color::Sente ? "Sente flags. Gote wins." : "Gote flags. Sente wins.");
}

void Game::tick_clock(int dt_ms) {
    if (!clock_enabled_ || status_ != GameStatus::Playing || !at_current_ply() || dt_ms <= 0) {
        return;
    }
    int& rem = (side_ == Color::Sente) ? clock_sente_ms_ : clock_gote_ms_;
    if (rem <= 0) {
        flag_timeout(side_);
        return;
    }
    rem -= dt_ms;
    if (rem <= 0) {
        rem = 0;
        flag_timeout(side_);
    }
}

void Game::apply_clock_increment(Color mover) {
    if (!clock_enabled_ || clock_inc_ms_ <= 0) {
        return;
    }
    if (mover == Color::Sente) {
        clock_sente_ms_ += clock_inc_ms_;
    } else {
        clock_gote_ms_ += clock_inc_ms_;
    }
}

void Game::set_position(Board board, Color first) {
    board_ = std::move(board);
    start_side_ = first;
    side_ = first;
    status_ = GameStatus::Playing;
    winner_ = Color::Sente;
    history_.clear();
    redo_.clear();
    view_ply_ = 0;
    repetition_.clear();
    message_.clear();
    resigned_ = false;
    handicap_ = HandicapSettings();
    record_position();
    update_end_conditions(); // mate / repetition; jishogi is manual claim
}

Color Game::furigoma_starter(int promoted_count, int total) {
    if (total <= 0) {
        return Color::Sente;
    }
    return (promoted_count * 2 > total) ? Color::Gote : Color::Sente;
}

void Game::record_position() {
    std::string key = board_.position_key(side_);
    repetition_[key] += 1;
}

void Game::set_result(GameStatus st, Color win, std::string msg) {
    status_ = st;
    winner_ = win;
    message_ = std::move(msg);
}

void Game::show_ply(std::size_t ply) {
    if (ply > history_.size()) {
        ply = history_.size();
    }

    // Replay from the real start (komaochi), not a flat hirate board.
    if (handicap_.enabled) {
        Color first = start_side_;
        board_ = make_handicap_position(handicap_, first);
        start_side_ = first;
    } else {
        board_ = Board::starting_position();
    }
    side_ = start_side_;
    status_ = GameStatus::Playing;
    winner_ = Color::Sente;
    repetition_.clear();
    record_position();

    for (std::size_t i = 0; i < ply; ++i) {
        Move m = history_[i].move;
        board_.apply(m, side_);
        side_ = opposite(side_);
        record_position();
    }

    view_ply_ = ply;

    if (ply < history_.size()) {
        if (allow_review_fork_) {
            message_ = "Review ply " + std::to_string(view_ply_) + "/" +
                       std::to_string(history_.size()) +
                       " — takebacks on: a move discards later plies.";
        } else {
            message_ = "Review ply " + std::to_string(view_ply_) + "/" +
                       std::to_string(history_.size()) +
                       " — moves disabled. → to return to current.";
        }
        return;
    }

    if (resigned_ && redo_.empty()) {
        Color loser = side_;
        set_result(GameStatus::Resignation, opposite(loser),
                   loser == Color::Sente ? "Sente resigns. Gote wins." : "Gote resigns. Sente wins.");
        return;
    }
    update_end_conditions();
    if (!redo_.empty() && status_ == GameStatus::Playing) {
        message_ = "← undo / → redo. A new move discards undone line.";
    }
}

void Game::update_end_conditions() {
    status_ = GameStatus::Playing;
    message_.clear();

    std::string key = board_.position_key(side_);
    int count = repetition_[key];

    if (count >= 4) {
        bool perpetual = false;
        if (history_.size() >= 4) {
            Color checker = opposite(side_);
            int needed = 0;
            int checks = 0;
            for (int i = static_cast<int>(history_.size()) - 1; i >= 0 && needed < 4; --i) {
                Color mover = (i % 2 == 0) ? start_side_ : opposite(start_side_);
                if (mover == checker) {
                    ++needed;
                    if (history_[static_cast<std::size_t>(i)].gave_check) {
                        ++checks;
                    }
                }
            }
            if (needed >= 4 && checks == needed) {
                perpetual = true;
            }
        }

        if (perpetual) {
            set_result(GameStatus::DrawPerpetualCheck, side_,
                       "Perpetual check: checker loses.");
        } else {
            set_result(GameStatus::DrawRepetition, Color::Sente, "Sennichite (draw by repetition).");
        }
        return;
    }

    // Hard mode ends by king capture / illegal, not tsume.
    // Jishogi is manual (@ref claim_jishogi) when both kings have entered.
    if (!hard_mode_ && Rules::is_checkmate(board_, side_)) {
        set_result(GameStatus::Checkmate, opposite(side_),
                   side_ == Color::Sente ? "Gote wins by checkmate." : "Sente wins by checkmate.");
    }
}

bool Game::can_claim_jishogi() const {
    return status_ == GameStatus::Playing && at_current_ply() &&
           Rules::both_kings_entered(board_);
}

void Game::apply_jishogi_result() {
    int sp = Rules::jishogi_points(board_, Color::Sente);
    int gp = Rules::jishogi_points(board_, Color::Gote);
    char buf[128];
    if (sp >= 24 && gp >= 24) {
        std::snprintf(buf, sizeof(buf), "Jishogi draw (Sente %d — Gote %d).", sp, gp);
        set_result(GameStatus::JishogiDraw, Color::Sente, buf);
    } else if (sp < 24 && gp < 24) {
        if (sp == gp) {
            std::snprintf(buf, sizeof(buf), "Jishogi draw (Sente %d — Gote %d).", sp, gp);
            set_result(GameStatus::JishogiDraw, Color::Sente, buf);
        } else if (sp > gp) {
            std::snprintf(buf, sizeof(buf), "Jishogi: Sente wins (%d — %d).", sp, gp);
            set_result(GameStatus::JishogiWin, Color::Sente, buf);
        } else {
            std::snprintf(buf, sizeof(buf), "Jishogi: Gote wins (%d — %d).", sp, gp);
            set_result(GameStatus::JishogiWin, Color::Gote, buf);
        }
    } else if (sp < 24) {
        std::snprintf(buf, sizeof(buf), "Jishogi: Gote wins (%d — %d).", sp, gp);
        set_result(GameStatus::JishogiWin, Color::Gote, buf);
    } else {
        std::snprintf(buf, sizeof(buf), "Jishogi: Sente wins (%d — %d).", sp, gp);
        set_result(GameStatus::JishogiWin, Color::Sente, buf);
    }
}

bool Game::claim_jishogi() {
    if (!can_claim_jishogi()) {
        return false;
    }
    apply_jishogi_result();
    return true;
}

std::vector<Move> Game::legal_moves() const {
    if (!can_accept_moves()) {
        return {};
    }
    return move_gen_(board_, side_);
}

void Game::lose_by_illegal() {
    if (!can_accept_moves()) {
        return;
    }
    Color loser = side_;
    set_result(GameStatus::IllegalMove, opposite(loser),
               loser == Color::Sente ? "Illegal move — Sente loses."
                                    : "Illegal move — Gote loses.");
}

bool Game::try_play(Move move) {
    if (!can_accept_moves()) {
        if (!at_current_ply() && !allow_review_fork_) {
            message_ = "Not at the current position. Press →, or enable takebacks in Menu.";
        } else {
            message_ = "Game is over.";
        }
        return false;
    }

    // Fork: drop all later history / redo, keep viewed ply as the new tip
    if (!at_current_ply() || !redo_.empty()) {
        truncate_future();
        // Rebuild repetition counts for the truncated line tip
        show_ply(history_.size());
    }

    auto legal = legal_moves();
    auto it = std::find_if(legal.begin(), legal.end(), [&](Move const& m) {
        return m == move;
    });
    if (it == legal.end()) {
        if (hard_mode_) {
            lose_by_illegal();
            return true;
        }
        message_ = "Illegal move.";
        return false;
    }
    move = *it;

    Piece moved_before = move.is_drop ? Piece{side_, move.drop_type, false}
                                      : board_.at(move.from).piece;

    // Notation before mutate — no Board snapshot copy needed (ea08).
    std::string nota = Notation::to_western(board_, side_, move);
    Color mover = side_;
    board_.apply(move, side_);
    bool gave_check = Rules::in_check(board_, opposite(side_));

    HistoryEntry entry;
    entry.move = move;
    entry.notation = std::move(nota);
    entry.moved_before = moved_before;
    entry.gave_check = gave_check;
    history_.push_back(entry);
    redo_.clear();
    view_ply_ = history_.size();
    resigned_ = false;

    if (hard_mode_ && move.is_capture && move.captured.type == PieceType::King) {
        set_result(GameStatus::KingCaptured, mover,
                   mover == Color::Sente ? "Sente captures the king." : "Gote captures the king.");
        return true;
    }

    apply_clock_increment(mover);
    side_ = opposite(side_);
    message_.clear();
    record_position();
    update_end_conditions();
    return true;
}

bool Game::undo() {
    if (view_ply_ == 0) {
        return false;
    }

    // At tip with nothing already undone: take-back last move into redo.
    if (view_ply_ == history_.size() && redo_.empty() && !history_.empty()) {
        redo_.push_back(history_.back());
        history_.pop_back();
        resigned_ = false;
        show_ply(history_.size());
        return true;
    }

    // Further ← : review earlier plies (moves stay disabled until back at tip).
    show_ply(view_ply_ - 1);
    return true;
}

bool Game::redo() {
    // First catch up to tip of history, then restore from redo stack.
    if (view_ply_ < history_.size()) {
        show_ply(view_ply_ + 1);
        return true;
    }
    if (!redo_.empty()) {
        history_.push_back(redo_.back());
        redo_.pop_back();
        show_ply(history_.size());
        return true;
    }
    return false;
}

void Game::resign() {
    if (!at_current_ply() || status_ != GameStatus::Playing) {
        return;
    }
    resigned_ = true;
    Color win = opposite(side_);
    set_result(GameStatus::Resignation, win,
               side_ == Color::Sente ? "Sente resigns. Gote wins." : "Gote resigns. Sente wins.");
}

bool Game::save_kifu(std::string const& path) const {
    std::ofstream out(path.c_str());
    if (!out) {
        return false;
    }
    out << Notation::game_header();
    out << "# start_side: " << (start_side_ == Color::Sente ? "sente" : "gote") << '\n';
    if (handicap_.enabled) {
        out << "# handicap: 1\n";
        out << "# handicap_uwate: " << (handicap_.uwate == Color::Sente ? "sente" : "gote")
            << '\n';
        out << "# handicap_rank: " << handicap_.rank_diff << '\n';
    } else {
        out << "# handicap: 0\n";
    }
    if (clock_enabled_) {
        out << "# clock: 1\n";
        out << "# clock_main_ms: " << clock_main_ms_ << '\n';
        out << "# clock_inc_ms: " << clock_inc_ms_ << '\n';
        out << "# clock_sente_ms: " << clock_sente_ms_ << '\n';
        out << "# clock_gote_ms: " << clock_gote_ms_ << '\n';
    } else {
        out << "# clock: 0\n";
    }
    for (std::size_t i = 0; i < history_.size(); ++i) {
        out << (i + 1) << ". " << history_[i].notation << '\n';
    }
    if (status_ != GameStatus::Playing && at_current_ply()) {
        out << "# result: " << message_ << '\n';
    }
    return static_cast<bool>(out);
}

bool Game::load_kifu(std::string const& path) {
    std::ifstream in(path.c_str());
    if (!in) {
        return false;
    }

    HandicapSettings hc;
    bool clock_on = false;
    int main_ms = 0;
    int inc_ms = 0;
    int sente_ms = -1;
    int gote_ms = -1;
    Color start = Color::Sente;
    bool have_start = false;

    std::vector<std::string> move_lines;
    std::string line;
    while (std::getline(in, line)) {
        // Strip CR from Windows CRLF so tokens parse cleanly.
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        if (line[0] == '#' || line.find("Shogi") != std::string::npos) {
            // Meta tags: "# key: value" (prefix match — do not use fixed compare lengths).
            if (line.size() > 2 && line[0] == '#') {
                std::istringstream ls(line.substr(1));
                std::string key;
                std::string val;
                if ((ls >> key) && (ls >> val)) {
                    if (key == "handicap:") {
                        hc.enabled = (val != "0");
                    } else if (key == "handicap_uwate:") {
                        hc.uwate = (val == "sente") ? Color::Sente : Color::Gote;
                    } else if (key == "handicap_rank:") {
                        hc.rank_diff = std::atoi(val.c_str());
                        if (hc.rank_diff < 1) {
                            hc.rank_diff = 1;
                        }
                    } else if (key == "clock:") {
                        clock_on = (val != "0");
                    } else if (key == "clock_main_ms:") {
                        main_ms = std::atoi(val.c_str());
                    } else if (key == "clock_inc_ms:") {
                        inc_ms = std::atoi(val.c_str());
                    } else if (key == "clock_sente_ms:") {
                        sente_ms = std::atoi(val.c_str());
                    } else if (key == "clock_gote_ms:") {
                        gote_ms = std::atoi(val.c_str());
                    } else if (key == "start_side:") {
                        start = (val == "gote") ? Color::Gote : Color::Sente;
                        have_start = true;
                    }
                }
            }
            continue;
        }
        move_lines.push_back(line);
    }

    clear_clock();
    if (hc.enabled) {
        reset_handicap(hc);
    } else if (have_start) {
        reset_starting(start);
    } else {
        reset();
    }

    if (clock_on) {
        int main_min = main_ms / 60000;
        int inc_sec = inc_ms / 1000;
        configure_clock(true, main_min, inc_sec);
        // Restore exact remaining if present (after moves we will re-apply by replay without ticking).
        if (sente_ms >= 0) {
            clock_sente_ms_ = sente_ms;
        }
        if (gote_ms >= 0) {
            clock_gote_ms_ = gote_ms;
        }
        // Replay without consuming clock: temporarily disable.
        clock_enabled_ = false;
    }

    for (std::size_t li = 0; li < move_lines.size(); ++li) {
        std::string const& ml = move_lines[li];
        std::size_t pos = 0;
        while (pos < ml.size() && std::isdigit(static_cast<unsigned char>(ml[pos]))) {
            ++pos;
        }
        if (pos < ml.size() && ml[pos] == '.') {
            ++pos;
        }
        while (pos < ml.size() && std::isspace(static_cast<unsigned char>(ml[pos]))) {
            ++pos;
        }
        std::string move_text = ml.substr(pos);
        Move m;
        if (!Notation::parse_western(board_, side_, move_text, m)) {
            reset();
            clear_clock();
            message_ = "Failed to parse kifu.";
            return false;
        }
        if (!try_play(m)) {
            reset();
            clear_clock();
            message_ = "Illegal move in kifu.";
            return false;
        }
        if (status_ != GameStatus::Playing) {
            break;
        }
    }

    if (clock_on) {
        clock_enabled_ = true;
        clock_main_ms_ = main_ms;
        clock_inc_ms_ = inc_ms;
        if (sente_ms >= 0) {
            clock_sente_ms_ = sente_ms;
        }
        if (gote_ms >= 0) {
            clock_gote_ms_ = gote_ms;
        }
    }

    message_ = "Loaded " + path;
    return true;
}

} // namespace shogi
