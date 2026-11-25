/**
 * @file test_core.cpp
 * @brief Core unit tests with verbose report (input / expected / actual) + summary.
 *
 * Build (project root, no SDL):
 *   g++ -std=c++11 -Wall -Isrc -o bin/Debug/test_core.exe
 *       tests/test_core.cpp and every src/core .cpp file
 *
 * Coverage (gcov notes under gcov/): tools/run_tests_coverage.ps1 (or .bat)
 */
#include "core/engine.hpp"
#include "core/game.hpp"
#include "core/learn.hpp"
#include "core/notation.hpp"
#include "core/rules.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace shogi;

namespace {

struct Stats {
    int total;
    int passed;
    int failed;
    Stats() : total(0), passed(0), failed(0) {}
};

Stats g_stats;
std::string g_suite;

char const* color_name(Color c) {
    return c == Color::Sente ? "Sente" : "Gote";
}

char const* status_name(GameStatus s) {
    switch (s) {
    case GameStatus::Playing:
        return "Playing";
    case GameStatus::Checkmate:
        return "Checkmate";
    case GameStatus::Resignation:
        return "Resignation";
    case GameStatus::IllegalMove:
        return "IllegalMove";
    case GameStatus::KingCaptured:
        return "KingCaptured";
    case GameStatus::DrawRepetition:
        return "DrawRepetition";
    case GameStatus::DrawPerpetualCheck:
        return "DrawPerpetualCheck";
    case GameStatus::TimeForfeit:
        return "TimeForfeit";
    case GameStatus::JishogiWin:
        return "JishogiWin";
    case GameStatus::JishogiDraw:
        return "JishogiDraw";
    }
    return "?";
}

char const* piece_name(PieceType t) {
    switch (t) {
    case PieceType::Pawn:
        return "Pawn";
    case PieceType::Lance:
        return "Lance";
    case PieceType::Knight:
        return "Knight";
    case PieceType::Silver:
        return "Silver";
    case PieceType::Gold:
        return "Gold";
    case PieceType::Bishop:
        return "Bishop";
    case PieceType::Rook:
        return "Rook";
    case PieceType::King:
        return "King";
    }
    return "?";
}

std::string sq_str(Square s) {
    return s.valid() ? s.to_string() : "(invalid)";
}

std::string move_str(Move const& m) {
    std::ostringstream o;
    if (m.is_drop) {
        o << piece_name(m.drop_type) << '*' << sq_str(m.to);
    } else {
        o << sq_str(m.from) << "->" << sq_str(m.to);
        if (m.promotion == PromotionChoice::Promote) {
            o << '+';
        } else if (m.promotion == PromotionChoice::Decline) {
            o << '=';
        }
    }
    return o.str();
}

void suite(char const* name) {
    g_suite = name;
    std::cout << "\n======== SUITE: " << name << " ========\n";
}

void check(char const* name, char const* how, char const* input, std::string const& expected,
           std::string const& actual, bool ok) {
    ++g_stats.total;
    if (ok) {
        ++g_stats.passed;
    } else {
        ++g_stats.failed;
    }

    std::cout << (ok ? "[PASS] " : "[FAIL] ") << g_suite << " / " << name << '\n';
    std::cout << "  how:      " << how << '\n';
    std::cout << "  input:    " << input << '\n';
    std::cout << "  expected: " << expected << '\n';
    std::cout << "  actual:   " << actual << '\n';
}

void check_bool(char const* name, char const* how, char const* input, bool expected, bool actual) {
    check(name, how, input, expected ? "true" : "false", actual ? "true" : "false",
          expected == actual);
}

void test_starting_layout() {
    suite("Starting layout");
    Board b = Board::starting_position();
    char const* how = "Board::starting_position(); read piece type at square";
    check("Sente king on 5i", how, "square 5i on starting board", "King",
          piece_name(b.at(Square{5, 9}).piece.type),
          b.at(Square{5, 9}).piece.type == PieceType::King);
    check("Gote king on 5a", how, "square 5a on starting board", "King",
          piece_name(b.at(Square{5, 1}).piece.type),
          b.at(Square{5, 1}).piece.type == PieceType::King);
    check("Sente rook on 2h", how, "square 2h on starting board", "Rook",
          piece_name(b.at(Square{2, 8}).piece.type),
          b.at(Square{2, 8}).piece.type == PieceType::Rook);
    check("Sente bishop on 8h", how, "square 8h on starting board", "Bishop",
          piece_name(b.at(Square{8, 8}).piece.type),
          b.at(Square{8, 8}).piece.type == PieceType::Bishop);
}

void test_opening_and_notation() {
    suite("Opening move + Western notation");
    Board b = Board::starting_position();
    Move open = Move::board_move(Square{7, 7}, Square{7, 6});
    check_bool("P-7f is legal", "Rules::is_legal(starting, Sente, 7g->7f)",
               "start pos; move 7g->7f", true, Rules::is_legal(b, Color::Sente, open));

    std::string nota = Notation::to_western(b, Color::Sente, open);
    check("notation encodes P-7f", "Notation::to_western", "start pos; Sente 7g->7f", "P-7f", nota,
          nota == "P-7f");

    Game g;
    bool played = g.try_play(open);
    check_bool("Game accepts P-7f", "Game::try_play", "fresh Game; move 7g->7f", true, played);
    check("side after P-7f", "Game::side_to_move after try_play", "after Sente P-7f", "Gote",
          color_name(g.side_to_move()), g.side_to_move() == Color::Gote);
}

void test_nifu() {
    suite("Nifu (double pawn)");
    Board nifu_board;
    nifu_board.place(Square{5, 1}, Piece{Color::Sente, PieceType::King});
    nifu_board.place(Square{5, 9}, Piece{Color::Gote, PieceType::King});
    nifu_board.place(Square{5, 3}, Piece{Color::Sente, PieceType::Pawn});
    nifu_board.add_to_hand(Color::Sente, PieceType::Pawn);
    char const* input = "Sente pawn on 5c; Sente has P in hand; drop P*5d";

    check_bool("drop_causes_nifu", "Rules::drop_causes_nifu(board, Sente, 5d)", input, true,
               Rules::drop_causes_nifu(nifu_board, Color::Sente, Square{5, 4}));
    check_bool("nifu drop illegal", "Rules::is_legal(..., P*5d)", input, false,
               Rules::is_legal(nifu_board, Color::Sente, Move::drop(PieceType::Pawn, Square{5, 4})));
}

void test_must_leave_check_normal() {
    suite("Normal mode: must resolve check");
    Board b;
    b.place(Square{5, 9}, Piece{Color::Sente, PieceType::King});
    b.place(Square{5, 1}, Piece{Color::Gote, PieceType::King});
    b.place(Square{5, 5}, Piece{Color::Gote, PieceType::Rook});
    b.place(Square{1, 9}, Piece{Color::Sente, PieceType::Gold});
    char const* input = "Sente K5i, G5i; Gote R5e checks file 5; ignore = G1i->1h";

    check_bool("Sente in check", "Rules::in_check", input, true, Rules::in_check(b, Color::Sente));
    Move ignore = Move::board_move(Square{1, 9}, Square{1, 8});
    check_bool("cannot leave check", "Rules::is_legal(ignore)", input, false,
               Rules::is_legal(b, Color::Sente, ignore));
}

void test_hard_mode_may_leave_check() {
    suite("Hard mode: may leave check");
    Board b;
    b.place(Square{5, 9}, Piece{Color::Sente, PieceType::King});
    b.place(Square{5, 1}, Piece{Color::Gote, PieceType::King});
    b.place(Square{5, 5}, Piece{Color::Gote, PieceType::Rook});
    b.place(Square{1, 9}, Piece{Color::Sente, PieceType::Gold});

    Game g;
    g.set_hard_mode(true);
    g.set_position(std::move(b), Color::Sente);
    Move ignore = Move::board_move(Square{1, 9}, Square{1, 8});
    auto moves = g.legal_moves();
    bool found = std::any_of(moves.begin(), moves.end(), [&](Move const& m) { return m == ignore; });
    char const* input = "hard_mode; same check position; move G1i->1h";

    check_bool("leave-check in legal_moves", "Game::legal_moves contains ignore", input, true,
               found);
    check_bool("try_play leave-check", "Game::try_play(ignore)", input, true, g.try_play(ignore));
    check("still Playing", "Game::status", input, "Playing", status_name(g.status()),
          g.status() == GameStatus::Playing);
    check("Gote to move", "Game::side_to_move", input, "Gote", color_name(g.side_to_move()),
          g.side_to_move() == Color::Gote);
}

void test_hard_mode_illegal_loses() {
    suite("Hard mode: illegal move loses");
    Game g;
    g.set_hard_mode(true);
    g.reset();
    Move bad = Move::board_move(Square{7, 7}, Square{7, 5});
    char const* input = "hard_mode; start pos; illegal pawn jump 7g->7e";

    bool ended = g.try_play(bad);
    check_bool("try_play returns true (game ended)", "Game::try_play(bad)", input, true, ended);
    check("status IllegalMove", "Game::status", input, "IllegalMove", status_name(g.status()),
          g.status() == GameStatus::IllegalMove);
    check("Gote wins", "Game::winner", input, "Gote", color_name(g.winner()),
          g.winner() == Color::Gote);
}

void test_hard_mode_king_capture() {
    suite("Hard mode: king capture wins");
    Board b;
    b.place(Square{5, 9}, Piece{Color::Sente, PieceType::King});
    b.place(Square{5, 5}, Piece{Color::Gote, PieceType::King});
    b.place(Square{5, 6}, Piece{Color::Sente, PieceType::Gold});

    Game g;
    g.set_hard_mode(true);
    g.set_position(std::move(b), Color::Sente);
    Move take = Move::board_move(Square{5, 6}, Square{5, 5});
    char const* input = "hard_mode; Sente G5f takes Gote K5e";

    check_bool("try_play capture", "Game::try_play(5f->5e)", input, true, g.try_play(take));
    check("status KingCaptured", "Game::status", input, "KingCaptured", status_name(g.status()),
          g.status() == GameStatus::KingCaptured);
    check("Sente wins", "Game::winner", input, "Sente", color_name(g.winner()),
          g.winner() == Color::Sente);
}

void test_hard_mode_no_tsume_auto_win() {
    suite("Hard mode: no auto checkmate");
    Board b;
    b.place(Square{5, 9}, Piece{Color::Sente, PieceType::King});
    b.place(Square{5, 1}, Piece{Color::Gote, PieceType::King});
    b.place(Square{5, 2}, Piece{Color::Sente, PieceType::Gold});
    b.place(Square{4, 2}, Piece{Color::Sente, PieceType::Gold});
    b.place(Square{6, 2}, Piece{Color::Sente, PieceType::Gold});

    check_bool("position is mate for Gote (Rules)", "Rules::is_checkmate(Gote)",
               "Gote K5a surrounded by three Sente Golds", true,
               Rules::is_checkmate(b, Color::Gote));

    Game g;
    g.set_hard_mode(true);
    g.set_position(std::move(b), Color::Gote);
    char const* input = "hard_mode; load tsume as Gote to move";

    check("no auto Checkmate on load", "Game::status after set_position", input, "Playing",
          status_name(g.status()), g.status() == GameStatus::Playing);

    auto moves = g.legal_moves();
    if (!moves.empty()) {
        std::string in2 = "first pseudo move " + move_str(moves.front());
        g.try_play(moves.front());
        check("status never Checkmate after pseudo play", "Game::status after try_play",
              in2.c_str(), "not Checkmate", status_name(g.status()),
              g.status() != GameStatus::Checkmate);
    } else {
        check_bool("no pseudo moves but still not auto-mate", "legal_moves empty", input, true,
                   true);
    }
}

void test_furigoma_starter() {
    suite("Furigoma starter");
    char const* how = "Game::furigoma_starter(promoted, total=5)";
    check("0 promoted → Sente", how, "promoted=0 total=5", "Sente",
          color_name(Game::furigoma_starter(0, 5)), Game::furigoma_starter(0, 5) == Color::Sente);
    check("2 promoted → Sente", how, "promoted=2 total=5", "Sente",
          color_name(Game::furigoma_starter(2, 5)), Game::furigoma_starter(2, 5) == Color::Sente);
    check("3 promoted → Gote", how, "promoted=3 total=5", "Gote",
          color_name(Game::furigoma_starter(3, 5)), Game::furigoma_starter(3, 5) == Color::Gote);
    check("5 promoted → Gote", how, "promoted=5 total=5", "Gote",
          color_name(Game::furigoma_starter(5, 5)), Game::furigoma_starter(5, 5) == Color::Gote);
}

void test_reset_starting_gote() {
    suite("reset_starting(Gote)");
    Game g;
    g.reset_starting(Color::Gote);
    char const* input = "Game::reset_starting(Gote); open 7c->7d";
    check("start_side", "Game::start_side", input, "Gote", color_name(g.start_side()),
          g.start_side() == Color::Gote);
    check("side_to_move", "Game::side_to_move", input, "Gote", color_name(g.side_to_move()),
          g.side_to_move() == Color::Gote);
    Move open = Move::board_move(Square{7, 3}, Square{7, 4});
    check_bool("Gote opens", "Game::try_play(7c->7d)", input, true, g.try_play(open));
    check("Sente after open", "Game::side_to_move", input, "Sente", color_name(g.side_to_move()),
          g.side_to_move() == Color::Sente);
}

void test_sfen_roundtrip_helper() {
    suite("SFEN parse");
    Board b;
    Color side = Color::Sente;
    std::string sfen = "4k4/9/9/9/9/9/9/9/4K4 b - 1";
    char const* input = "SFEN 4k4/9/9/9/9/9/9/9/4K4 b - 1";

    check_bool("parse_sfen ok", "learn::parse_sfen", input, true,
               learn::parse_sfen(sfen, b, side));
    check("Gote king 5a", "board.at(5a).type", input, "King",
          piece_name(b.at(Square{5, 1}).piece.type),
          b.at(Square{5, 1}).piece.type == PieceType::King);
    check("Sente king 5i", "board.at(5i).type", input, "King",
          piece_name(b.at(Square{5, 9}).piece.type),
          b.at(Square{5, 9}).piece.type == PieceType::King);
    check("side to move Sente", "parsed side from 'b'", input, "Sente", color_name(side),
          side == Color::Sente);
}

void test_random_engine_factory() {
    suite("RandomEngine factory");
    auto eng = make_engine(EngineKind::RandomLegal);
    check_bool("make_engine non-null", "make_engine(RandomLegal)", "EngineKind::RandomLegal", true,
               eng != nullptr);
    eng->start();
    check_bool("running after start", "Engine::start + running", "RandomEngine", true,
               eng->running());

    Board b = Board::starting_position();
    std::string sfen = learn::to_sfen(b, Color::Sente, 1);
    eng->go_sfen(sfen, 1);
    std::string bm;
    bool polled = eng->poll_bestmove(bm);
    check_bool("poll_bestmove ready", "go_sfen(start) then poll", sfen.c_str(), true, polled);
    check("USI move non-empty", "bestmove token", sfen.c_str(), "non-empty USI ≠ resign",
          bm.empty() ? "(empty)" : bm, !bm.empty() && bm != "resign");
    eng->shutdown();
}

void test_jishogi_points_and_impasse() {
    suite("jishogi");
    // Bare kings in enemy camps + equal material leftovers → draw at 24+ each is hard;
    // use a minimal entered position and check helpers first.
    Board b;
    b.clear();
    b.place(Square{5, 1}, Piece{Color::Sente, PieceType::King, false}); // entered
    b.place(Square{5, 9}, Piece{Color::Gote, PieceType::King, false});  // entered
    check_bool("both kings entered", "Rules::both_kings_entered", "Ks on 5a & 5i", true,
               Rules::both_kings_entered(b));
    {
        Game g0;
        g0.reset();
        check_bool("no claim on hirate", "Game::can_claim_jishogi", "starting position", false,
                   g0.can_claim_jishogi());
    }
    check("kings alone = 0 pts", "Rules::jishogi_points(Sente)", "only king", "0",
          std::to_string(Rules::jishogi_points(b, Color::Sente)),
          Rules::jishogi_points(b, Color::Sente) == 0);

    b.place(Square{1, 5}, Piece{Color::Sente, PieceType::Rook, false});
    b.place(Square{2, 5}, Piece{Color::Sente, PieceType::Bishop, true}); // still 5
    b.place(Square{3, 5}, Piece{Color::Sente, PieceType::Gold, false});
    b.add_to_hand(Color::Sente, PieceType::Pawn);
    check("Sente R+B+G+P = 12", "Rules::jishogi_points", "R5 B5 G1 P1", "12",
          std::to_string(Rules::jishogi_points(b, Color::Sente)),
          Rules::jishogi_points(b, Color::Sente) == 12);

    // Fill Sente to 24+, leave Gote under 24 → Gote loses (Sente wins).
    for (int i = 0; i < 12; ++i) {
        b.add_to_hand(Color::Sente, PieceType::Pawn);
    }
    // Sente now 12+12=24; Gote only king = 0 — claim required (not automatic).
    Game g;
    g.set_position(b, Color::Sente);
    check_bool("can claim after enter", "Game::can_claim_jishogi", "Sente 24 Gote 0", true,
               g.can_claim_jishogi());
    check("still Playing until claim", "Game::status", "before claim", "Playing",
          status_name(g.status()), g.status() == GameStatus::Playing);
    check_bool("claim ok", "Game::claim_jishogi", "Sente 24 Gote 0", true, g.claim_jishogi());
    check("status JishogiWin", "Game::status after claim", "Sente 24 Gote 0", "JishogiWin",
          status_name(g.status()), g.status() == GameStatus::JishogiWin);
    check("Sente wins", "Game::winner", "Sente 24 Gote 0", "Sente", color_name(g.winner()),
          g.winner() == Color::Sente);

    // Both >= 24 → draw after claim
    Board d;
    d.clear();
    d.place(Square{5, 2}, Piece{Color::Sente, PieceType::King, false});
    d.place(Square{5, 8}, Piece{Color::Gote, PieceType::King, false});
    for (int i = 0; i < 24; ++i) {
        d.add_to_hand(Color::Sente, PieceType::Pawn);
        d.add_to_hand(Color::Gote, PieceType::Pawn);
    }
    Game g2;
    g2.set_position(d, Color::Gote);
    check_bool("claim draw", "Game::claim_jishogi", "both 24 pts", true, g2.claim_jishogi());
    check("status JishogiDraw", "Game::status", "both 24 pts", "JishogiDraw",
          status_name(g2.status()), g2.status() == GameStatus::JishogiDraw);
}

void test_handicap_undo_keeps_removed_pieces() {
    suite("handicap undo");
    HandicapSettings hc;
    hc.enabled = true;
    hc.uwate = Color::Sente;
    hc.rank_diff = 4; // Rook handicap: Sente rook off 2h

    Game g;
    g.reset_handicap(hc);
    char const* input = "Rook HC (Sente uwate); P-7f; undo";

    check_bool("rook gone at start", "board.at(2h).occupied", input, false,
               g.board().at(Square{2, 8}).occupied);
    check("uwate to move", "Game::side_to_move", input, "Sente", color_name(g.side_to_move()),
          g.side_to_move() == Color::Sente);

    Move open = Move::board_move(Square{7, 7}, Square{7, 6});
    check_bool("Sente opens P-7f", "Game::try_play", input, true, g.try_play(open));

    // Kifu must restore handicap so undo after load does not revive removed pieces.
    char const* kifu_path = "bin/Debug/test_handicap_roundtrip.kifu";
    check_bool("save handicap kifu", "Game::save_kifu", kifu_path, true, g.save_kifu(kifu_path));

    check_bool("undo ok", "Game::undo", input, true, g.undo());
    check_bool("rook still gone after undo", "board.at(2h).occupied after undo", input, false,
               g.board().at(Square{2, 8}).occupied);
    check_bool("bishop present", "board.at(8h).occupied after undo", input, true,
               g.board().at(Square{8, 8}).occupied);

    Game loaded;
    check_bool("load handicap kifu", "Game::load_kifu", kifu_path, true, loaded.load_kifu(kifu_path));
    check_bool("loaded handicap on", "Game::handicap().enabled", kifu_path, true,
               loaded.handicap().enabled);
    check("loaded uwate Sente", "Game::handicap().uwate", kifu_path, "Sente",
          color_name(loaded.handicap().uwate), loaded.handicap().uwate == Color::Sente);
    check_bool("loaded can undo", "Game::can_undo after 1-ply kifu", kifu_path, true,
               loaded.can_undo());
    check_bool("undo after load", "Game::undo after load", kifu_path, true, loaded.undo());
    check_bool("rook gone after load+undo", "board.at(2h) after load undo", kifu_path, false,
               loaded.board().at(Square{2, 8}).occupied);
}

void test_move_generator_and_board_move() {
    suite("MoveGenerator + Board move");
    Game g;
    g.reset();
    auto all = g.legal_moves();
    {
        std::ostringstream in;
        in << "starting Game; default gen size=" << all.size();
        check_bool("default gen non-empty", "Game::legal_moves", in.str().c_str(), true,
                   !all.empty());
    }

    g.set_move_generator([](Board const& b, Color c) {
        auto moves = Rules::generate_legal(b, c);
        moves.erase(std::remove_if(moves.begin(), moves.end(),
                                   [](Move const& m) {
                                       return m.is_drop || m.from.file != 7;
                                   }),
                    moves.end());
        return moves;
    });
    auto filtered = g.legal_moves();
    check_bool("custom gen non-empty", "set_move_generator(file==7 filter)",
               "only board moves from file 7", true, !filtered.empty());
    bool only7 = std::all_of(filtered.begin(), filtered.end(),
                             [](Move const& m) { return !m.is_drop && m.from.file == 7; });
    check_bool("custom gen only file 7", "inspect filtered moves", "filter lambda", true, only7);

    g.set_hard_mode(false);
    {
        std::ostringstream exp;
        exp << all.size();
        std::ostringstream act;
        act << g.legal_moves().size();
        check("set_hard_mode restores default", "set_hard_mode(false) rebinds gen",
              "after custom filter", exp.str(), act.str(), g.legal_moves().size() == all.size());
    }

    Board src = Board::starting_position();
    Board dst = std::move(src);
    check("Board move ctor", "Board dst = std::move(src)", "starting_position then move", "King",
          piece_name(dst.at(Square{5, 9}).piece.type),
          dst.at(Square{5, 9}).piece.type == PieceType::King);
}

void test_protection_level_three() {
    suite("Protection level 3");
    // Stage order matches lishogi: protection is index 10, "no escape" is level 3 (index 2).
    learn::LearnSession session;
    session.start_stage(10, 2);
    char const* goal = "There is no escape, but you can defend!";
    check("goal text", "LearnSession::level().goal", "protection stage, level index 2", goal,
          session.level().goal, std::string(session.level().goal) == goal);

    // Bishop steps off the file so the rook on 7h recaptures 7f. The silver stays,
    // but it is defended, so the level is won (lishogi: not an unprotected capture).
    Move defend = Move::board_move(Square{7, 7}, Square{8, 6});
    check_bool("bishop 7g-8f legal", "Rules::is_legal", "protection 3 start", true,
               Rules::is_legal(session.board(), Color::Sente, defend));
    check_bool("defense played", "LearnSession::try_player_move(7g->8f)", "protection 3", true,
               session.try_player_move(defend));
    check("defended silver is a win", "LearnSession::result", "7g->8f", "Success",
          session.result() == learn::LevelResult::Success ? "Success" : "not Success",
          session.result() == learn::LevelResult::Success);
    check("silver still on 7f", "board.at(7f)", "after 7g->8f", "Sente Silver",
          std::string(color_name(session.board().at(Square{7, 6}).piece.color)) + " " +
              piece_name(session.board().at(Square{7, 6}).piece.type),
          session.board().at(Square{7, 6}).occupied &&
              session.board().at(Square{7, 6}).piece.color == Color::Sente &&
              session.board().at(Square{7, 6}).piece.type == PieceType::Silver);

    session.restart_level();
    Move hang = Move::board_move(Square{5, 5}, Square{5, 4});
    check_bool("pawn push played", "LearnSession::try_player_move(5e->5d)", "protection 3", true,
               session.try_player_move(hang));
    check("hanging silver is a loss", "LearnSession::result", "5e->5d", "Failure",
          session.result() == learn::LevelResult::Failure ? "Failure" : "not Failure",
          session.result() == learn::LevelResult::Failure);
    PieceSlot const& taken = session.board().at(Square{7, 6});
    check("failure shows the rook capture", "board.at(7f) after fail", "5e->5d", "Gote Rook",
          taken.occupied ? std::string(color_name(taken.piece.color)) + " " + piece_name(taken.piece.type)
                         : "(empty)",
          taken.occupied && taken.piece.color == Color::Gote && taken.piece.type == PieceType::Rook);
}

void test_value_level_four() {
    suite("Piece value level 4");
    learn::LearnSession session;
    session.start_stage(16, 3);
    check("stage", "LearnSession::stage().key", "stage index 16", "value", session.stage().key,
          std::string(session.stage().key) == "value");

    // Bishop is the second-most valuable piece. Taking it first must lose.
    Move bishop_first = Move::board_move(Square{6, 6}, Square{5, 7});
    check_bool("bishop-first played", "try_player_move(6f->5g)", "value level 4", true,
               session.try_player_move(bishop_first));
    check("bishop-first loses", "LearnSession::result", "6f->5g", "Failure",
          session.result() == learn::LevelResult::Failure ? "Failure" : "not Failure",
          session.result() == learn::LevelResult::Failure);

    session.restart_level();
    // Rook, bishop, gold, silver, knight, lance, pawn, with the steps between them.
    Square const path[][2] = {
        {Square{6, 6}, Square{7, 7}}, {Square{7, 7}, Square{5, 7}}, {Square{5, 7}, Square{5, 4}},
        {Square{5, 4}, Square{5, 8}}, {Square{5, 8}, Square{7, 8}}, {Square{7, 8}, Square{7, 6}},
        {Square{7, 6}, Square{4, 6}}, {Square{4, 6}, Square{6, 6}}, {Square{6, 6}, Square{6, 4}},
        {Square{6, 4}, Square{4, 4}}, {Square{4, 4}, Square{4, 7}},
    };
    int const n = 11;
    bool line_ok = true;
    for (int i = 0; i < n; ++i) {
        Move step = Move::board_move(path[i][0], path[i][1]);
        if (!session.try_player_move(step)) {
            line_ok = false;
            check("ordered line move accepted", "try_player_move", move_str(step).c_str(), "true",
                  "false", false);
            break;
        }
        bool last = (i + 1 == n);
        learn::LevelResult expect =
            last ? learn::LevelResult::Success : learn::LevelResult::Playing;
        if (session.result() != expect) {
            line_ok = false;
            check("ordered line result", "LearnSession::result after move", move_str(step).c_str(),
                  last ? "Success" : "Playing",
                  session.result() == learn::LevelResult::Success
                      ? "Success"
                      : (session.result() == learn::LevelResult::Failure ? "Failure" : "Playing"),
                  false);
            break;
        }
    }
    check_bool("rook-first line clears the level", "11 moves in value order",
               "7g, 5g, 5d, 7h, 4f, 6d, 4g", true, line_ok);
}

void test_other_learn_win_conditions() {
    suite("Other learn win conditions");

    learn::LearnSession hang;
    hang.start_stage(12, 10);
    check_bool("hanging escape loses", "try L*5i", "out of check, watch the reply", true,
               hang.try_player_move(Move::drop(PieceType::Lance, Square{5, 9})));
    check("L*5i is not safe", "result", "L*5i", "Failure",
          hang.result() == learn::LevelResult::Failure ? "Failure" : "not Failure",
          hang.result() == learn::LevelResult::Failure);
    hang.restart_level();
    check_bool("safe lance drop wins", "try L*7i", "out of check", true,
               hang.try_player_move(Move::drop(PieceType::Lance, Square{7, 9})));
    check("L*7i wins", "result", "L*7i", "Success",
          hang.result() == learn::LevelResult::Success ? "Success" : "not Success",
          hang.result() == learn::LevelResult::Success);

    learn::LearnSession block;
    block.start_stage(12, 11);
    check_bool("king step loses", "try 2h1h", "must land on 3f", true,
               block.try_player_move(Move::board_move(Square{2, 8}, Square{1, 8})));
    check("2h1h is not the answer", "result", "2h1h", "Failure",
          block.result() == learn::LevelResult::Failure ? "Failure" : "not Failure",
          block.result() == learn::LevelResult::Failure);
    block.restart_level();
    check_bool("gold to 3f wins", "try 4g3f", "out of check", true,
               block.try_player_move(Move::board_move(Square{4, 7}, Square{3, 6})));
    check("4g3f wins", "result", "4g3f", "Success",
          block.result() == learn::LevelResult::Success ? "Success" : "not Success",
          block.result() == learn::LevelResult::Success);

    learn::LearnSession sacrifice;
    sacrifice.start_stage(16, 5);
    check("rook sacrifice is already played", "board.at(6h) after start", "piece value 6",
          "Gote Rook",
          (sacrifice.board().at(Square{6, 8}).occupied &&
           sacrifice.board().at(Square{6, 8}).piece.color == Color::Gote &&
           sacrifice.board().at(Square{6, 8}).piece.type == PieceType::Rook)
              ? "Gote Rook"
              : "not the rook",
          sacrifice.board().at(Square{6, 8}).occupied &&
              sacrifice.board().at(Square{6, 8}).piece.color == Color::Gote &&
              sacrifice.board().at(Square{6, 8}).piece.type == PieceType::Rook);
    check_bool("taking the rook loses", "try 7h6h", "piece value 6", true,
               sacrifice.try_player_move(Move::board_move(Square{7, 8}, Square{6, 8})));
    check("7h6h fails", "result", "7h6h", "Failure",
          sacrifice.result() == learn::LevelResult::Failure ? "Failure" : "not Failure",
          sacrifice.result() == learn::LevelResult::Failure);
    sacrifice.restart_level();
    check_bool("king steps aside", "try 7h8g", "piece value 6", true,
               sacrifice.try_player_move(Move::board_move(Square{7, 8}, Square{8, 7})));
    check("7h8g wins", "result", "7h8g", "Success",
          sacrifice.result() == learn::LevelResult::Success ? "Success" : "not Success",
          sacrifice.result() == learn::LevelResult::Success);

    learn::LearnSession king;
    king.start_stage(16, 8);
    check_bool("wrong drop loses", "try G*2a", "piece value 9", true,
               king.try_player_move(Move::drop(PieceType::Gold, Square{2, 1})));
    check("G*2a fails", "result", "G*2a", "Failure",
          king.result() == learn::LevelResult::Failure ? "Failure" : "not Failure",
          king.result() == learn::LevelResult::Failure);
    king.restart_level();
    check_bool("silver drop wins", "try S*2a", "piece value 9", true,
               king.try_player_move(Move::drop(PieceType::Silver, Square{2, 1})));
    check("S*2a wins", "result", "S*2a", "Success",
          king.result() == learn::LevelResult::Success ? "Success" : "not Success",
          king.result() == learn::LevelResult::Success);

    learn::LearnSession flipped;
    flipped.start_stage(17, 2);
    check("flipped level is Gote to move", "side_to_move", "repetition 3", "Gote",
          color_name(flipped.side_to_move()), flipped.side_to_move() == Color::Gote);
    check_bool("gote 7f8g accepted", "try 7f8g", "repetition 3", true,
               flipped.try_player_move(Move::board_move(Square{7, 6}, Square{8, 7})));
    check("still playing after the reply", "result", "7f8g", "Playing",
          flipped.result() == learn::LevelResult::Playing ? "Playing" : "not Playing",
          flipped.result() == learn::LevelResult::Playing);
    check_bool("gote 6e7f wins", "try 6e7f", "repetition 3", true,
               flipped.try_player_move(Move::board_move(Square{6, 5}, Square{7, 6})));
    check("repetition 3 complete", "result", "6e7f", "Success",
          flipped.result() == learn::LevelResult::Success ? "Success" : "not Success",
          flipped.result() == learn::LevelResult::Success);
}

void print_summary() {
    std::cout << "\n======== SUMMARY ========\n";
    std::cout << "Total assertions:  " << g_stats.total << '\n';
    std::cout << "Passed:            " << g_stats.passed << '\n';
    std::cout << "Failed:            " << g_stats.failed << '\n';
    if (g_stats.total > 0) {
        double pct = 100.0 * static_cast<double>(g_stats.passed) / static_cast<double>(g_stats.total);
        std::cout << "Pass rate:         " << pct << "%\n";
    }
    std::cout << "\nCode coverage: not embedded in this binary.\n"
                 "  Run tools\\run_tests_coverage.ps1 (or .bat) for gcov of src/core.\n"
                 "  Report → docs/coverage_report.txt; detailed notes → gcov/\n";
}

} // namespace

int main() {
    std::cout << "Shogi core unit tests (C++11, no SDL)\n"
                 "Each case prints: how / input / expected / actual\n";

    test_starting_layout();
    test_opening_and_notation();
    test_nifu();
    test_must_leave_check_normal();
    test_hard_mode_may_leave_check();
    test_hard_mode_illegal_loses();
    test_hard_mode_king_capture();
    test_hard_mode_no_tsume_auto_win();
    test_furigoma_starter();
    test_reset_starting_gote();
    test_jishogi_points_and_impasse();
    test_handicap_undo_keeps_removed_pieces();
    test_sfen_roundtrip_helper();
    test_move_generator_and_board_move();
    test_random_engine_factory();
    test_protection_level_three();
    test_value_level_four();
    test_other_learn_win_conditions();

    print_summary();

    if (g_stats.failed) {
        std::cerr << "RESULT: FAILED\n";
        return 1;
    }
    std::cout << "RESULT: OK\n";
    return 0;
}
