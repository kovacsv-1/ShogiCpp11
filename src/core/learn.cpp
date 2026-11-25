#include "learn.hpp"
#include "rules.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

namespace shogi {
namespace learn {
namespace {

PieceType hand_letter(char c) {
    switch (std::toupper(static_cast<unsigned char>(c))) {
    case 'P':
        return PieceType::Pawn;
    case 'L':
        return PieceType::Lance;
    case 'N':
        return PieceType::Knight;
    case 'S':
        return PieceType::Silver;
    case 'G':
        return PieceType::Gold;
    case 'B':
        return PieceType::Bishop;
    case 'R':
        return PieceType::Rook;
    default:
        return PieceType::Pawn;
    }
}

bool parse_piece_char(std::string const& token, Piece& out) {
    if (token.empty()) {
        return false;
    }
    bool promoted = false;
    std::size_t i = 0;
    if (token[0] == '+') {
        promoted = true;
        i = 1;
    }
    if (i >= token.size()) {
        return false;
    }
    char ch = token[i];
    Color color = std::isupper(static_cast<unsigned char>(ch)) ? Color::Sente : Color::Gote;
    PieceType t = piece_from_letter(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    out = Piece{color, t, promoted};
    return true;
}

} // namespace

bool parse_usi_square(std::string const& key, Square& out) {
    if (key.size() != 2) {
        return false;
    }
    if (key[0] < '1' || key[0] > '9') {
        return false;
    }
    char rank_ch = static_cast<char>(std::tolower(static_cast<unsigned char>(key[1])));
    if (rank_ch < 'a' || rank_ch > 'i') {
        return false;
    }
    out = Square{key[0] - '0', rank_ch - 'a' + 1};
    return out.valid();
}

bool parse_sfen(std::string const& sfen, Board& board, Color& side_to_move) {
    board.clear();
    std::istringstream in(sfen);
    std::string board_part;
    std::string stm;
    std::string hands;
    if (!(in >> board_part >> stm)) {
        return false;
    }
    if (!(in >> hands)) {
        hands = "-";
    }

    int file = 9;
    int rank = 1; // SFEN first rank = a
    for (std::size_t i = 0; i < board_part.size();) {
        char c = board_part[i];
        if (c == '/') {
            ++rank;
            file = 9;
            ++i;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            file -= (c - '0');
            ++i;
            continue;
        }
        std::string tok;
        if (c == '+') {
            if (i + 1 >= board_part.size()) {
                return false;
            }
            tok.push_back('+');
            tok.push_back(board_part[i + 1]);
            i += 2;
        } else {
            tok.push_back(c);
            ++i;
        }
        Piece p;
        if (!parse_piece_char(tok, p)) {
            return false;
        }
        if (file < 1 || file > 9 || rank < 1 || rank > 9) {
            return false;
        }
        board.place(Square{file, rank}, p);
        --file;
    }

    side_to_move = (stm == "w" || stm == "g") ? Color::Gote : Color::Sente;

    if (hands != "-") {
        int count = 0;
        bool have_count = false;
        for (std::size_t i = 0; i < hands.size(); ++i) {
            char c = hands[i];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                count = count * 10 + (c - '0');
                have_count = true;
                continue;
            }
            int n = have_count ? count : 1;
            have_count = false;
            count = 0;
            Color col = std::isupper(static_cast<unsigned char>(c)) ? Color::Sente : Color::Gote;
            PieceType t = hand_letter(c);
            for (int k = 0; k < n; ++k) {
                board.add_to_hand(col, t);
            }
        }
    }
    return true;
}

std::string to_sfen(Board const& board, Color side_to_move, int move_number) {
    std::ostringstream oss;
    for (int rank = 1; rank <= 9; ++rank) {
        if (rank > 1) {
            oss << '/';
        }
        int empty = 0;
        for (int file = 9; file >= 1; --file) {
            auto const& slot = board.at(Square{file, rank});
            if (!slot.occupied) {
                ++empty;
                continue;
            }
            if (empty > 0) {
                oss << empty;
                empty = 0;
            }
            char ch = piece_letter(slot.piece.type);
            if (slot.piece.color == Color::Gote) {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }
            if (slot.piece.promoted) {
                oss << '+';
            }
            oss << ch;
        }
        if (empty > 0) {
            oss << empty;
        }
    }

    oss << ' ' << (side_to_move == Color::Sente ? 'b' : 'w') << ' ';

    std::ostringstream hands;
    char const* order = "RBGSNLP";
    for (int ci = 0; ci < 2; ++ci) {
        Color c = (ci == 0) ? Color::Sente : Color::Gote;
        for (char const* p = order; *p; ++p) {
            PieceType t = piece_from_letter(*p);
            int n = board.hand_count(c, t);
            if (n <= 0) {
                continue;
            }
            if (n > 1) {
                hands << n;
            }
            char ch = *p;
            if (c == Color::Gote) {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }
            hands << ch;
        }
    }
    std::string hand_str = hands.str();
    oss << (hand_str.empty() ? "-" : hand_str);
    oss << ' ' << (move_number < 1 ? 1 : move_number);
    return oss.str();
}

std::string to_usi_move(Board const& before, Move const& move) {
    (void)before;
    if (move.is_drop) {
        std::string s;
        s.push_back(piece_letter(move.drop_type));
        s.push_back('*');
        s += move.to.to_string();
        return s;
    }
    std::string s = move.from.to_string() + move.to.to_string();
    if (move.promotion == PromotionChoice::Promote) {
        s.push_back('+');
    }
    return s;
}

bool parse_usi_move(std::string const& usi, Board const& board, Color side, Move& out) {
    if (usi.size() < 4) {
        return false;
    }
    if (usi[1] == '*') {
        PieceType t = hand_letter(usi[0]);
        Square to;
        if (!parse_usi_square(usi.substr(2, 2), to)) {
            return false;
        }
        out = Move::drop(t, to);
        return Rules::is_legal(board, side, out);
    }
    Square from;
    Square to;
    if (!parse_usi_square(usi.substr(0, 2), from) || !parse_usi_square(usi.substr(2, 2), to)) {
        return false;
    }
    PromotionChoice promo = PromotionChoice::None;
    Move probe = Move::board_move(from, to);
    if (usi.size() >= 5 && usi[4] == '+') {
        promo = PromotionChoice::Promote;
    } else if (Rules::promotion_forced(board, probe, side)) {
        promo = PromotionChoice::Promote;
    } else if (Rules::promotion_available(board, probe, side)) {
        promo = PromotionChoice::Decline;
    }
    out = Move::board_move(from, to, promo);
    return Rules::is_legal(board, side, out);
}

int score_stars(int nb_moves, int player_moves) {
    double raw = 3.0 - (static_cast<double>(player_moves - nb_moves) / 2.0);
    int s = static_cast<int>(std::ceil(raw));
    if (s < 0) {
        s = 0;
    }
    if (s > 3) {
        s = 3;
    }
    return s;
}

LearnProgress::LearnProgress() {
    ensure_size();
}

void LearnProgress::ensure_size() {
    auto const& stages = all_stages();
    if (best_.size() < stages.size()) {
        best_.resize(stages.size());
    }
    for (std::size_t s = 0; s < stages.size(); ++s) {
        int n = stages[s].level_count;
        if (static_cast<int>(best_[s].size()) < n) {
            best_[s].resize(static_cast<std::size_t>(n), 0);
        }
    }
}

void LearnProgress::load(std::string const& path) {
    ensure_size();
    std::ifstream in(path.c_str());
    if (!in) {
        return;
    }
    std::string key;
    int level = 0;
    int score = 0;
    while (in >> key >> level >> score) {
        for (std::size_t s = 0; s < all_stages().size(); ++s) {
            if (key == all_stages()[s].key) {
                if (level >= 0 && level < static_cast<int>(best_[s].size())) {
                    if (score < 0) {
                        score = 0;
                    }
                    if (score > 3) {
                        score = 3;
                    }
                    best_[s][static_cast<std::size_t>(level)] = score;
                }
                break;
            }
        }
    }
}

void LearnProgress::save(std::string const& path) const {
    std::ofstream out(path.c_str(), std::ios::trunc);
    if (!out) {
        return;
    }
    for (std::size_t s = 0; s < best_.size() && s < all_stages().size(); ++s) {
        for (std::size_t l = 0; l < best_[s].size(); ++l) {
            if (best_[s][l] > 0) {
                out << all_stages()[s].key << ' ' << l << ' ' << best_[s][l] << '\n';
            }
        }
    }
}

int LearnProgress::best(int stage, int level) const {
    if (stage < 0 || stage >= static_cast<int>(best_.size())) {
        return 0;
    }
    if (level < 0 || level >= static_cast<int>(best_[static_cast<std::size_t>(stage)].size())) {
        return 0;
    }
    return best_[static_cast<std::size_t>(stage)][static_cast<std::size_t>(level)];
}

bool LearnProgress::record(int stage, int level, int score) {
    ensure_size();
    if (stage < 0 || stage >= static_cast<int>(best_.size())) {
        return false;
    }
    if (level < 0 || level >= static_cast<int>(best_[static_cast<std::size_t>(stage)].size())) {
        return false;
    }
    if (score < 0) {
        score = 0;
    }
    if (score > 3) {
        score = 3;
    }
    int& slot = best_[static_cast<std::size_t>(stage)][static_cast<std::size_t>(level)];
    if (score <= slot) {
        return false;
    }
    slot = score;
    save();
    return true;
}

int LearnProgress::stage_score(int stage) const {
    if (stage < 0 || stage >= static_cast<int>(best_.size())) {
        return 0;
    }
    int sum = 0;
    for (int v : best_[static_cast<std::size_t>(stage)]) {
        sum += v;
    }
    return sum;
}

int LearnProgress::stage_max(int stage) const {
    if (stage < 0 || stage >= static_cast<int>(all_stages().size())) {
        return 0;
    }
    return all_stages()[static_cast<std::size_t>(stage)].level_count * 3;
}

bool LearnProgress::all_cleared() const {
    auto const& stages = all_stages();
    if (stages.empty()) {
        return false;
    }
    for (std::size_t s = 0; s < stages.size(); ++s) {
        if (s >= best_.size()) {
            return false;
        }
        int n = stages[s].level_count;
        if (static_cast<int>(best_[s].size()) < n) {
            return false;
        }
        for (int l = 0; l < n; ++l) {
            if (best_[s][static_cast<std::size_t>(l)] < 1) {
                return false;
            }
        }
    }
    return true;
}

bool LearnProgress::all_perfect() const {
    auto const& stages = all_stages();
    if (stages.empty()) {
        return false;
    }
    for (std::size_t s = 0; s < stages.size(); ++s) {
        if (s >= best_.size()) {
            return false;
        }
        int n = stages[s].level_count;
        if (static_cast<int>(best_[s].size()) < n) {
            return false;
        }
        for (int l = 0; l < n; ++l) {
            if (best_[s][static_cast<std::size_t>(l)] < 3) {
                return false;
            }
        }
    }
    return true;
}

double LearnProgress::average_best() const {
    auto const& stages = all_stages();
    int levels = 0;
    int sum = 0;
    for (std::size_t s = 0; s < stages.size(); ++s) {
        int n = stages[s].level_count;
        levels += n;
        for (int l = 0; l < n; ++l) {
            sum += best(static_cast<int>(s), l);
        }
    }
    if (levels <= 0) {
        return 0.0;
    }
    return static_cast<double>(sum) / static_cast<double>(levels);
}

LearnSession::LearnSession()
    : side_(Color::Sente),
      stage_i_(0),
      level_i_(0),
      result_(LevelResult::Playing),
      player_moves_(0),
      last_score_(0),
      scenario_i_(0),
      capture_order_i_(0),
      order_bad_(false),
      player_color_(Color::Sente) {}

int LearnSession::stage_count() const {
    return static_cast<int>(all_stages().size());
}

int LearnSession::level_count() const {
    return stage().level_count;
}

LearnStage const& LearnSession::stage() const {
    return all_stages()[static_cast<std::size_t>(stage_i_)];
}

LearnLevel const& LearnSession::level() const {
    return stage().levels[level_i_];
}

void LearnSession::start_stage(int stage_index, int level_index) {
    stage_i_ = std::max(0, std::min(stage_index, stage_count() - 1));
    level_i_ = std::max(0, std::min(level_index, level_count() - 1));
    load_current();
}

void LearnSession::select_stage_browser(int stage_index) {
    start_stage(stage_index, 0);
}

void LearnSession::restart_level() {
    load_current();
}

bool LearnSession::next_level() {
    if (level_i_ + 1 < level_count()) {
        ++level_i_;
        load_current();
        return true;
    }
    if (stage_i_ + 1 < stage_count()) {
        ++stage_i_;
        level_i_ = 0;
        load_current();
        return true;
    }
    return false;
}

bool LearnSession::prev_level() {
    if (level_i_ > 0) {
        --level_i_;
        load_current();
        return true;
    }
    if (stage_i_ > 0) {
        --stage_i_;
        level_i_ = level_count() - 1;
        load_current();
        return true;
    }
    return false;
}

void LearnSession::load_current() {
    LearnLevel const& lv = level();
    if (!parse_sfen(lv.sfen, board_, side_)) {
        message_ = "Invalid SFEN";
        result_ = LevelResult::Failure;
        return;
    }
    player_color_ = Color::Sente;
    result_ = LevelResult::Playing;
    player_moves_ = 0;
    last_score_ = 0;
    scenario_i_ = 0;
    capture_order_i_ = 0;
    order_bad_ = false;
    obstacle_hit_.assign(static_cast<std::size_t>(lv.obstacle_count), false);
    // Lishogi places temporary opponent pawns on obstacle squares (stars).
    for (int i = 0; i < lv.obstacle_count; ++i) {
        Square s;
        if (!parse_usi_square(lv.obstacles[i], s)) {
            continue;
        }
        if (!board_.at(s).occupied) {
            board_.place(s, Piece{Color::Gote, PieceType::Pawn, false});
        }
    }
    message_ = lv.goal;
}

std::vector<Square> LearnSession::remaining_obstacles() const {
    std::vector<Square> out;
    LearnLevel const& lv = level();
    for (int i = 0; i < lv.obstacle_count; ++i) {
        if (obstacle_hit_[static_cast<std::size_t>(i)]) {
            continue;
        }
        Square s;
        if (parse_usi_square(lv.obstacles[i], s)) {
            out.push_back(s);
        }
    }
    return out;
}

void LearnSession::mark_obstacle(Square to) {
    LearnLevel const& lv = level();
    for (int i = 0; i < lv.obstacle_count; ++i) {
        if (obstacle_hit_[static_cast<std::size_t>(i)]) {
            continue;
        }
        Square s;
        if (parse_usi_square(lv.obstacles[i], s) && s == to) {
            obstacle_hit_[static_cast<std::size_t>(i)] = true;
        }
    }
}

bool LearnSession::no_pieces(Color c) const {
    for (int i = 0; i < 81; ++i) {
        PieceSlot const& slot = board_.at(Square::from_index(i));
        if (slot.occupied && slot.piece.color == c) {
            return false;
        }
    }
    return true;
}

bool LearnSession::opponent_has_capture() const {
    Color opp = opposite(player_color_);
    auto moves = Rules::generate_legal(board_, opp);
    for (Move const& m : moves) {
        if (!m.is_drop && board_.at(m.to).occupied && board_.at(m.to).piece.color == player_color_) {
            return true;
        }
    }
    return false;
}

bool LearnSession::player_can_recapture(Move capture) const {
    // A friendly piece does not "attack" a square its own piece occupies, so
    // square_attacked before the capture never sees a defender. Lishogi asks
    // whether, after the capture, the learner has a legal recapture there.
    Board next = board_;
    Color opp = opposite(player_color_);
    next.apply(capture, opp);
    std::vector<Move> replies = Rules::generate_legal(next, player_color_);
    for (std::size_t i = 0; i < replies.size(); ++i) {
        if (!replies[i].is_drop && replies[i].to == capture.to) {
            return true;
        }
    }
    return false;
}

bool LearnSession::opponent_has_unprotected_capture() const {
    Color opp = opposite(player_color_);
    std::vector<Move> moves = Rules::generate_legal(board_, opp);
    for (std::size_t i = 0; i < moves.size(); ++i) {
        Move const& m = moves[i];
        if (m.is_drop || !board_.at(m.to).occupied) {
            continue;
        }
        if (board_.at(m.to).piece.color != player_color_) {
            continue;
        }
        if (!player_can_recapture(m)) {
            return true;
        }
    }
    return false;
}

void LearnSession::play_failure_capture() {
    LearnLevel const& lv = level();
    if (lv.failure != FailureKind::AnyCapture && lv.failure != FailureKind::UnprotectedCapture) {
        return;
    }
    Color opp = opposite(player_color_);
    std::vector<Move> moves = Rules::generate_legal(board_, opp);
    for (std::size_t i = 0; i < moves.size(); ++i) {
        Move m = moves[i];
        if (m.is_drop || !board_.at(m.to).occupied) {
            continue;
        }
        if (board_.at(m.to).piece.color != player_color_) {
            continue;
        }
        if (lv.failure == FailureKind::UnprotectedCapture && player_can_recapture(m)) {
            continue;
        }
        // Lishogi shows the punishing capture unless it would take the king.
        if (board_.at(m.to).piece.type == PieceType::King) {
            return;
        }
        board_.apply(m, opp);
        side_ = player_color_;
        return;
    }
}

void LearnSession::note_capture_order(Square to) {
    LearnLevel const& lv = level();
    if (lv.failure != FailureKind::CaptureOrder || order_bad_) {
        return;
    }
    if (lv.capture_order == nullptr || capture_order_i_ >= lv.capture_order_count) {
        return;
    }
    Square next;
    if (!parse_usi_square(lv.capture_order[capture_order_i_], next)) {
        return;
    }
    if (to == next) {
        ++capture_order_i_;
        return;
    }
    // Landing on a later target before the earlier ones is the wrong order.
    for (int i = capture_order_i_ + 1; i < lv.capture_order_count; ++i) {
        Square later;
        if (parse_usi_square(lv.capture_order[i], later) && later == to) {
            order_bad_ = true;
            return;
        }
    }
}

bool LearnSession::evaluate_success() const {
    LearnLevel const& lv = level();
    switch (lv.success) {
    case SuccessKind::Always:
        return player_moves_ >= 1;
    case SuccessKind::Obstacles:
        return std::find(obstacle_hit_.begin(), obstacle_hit_.end(), false) == obstacle_hit_.end() &&
               !obstacle_hit_.empty();
    case SuccessKind::ExtinctGote:
        return no_pieces(Color::Gote);
    case SuccessKind::CheckGote:
        return Rules::in_check(board_, Color::Gote);
    case SuccessKind::Checkmate:
        return Rules::is_checkmate(board_, Color::Gote);
    case SuccessKind::NotInCheck:
        return !Rules::in_check(board_, player_color_);
    case SuccessKind::NoOpponentCapture:
        return player_moves_ >= 1 && !opponent_has_capture();
    case SuccessKind::NoUnprotectedCapture:
        return player_moves_ >= 1 && !opponent_has_unprotected_capture();
    case SuccessKind::ColorOn: {
        Square s;
        if (!parse_usi_square(lv.target_sq, s)) {
            return false;
        }
        PieceSlot const& slot = board_.at(s);
        return slot.occupied && slot.piece.color == lv.piece_color;
    }
    case SuccessKind::PieceOn: {
        Square s;
        if (!parse_usi_square(lv.target_sq, s)) {
            return false;
        }
        PieceSlot const& slot = board_.at(s);
        return slot.occupied && slot.piece.color == lv.piece_color &&
               slot.piece.type == lv.piece_type && slot.piece.promoted == lv.piece_promoted;
    }
    case SuccessKind::Scenario:
        return scenario_i_ >= lv.scenario_count && lv.scenario_count > 0;
    }
    return false;
}

bool LearnSession::evaluate_failure() const {
    LearnLevel const& lv = level();
    switch (lv.failure) {
    case FailureKind::None:
        return false;
    case FailureKind::AnyCapture:
        return opponent_has_capture();
    case FailureKind::UnprotectedCapture:
        return opponent_has_unprotected_capture();
    case FailureKind::Scenario:
        // Checked during scenario follow.
        return false;
    case FailureKind::IfNotSuccess:
        // Fail immediately when move didn't achieve success (one-shot puzzles).
        return !evaluate_success();
    case FailureKind::CaptureOrder:
        return order_bad_;
    }
    return false;
}

void LearnSession::maybe_opponent_reply() {
    LearnLevel const& lv = level();
    Color opp = opposite(player_color_);

    if (lv.success == SuccessKind::Scenario && scenario_i_ < lv.scenario_count) {
        ScenarioStep const& step = lv.scenario[scenario_i_];
        if (step.color != opp) {
            return;
        }
        Move m;
        if (parse_usi_move(step.usi, board_, opp, m)) {
            board_.apply(m, opp);
            ++scenario_i_;
            side_ = player_color_;
        }
        return;
    }

    // Multi-move checkmate: opponent replies (prefer capturing/checking defense).
    // Capture/protection puzzles only *threat-check* the opponent; they do not auto-move.
    if (lv.success != SuccessKind::Checkmate || no_pieces(opp)) {
        side_ = player_color_;
        return;
    }

    auto moves = Rules::generate_legal(board_, opp);
    if (moves.empty()) {
        side_ = player_color_;
        return;
    }
    Move chosen = moves.front();
    for (Move const& m : moves) {
        if (!m.is_drop && board_.at(m.to).occupied && board_.at(m.to).piece.color == player_color_) {
            chosen = m;
            break;
        }
    }
    board_.apply(chosen, opp);
    side_ = player_color_;
}

bool LearnSession::try_player_move(Move m) {
    if (result_ != LevelResult::Playing) {
        return false;
    }
    if (side_ != player_color_) {
        return false;
    }
    if (!Rules::is_legal(board_, side_, m)) {
        // UI may pass PromotionChoice::None; legal moves use Promote/Decline in the zone.
        if (!m.is_drop && m.promotion == PromotionChoice::None) {
            Move decline = m;
            decline.promotion = PromotionChoice::Decline;
            Move promote = m;
            promote.promotion = PromotionChoice::Promote;
            if (Rules::is_legal(board_, side_, decline)) {
                m = decline;
            } else if (Rules::is_legal(board_, side_, promote)) {
                m = promote;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    LearnLevel const& lv = level();
    if (lv.success == SuccessKind::Scenario && scenario_i_ < lv.scenario_count) {
        ScenarioStep const& step = lv.scenario[scenario_i_];
        if (step.color == player_color_) {
            Move expected;
            if (!parse_usi_move(step.usi, board_, player_color_, expected) || !(expected == m)) {
                // Allow promotion mismatch if expected promotes.
                bool ok = false;
                if (parse_usi_move(step.usi, board_, player_color_, expected)) {
                    Move alt = m;
                    if (expected.promotion == PromotionChoice::Promote) {
                        alt.promotion = PromotionChoice::Promote;
                    }
                    ok = (expected == alt);
                    if (ok) {
                        m = alt;
                    }
                }
                if (!ok) {
                    result_ = LevelResult::Failure;
                    message_ = "Wrong move — press R to retry";
                    return true;
                }
            }
            ++scenario_i_;
        }
    }

    board_.apply(m, side_);
    ++player_moves_;
    mark_obstacle(m.to);
    note_capture_order(m.to);
    side_ = opposite(side_);

    if (evaluate_success()) {
        result_ = LevelResult::Success;
        last_score_ = score_stars(lv.nb_moves, player_moves_);
        message_ = "Success! Score: " + std::to_string(last_score_) + "/3  (N=next, R=retry)";
        return true;
    }

    if (evaluate_failure()) {
        play_failure_capture();
        result_ = LevelResult::Failure;
        message_ = "Failed — press R to retry";
        return true;
    }

    maybe_opponent_reply();

    if (evaluate_success()) {
        result_ = LevelResult::Success;
        last_score_ = score_stars(lv.nb_moves, player_moves_);
        message_ = "Success! Score: " + std::to_string(last_score_) + "/3  (N=next, R=retry)";
        return true;
    }
    if (evaluate_failure()) {
        play_failure_capture();
        result_ = LevelResult::Failure;
        message_ = "Failed — press R to retry";
        return true;
    }

    message_ = std::string(lv.goal) + "  (" + std::to_string(player_moves_) + "/" +
               std::to_string(lv.nb_moves) + " moves)";
    return true;
}

} // namespace learn
} // namespace shogi
