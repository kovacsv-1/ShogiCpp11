#ifndef SHOGI_LEARN_HPP
#define SHOGI_LEARN_HPP

/**
 * @file learn.hpp
 * @brief Tutorial curriculum, SFEN/USI helpers, progress persistence.
 */

#include "board.hpp"
#include "types.hpp"

#include <string>
#include <vector>

namespace shogi {
namespace learn {

/** How a level decides the player succeeded. */
enum class SuccessKind {
    Always,
    Obstacles,
    ExtinctGote,
    CheckGote,
    Checkmate,
    NotInCheck,
    NoOpponentCapture,
    NoUnprotectedCapture,
    ColorOn,
    PieceOn,
    Scenario
};

/** How a level decides the player failed. */
enum class FailureKind {
    None,
    AnyCapture,
    UnprotectedCapture,
    Scenario,
    IfNotSuccess
};

/** One forced USI ply in a scenario level. */
struct ScenarioStep {
    char const* usi;
    Color color;
};

/** Static description of one tutorial level (Lishogi-style data). */
struct LearnLevel {
    char const* goal;
    char const* sfen;
    int nb_moves;
    SuccessKind success;
    FailureKind failure;
    char const* const* obstacles; ///< USI keys e.g. @c "5d"
    int obstacle_count;
    ScenarioStep const* scenario;
    int scenario_count;
    char const* target_sq; ///< For ColorOn / PieceOn
    PieceType piece_type;
    Color piece_color;
    bool piece_promoted;
};

/** A named stage containing several @ref LearnLevel entries. */
struct LearnStage {
    char const* key;
    char const* title;
    char const* subtitle;
    LearnLevel const* levels;
    int level_count;
};

/** @return Full static curriculum. */
std::vector<LearnStage> const& all_stages();

/** Parse SFEN into board + side to move. @return False on error. */
bool parse_sfen(std::string const& sfen, Board& board, Color& side_to_move);

/** Serialize board + side to SFEN (for USI @c position sfen ...). */
std::string to_sfen(Board const& board, Color side_to_move, int move_number = 1);

/** Parse USI square like @c "5d" into @ref Square. */
bool parse_usi_square(std::string const& key, Square& out);

/** Parse USI move (e.g. @c "5g5f", @c "5g5f+", @c "P*5e") into @ref Move. */
bool parse_usi_move(std::string const& usi, Board const& board, Color side, Move& out);

/** Format a move as USI (@c 7g7f, @c 7g7f+, @c P*5e). */
std::string to_usi_move(Board const& before, Move const& move);

/** Score 0..3 like Lishogi: @c ceil(3 - (moves - nbMoves) / 2). */
int score_stars(int nb_moves, int player_moves);

enum class LevelResult { Playing, Success, Failure };

/** Best stars per level; persisted to @c learn_scores.txt. */
class LearnProgress {
public:
    LearnProgress();

    void load(std::string const& path = "learn_scores.txt");
    void save(std::string const& path = "learn_scores.txt") const;

    /** Best score for a level (0 if never cleared). */
    int best(int stage, int level) const;

    /** Update if @p score is higher; returns true if stored and saved. */
    bool record(int stage, int level, int score);

    /** Sum of best scores in a stage. */
    int stage_score(int stage) const;

    /** Max possible stars in a stage (@c level_count * 3). */
    int stage_max(int stage) const;

    /** True if every level in every stage has been cleared (best ≥ 1). */
    bool all_cleared() const;

    /** True if every level in every stage has best score 3. */
    bool all_perfect() const;

    /** Mean best stars per level across all stages (0..3). Empty curriculum → 0. */
    double average_best() const;

private:
    std::vector<std::vector<int> > best_;
    void ensure_size();
};

/** Runtime state machine for playing one tutorial level. */
class LearnSession {
public:
    LearnSession();

    void start_stage(int stage_index, int level_index = 0);
    void restart_level();
    /** @return False if the stage has no further level. */
    bool next_level();
    bool prev_level();

    LearnStage const& stage() const;
    LearnLevel const& level() const;
    int stage_index() const { return stage_i_; }
    int level_index() const { return level_i_; }
    int stage_count() const;
    int level_count() const;

    Board const& board() const { return board_; }
    Color side_to_move() const { return side_; }
    LevelResult result() const { return result_; }
    int player_moves() const { return player_moves_; }
    int last_score() const { return last_score_; }
    std::string const& message() const { return message_; }

    /** Remaining obstacle squares. */
    std::vector<Square> remaining_obstacles() const;

    bool try_player_move(Move m);
    void select_stage_browser(int stage_index);

private:
    Board board_;
    Color side_;
    int stage_i_;
    int level_i_;
    LevelResult result_;
    int player_moves_;
    int last_score_;
    std::vector<bool> obstacle_hit_;
    int scenario_i_;
    std::string message_;
    Color player_color_;

    void load_current();
    bool evaluate_success() const;
    bool evaluate_failure() const;
    bool opponent_has_capture() const;
    bool opponent_has_unprotected_capture() const;
    void maybe_opponent_reply();
    void mark_obstacle(Square to);
    bool no_pieces(Color c) const;
};

} // namespace learn
} // namespace shogi

#endif
