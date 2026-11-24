// Auto-generated from Lishogi learn stages (AGPL-3.0 attribution: lila/ui/learn).
// Do not edit by hand — run tools/extract_learn.py
#include "learn.hpp"

namespace shogi {
namespace learn {

namespace {

char const* const obs_1_0[] = {"5d"};
char const* const obs_1_1[] = {"6c", "5b", "4c", "4b"};
char const* const obs_1_2[] = {"5c"};
char const* const obs_2_0[] = {"4d"};
char const* const obs_2_1[] = {"8g", "8f", "7g", "6f"};
char const* const obs_2_2[] = {"6h", "7g", "8f", "8e", "7d", "6c", "6d", "5d"};
char const* const obs_2_3[] = {"2c", "3b"};
char const* const obs_2_4[] = {"5d"};
char const* const obs_3_0[] = {"5e", "6d", "8f"};
char const* const obs_3_1[] = {"4e", "5f", "6g", "5h", "4i"};
char const* const obs_3_2[] = {"6f", "6g", "5f", "3e", "2f"};
char const* const obs_3_3[] = {"5d", "4d", "4c"};
char const* const obs_3_4[] = {"5d"};
char const* const obs_3_5[] = {"5d"};
char const* const obs_4_0[] = {"7g", "8e"};
char const* const obs_4_1[] = {"7f", "8d", "7b", "6b", "6c"};
char const* const obs_4_2[] = {"4d"};
char const* const obs_4_3[] = {"5d"};
char const* const obs_5_0[] = {"5g", "5d"};
char const* const obs_5_1[] = {"9b", "8a", "1b", "2a"};
char const* const obs_5_2[] = {"5d"};
char const* const obs_5_3[] = {"5d"};
char const* const obs_6_0[] = {"6d", "8f"};
char const* const obs_6_1[] = {"8h", "7i", "7e", "5i", "5g", "4h"};
char const* const obs_6_2[] = {"2b", "3b", "8g"};
char const* const obs_6_3[] = {"9i"};
char const* const obs_6_4[] = {"8b", "8c"};
char const* const obs_7_0[] = {"5d"};
char const* const obs_7_1[] = {"7g", "3g"};
char const* const obs_7_2[] = {"2h", "3i", "3b", "4c"};
char const* const obs_7_3[] = {"5i"};
char const* const obs_7_4[] = {"5b", "6c"};
char const* const obs_8_0[] = {"7d"};
char const* const obs_8_1[] = {"2c"};
char const* const obs_8_2[] = {"5d"};
char const* const obs_8_3[] = {"5d"};
ScenarioStep const scen_14_2[] = {{"G*6i", Color::Sente}, {"G*4a", Color::Gote}, {"G*4i", Color::Sente}, {"G*6a", Color::Gote}};
ScenarioStep const scen_14_3[] = {{"S*7i", Color::Sente}, {"S*7a", Color::Gote}, {"S*3i", Color::Sente}, {"S*3a", Color::Gote}};
ScenarioStep const scen_14_4[] = {{"N*8i", Color::Sente}, {"N*2a", Color::Gote}, {"N*2i", Color::Sente}, {"N*8a", Color::Gote}};
ScenarioStep const scen_14_5[] = {{"L*9i", Color::Sente}, {"P*1c", Color::Gote}, {"L*1i", Color::Sente}, {"P*2c", Color::Gote}};
ScenarioStep const scen_14_6[] = {{"B*8h", Color::Sente}, {"P*3c", Color::Gote}, {"R*2h", Color::Sente}, {"P*4c", Color::Gote}};
ScenarioStep const scen_14_7[] = {{"P*5g", Color::Sente}, {"P*5c", Color::Gote}, {"P*6g", Color::Sente}, {"P*6c", Color::Gote}, {"P*4g", Color::Sente}, {"P*7c", Color::Gote}, {"P*7g", Color::Sente}, {"P*8c", Color::Gote}, {"P*3g", Color::Sente}, {"P*9c", Color::Gote}, {"P*8g", Color::Sente}, {"L*1a", Color::Gote}, {"P*2g", Color::Sente}, {"L*9a", Color::Gote}, {"P*9g", Color::Sente}, {"B*2b", Color::Gote}, {"P*1g", Color::Sente}, {"R*8b", Color::Gote}};
ScenarioStep const scen_14_8[] = {{"7g7f", Color::Sente}};
ScenarioStep const scen_16_5[] = {{"6b6h+", Color::Gote}, {"7h8g", Color::Sente}};
ScenarioStep const scen_16_7[] = {{"7f4c+", Color::Sente}, {"3b4c", Color::Gote}, {"S*3b", Color::Sente}, {"2a2b", Color::Gote}, {"3b4c+", Color::Sente}};
ScenarioStep const scen_17_0[] = {{"9i8i", Color::Sente}, {"9e8e", Color::Gote}, {"8i9i", Color::Sente}, {"8e9e", Color::Gote}, {"9i8i", Color::Sente}, {"9e8e", Color::Gote}, {"8i9i", Color::Sente}, {"8e9e", Color::Gote}, {"9i8i", Color::Sente}, {"9e8e", Color::Gote}, {"8i9i", Color::Sente}, {"8e9e", Color::Gote}};
ScenarioStep const scen_17_1[] = {{"9f8e", Color::Sente}, {"8g7f", Color::Gote}, {"8e9f", Color::Sente}, {"7f8g", Color::Gote}, {"9f8e", Color::Sente}, {"8g7f", Color::Gote}, {"8e9f", Color::Sente}, {"7f8g", Color::Gote}, {"9f8e", Color::Sente}, {"8g7f", Color::Gote}, {"8e9f", Color::Sente}, {"7f8g", Color::Gote}};
ScenarioStep const scen_17_2[] = {{"7f8g", Color::Gote}, {"9f8e", Color::Sente}, {"6e7f", Color::Gote}};

LearnLevel const levels_0[] = {
    {
        "Choose a piece design (or just make any move).",
        "9/9/9/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b -",
        1,
        SuccessKind::Always,
        FailureKind::None,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Promotion zone — move the pawn into the highlighted ranks.",
        "9/9/9/4P4/9/9/9/9/9 b -",
        1,
        SuccessKind::Always,
        FailureKind::None,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Sente (Black) moves first — make any legal move.",
        "9/9/9/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b -",
        1,
        SuccessKind::Always,
        FailureKind::None,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_1[] = {
    {
        "The king is slow — collect the star.",
        "9/9/9/9/9/9/3K5/9/9 b -",
        3,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_1_0, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Grab all the stars!",
        "9/9/9/5K3/9/9/9/9/9 b -",
        4,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_1_1, 4,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "King summary — collect the star.",
        "9/9/9/9/4K4/9/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_1_2, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_2[] = {
    {
        "Grab all the stars!",
        "9/9/9/9/9/4G4/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_2_0, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Fewer moves = better score.",
        "9/9/9/9/1G7/9/9/9/9 b -",
        4,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_2_1, 4,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Fewer moves = better score.",
        "9/9/9/9/9/9/9/9/3G5 b -",
        8,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_2_2, 8,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Gold Doesnt Promote",
        "9/9/9/6G2/9/9/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_2_3, 2,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Gold summary — collect the star.",
        "9/9/9/9/9/4G4/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_2_4, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_3[] = {
    {
        "Grab all the stars!",
        "9/9/9/9/9/4S4/9/9/9 b -",
        4,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_3_0, 3,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Grab all the stars!",
        "9/9/9/4S4/9/9/9/9/9 b -",
        5,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_3_1, 5,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Fewer moves = better score.",
        "9/9/9/9/3S5/6S2/9/9/9 b -",
        5,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_3_2, 5,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Silver Promotion",
        "9/9/9/6S2/9/9/9/9/9 b -",
        3,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_3_3, 3,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Silver summary — collect the star.",
        "9/9/9/9/9/4S4/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_3_4, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Psilver Summary",
        "9/9/9/9/9/4+S4/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_3_5, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_4[] = {
    {
        "Knights jump — collect the stars.",
        "9/9/9/9/9/9/9/9/1N7 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_4_0, 2,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Knight Promotion",
        "9/9/9/9/9/9/9/1N7/9 b -",
        5,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_4_1, 5,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Knight Summary",
        "9/9/9/9/9/4N4/9/9/9 b -",
        1,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_4_2, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Pknight Summary",
        "9/9/9/9/9/4+N4/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_4_3, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_5[] = {
    {
        "Lances Are Straighforward",
        "9/9/9/9/9/9/9/9/4L4 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_5_0, 2,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Lance Promotion",
        "9/9/9/9/9/9/9/9/L7L b -",
        4,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_5_1, 4,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Lance summary — collect the star.",
        "9/9/9/9/9/4L4/9/9/9 b -",
        1,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_5_2, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Plance Summary Two",
        "9/9/9/9/9/4+L4/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_5_3, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_6[] = {
    {
        "Grab all the stars!",
        "9/9/9/9/9/9/6B2/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_6_0, 2,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Eh There Is No Piece",
        "9/9/9/9/9/9/9/9/9 b B",
        7,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_6_1, 6,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Bishop Promotion",
        "9/9/9/9/9/9/2B6/9/9 b -",
        3,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_6_2, 3,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Bishop summary — collect the star.",
        "9/9/9/9/4B4/9/9/9/9 b -",
        1,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_6_3, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Horse Summary",
        "9/9/9/9/4+B4/9/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_6_4, 2,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_7[] = {
    {
        "Rook Goal",
        "9/9/9/9/9/9/9/4R4/9 b -",
        1,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_7_0, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Grab All The Stars Remember",
        "9/9/9/9/9/9/9/9/9 b R",
        3,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_7_1, 2,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Rook Promotion",
        "9/7R1/9/9/9/9/9/9/9 b -",
        4,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_7_2, 4,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Rook summary — collect the star.",
        "9/9/9/9/4R4/9/9/9/9 b -",
        1,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_7_3, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Dragon Summary Two",
        "9/9/9/9/4+R4/9/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_7_4, 2,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_8[] = {
    {
        "Pawns Move One Square Only",
        "9/9/9/9/9/9/2P6/9/9 b -",
        3,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_8_0, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Pawn Promotion",
        "9/9/9/9/9/9/6P2/9/9 b -",
        5,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_8_1, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Pawn summary — collect the star.",
        "9/9/9/9/9/4P4/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_8_2, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Tokin Summary",
        "9/9/9/9/9/4+P4/9/9/9 b -",
        2,
        SuccessKind::Obstacles,
        FailureKind::None,
        obs_8_3, 1,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_9[] = {
    {
        "Capture all enemy pieces.",
        "9/4n4/9/9/4p4/9/9/4L4/9 b -",
        2,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/9/4nr3/4G4/9/9/9/9/9 b -",
        2,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/9/9/4p4/9/4B1s2/5g3/9/9 b -",
        4,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/3spg3/3p1p3/4N4/9/9/9/9/9 b -",
        5,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/4r1s2/5n3/3R1b3/9/9/9/9/9 b -",
        6,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/5l+p2/4S4/4pp3/9/9/9/9/9 b -",
        7,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_10[] = {
    {
        "Get out of check!",
        "9/1r7/9/9/9/2PP5/Pp7/1B7/LNS6 b - 1",
        1,
        SuccessKind::NoOpponentCapture,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Get out of check!",
        "9/9/9/9/7Pb/9/8P/6+p2/3S3RL b - 1",
        1,
        SuccessKind::NoOpponentCapture,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "No Escape",
        "9/9/2n6/2r6/4P4/2S6/1PBP5/2R6/9 b - 1",
        3,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Make Sure All Safe",
        "9/9/6b2/7R1/9/9/1P7/1S7/9 b - 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Make Sure All Safe",
        "9/9/6n2/9/9/5Pp2/6N2/9/9 b - 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Make Sure All Safe",
        "9/9/7+P1/6s2/5N3/9/9/9/9 b - 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Make Sure All Safe",
        "9/9/9/9/9/9/5G3/9/3+bS4 b - 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Make Sure All Safe",
        "9/9/9/9/3n5/9/2PPS4/3R5/9 b - 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Dont Forget You Can Drop To Defend",
        "9/9/4S4/9/4r4/9/4G4/9/9 b L 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Dont Forget You Can Drop To Defend",
        "9/9/9/9/9/3B5/7S1/9/3G3r1 b NLP 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Dont Let Them Take Any Undefended Piece",
        "9/9/9/9/9/9/4P4/4G4/2S2+r3 b NLP 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Dont Let Them Take Any Undefended Piece",
        "9/9/9/9/7S1/7+b1/9/5G3/9 b NLP 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Dont Let Them Take Any Undefended Piece",
        "8l/9/9/7n1/8P/8L/6PP1/6S2/7N1 b LP 1",
        1,
        SuccessKind::NoUnprotectedCapture,
        FailureKind::UnprotectedCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_11[] = {
    {
        "Check In One Goal",
        "5R3/4Ssk2/5p3/9/7L1/9/9/9/9 b r2b4g2s4n3l17p 3",
        1,
        SuccessKind::CheckGote,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Check In One Goal",
        "3g5/2rk1g3/pps2pppp/2S6/9/9/9/9/9 b G 1",
        1,
        SuccessKind::CheckGote,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Check In One Goal",
        "9/9/9/2B2r3/9/4L4/9/9/7k1 b - 1",
        1,
        SuccessKind::CheckGote,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Check In One Goal",
        "5+B2s/7k1/7N1/6Ss1/9/9/9/9/9 b r2b4g2s4n3l17p 1",
        1,
        SuccessKind::CheckGote,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Check In One Goal",
        "6snl/4+Rgk2/5pppp/9/9/9/9/9/9 b GSr2b2g2s3n3l14p 1",
        1,
        SuccessKind::CheckGote,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_12[] = {
    {
        "If Your King Is Attacked",
        "9/9/9/9/4r4/9/9/9/4K4 b - 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Get out of check!",
        "9/9/9/9/9/9/9/2s1r4/3K5 b - 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "The King Cannot Escape But Block",
        "9/9/9/9/9/9/6PPP/4+r2K1/6SNL b - 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "The King Cannot Escape But Block",
        "9/9/9/9/9/9/6PPP/5+r1K1/7NL b G 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "The King Cannot Escape But Block",
        "9/9/9/9/5b3/9/6B1P/8K/6sNL b P 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "You Can Get Out Of Check By Taking",
        "9/9/9/9/9/9/6PPp/5+p1KP/5GbNL b - 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Get Out Of Check",
        "9/9/9/9/9/9/3Ss4/3s5/3K5 b - 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Get Out Of Check",
        "9/9/9/7p1/8b/4sPK2/6g2/9/9 b - 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Get Out Of Check",
        "9/9/9/6n2/6p2/5Ks2/9/4r4/9 b - 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Get Out Of Check",
        "9/9/9/9/8P/8l/7P1/6rSK/7NL b P 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Watch Out For Your Opponents Reply",
        "9/9/9/9/9/4b4/PPG6/LS7/K4r3 b L 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Watch Out For Your Opponents Reply",
        "9/9/9/9/9/5+bn2/5GPPP/6SK1/5G1NL b g 1",
        1,
        SuccessKind::NotInCheck,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_13[] = {
    {
        "To Win In Shogi",
        "6k2/9/6P2/9/9/9/9/9/9 b G2r2b3g4s4n4l17p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Drops Common Deliver Mate",
        "7k1/9/7S1/9/9/9/9/9/9 b S2r2b4g2s4n4l18p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "7nk/7bl/9/9/6N2/9/9/9/9 b 2rb4g4s2n3l18p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "4R3G/7k1/6ppp/9/9/9/9/9/9 b r2b3g4s4n4l15p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "7kl/7s1/6PSp/9/9/9/9/9/9 b 2r2b4g2s4n3l16p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "9/5n2+B/5pk2/9/7+R1/9/9/9/9 b rb4g4s3n4l17p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "5k3/6s2/5NB2/9/9/9/9/9/9 b 2rb4g3s3n4l18p 2",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "4R3l/6g1k/5gsp1/6p1P/9/9/9/9/8L b r2b2g3s4n2l15p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Choose Your Piece Carefully",
        "8l/6S1k/9/9/8g/9/9/9/9 b GS2r2b2g2s4n3l18p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Choose Your Piece Carefully",
        "8l/7k1/7pb/7N1/9/9/9/9/9 b RGrb3g4s3n3l17p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Choose Your Piece Carefully",
        "8l/6+Rgk/7pp/9/9/9/9/9/9 b GSr2b2g3s4n3l16p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Mate With A Dropped Pawn Is",
        "7+P1/8g/7pk/9/7Sp/9/9/9/9 b LP2r2b3g3s4n3l14p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Mate With A Pushed Pawn Is",
        "7+P1/8g/7pk/9/7SP/9/9/9/9 b 2r2b3g3s4n4l15p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "8l/5BSk1/6p1P/9/9/9/9/9/9 b 2rb4g3s4n3l16p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "5lk2/8R/5Ps2/6N2/6L2/9/9/9/9 b r2b4g3s3n2l17p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Put the opponent in check!",
        "2R3gkl/3R3s1/7p1/7Np/9/9/9/9/9 b 2b3g3s3n3l16p 1",
        1,
        SuccessKind::Checkmate,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_14[] = {
    {
        "This Is The Initial Position",
        "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1",
        1,
        SuccessKind::Always,
        FailureKind::None,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "First Place The King",
        "4k4/9/9/9/9/9/9/6K2/9 b RB2G2S2N2L9Prb2g2s2n2l9p 1",
        2,
        SuccessKind::PieceOn,
        FailureKind::None,
        nullptr, 0,
        nullptr, 0,
        "5i", PieceType::King, Color::Sente, false
    },
    {
        "Drop A Gold",
        "4k4/9/9/9/9/9/9/9/4K4 b RB2G2S2N2L9Prb2g2s2n2l9p 1",
        2,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_14_2, 4,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Then Place A Silver",
        "3gkg3/9/9/9/9/9/9/9/3GKG3 b RB2S2N2L9Prb2s2n2l9p 1",
        2,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_14_3, 4,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Drop The Knights",
        "2sgkgs2/9/9/9/9/9/9/9/2SGKGS2 b RB2N2L9Prb2n2l9p 1",
        2,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_14_4, 4,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Drop The Lances",
        "1nsgkgsn1/9/9/9/9/9/9/9/1NSGKGSN1 b RB2L9Prb2l9p 1",
        2,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_14_5, 4,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Place The Bishop Then Rook",
        "1nsgkgsn1/9/7pp/9/9/9/9/9/LNSGKGSNL b RB9Prb2l7p 1",
        2,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_14_6, 4,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Place The Pawns",
        "1nsgkgsn1/9/5pppp/9/9/9/9/1B5R1/LNSGKGSNL b 9Prb2l5p 1",
        17,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_14_7, 18,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Pushing The3rd Pawn",
        "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1",
        2,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_14_8, 1,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_15[] = {
    {
        "Captured pieces can be dropped — clear the board.",
        "9/9/8r/4n4/9/9/4L4/9/9 b -",
        3,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Drop limitations — clear the enemies.",
        "9/9/9/9/3nbl3/9/9/9/9 b P",
        6,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/9/9/2g1s4/9/9/9/6N2/9 b - 1",
        4,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/5p3/4Gl3/9/9/5g3/9/9/9 b - 1",
        4,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "4n4/s3g4/8g/9/4B4/9/9/9/9 b - 1",
        7,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/3rn4/5S3/3lp4/9/7b1/9/9/9 b - 1",
        7,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Capture all enemies — don't leave yours hanging!",
        "9/9/2n1r1b2/9/3s1g1R1/9/9/9/9 b - 1",
        8,
        SuccessKind::ExtinctGote,
        FailureKind::AnyCapture,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Nifu — clear the enemies without illegal pawn drop.",
        "9/9/4s4/9/9/9/2PP1PP2/9/9 b P 1",
        4,
        SuccessKind::ExtinctGote,
        FailureKind::None,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Nifu — clear the enemies without illegal pawn drop.",
        "9/4G4/1n2p2b1/9/9/1+P5P1/9/9/9 b - 1",
        5,
        SuccessKind::ExtinctGote,
        FailureKind::None,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_16[] = {
    {
        "Pawns Are The Least Valuable",
        "9/9/3pl4/2n1+R4/9/9/9/9/9 b - 1",
        1,
        SuccessKind::ColorOn,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "7d", PieceType::Pawn, Color::Sente, false
    },
    {
        "Knight Silver Gold",
        "9/9/4ns3/4S4/3g5/9/9/9/9 b - 1",
        1,
        SuccessKind::ColorOn,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "4c", PieceType::Pawn, Color::Sente, false
    },
    {
        "Gold Bishop Rook",
        "9/9/9/5gb2/4p+Bn2/4srl2/4gs3/9/9 b - 1",
        1,
        SuccessKind::ColorOn,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "4f", PieceType::Pawn, Color::Sente, false
    },
    {
        "Take All The Pieces Starting From The Most",
        "9/9/9/3lg4/9/3+R1n3/2r1bp3/2s6/9 b - 1",
        11,
        SuccessKind::ExtinctGote,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "An Exchange Is",
        "9/6k2/3p2g2/3s5/7N1/9/9/3R5/9 b - 1",
        1,
        SuccessKind::ColorOn,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "3c", PieceType::Pawn, Color::Sente, false
    },
    {
        "The Opponent Just Gave Away",
        "7k1/3r5/6+P+P+P/9/P3n4/2N6/2P6/2K6/L8 b 2g 1",
        2,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_16_5, 2,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Your Kings Value Is Infinite",
        "7k1/9/6+P+P+P/9/Pg2n4/2N6/1KP6/3+r5/L2G5 b g 1",
        1,
        SuccessKind::PieceOn,
        FailureKind::IfNotSuccess,
        nullptr, 0,
        nullptr, 0,
        "2b", PieceType::Pawn, Color::Sente, true
    },
    {
        "Two Generals Are Better",
        "l6rl/2k3g2/2ppps3/pp3pp2/2P4pp/P1BP1P3/1P2P1PPP/2S4R1/L1K2G2L b b 1",
        3,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_16_7, 5,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Remember Which Piece Is The Most Valuable",
        "ln+R5l/3r1gks1/5pnp1/2b3p1p/1p7/P3pP2P/1P1+b1GPP1/6SK1/5+p1NL b GSPgsnl3p 1",
        1,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        nullptr, 0,
        "", PieceType::Pawn, Color::Sente, false
    },
};

LearnLevel const levels_17[] = {
    {
        "If The Same Position Occurs",
        "9/9/9/9/k8/9/+p8/3+p5/K8 b - 1",
        12,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_17_0, 12,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Perpetual Check Is A Loss",
        "3k5/9/1s+P+P+P4/9/P2+b5/KP7/1+b7/9/9 b -",
        12,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_17_1, 12,
        "", PieceType::Pawn, Color::Sente, false
    },
    {
        "Board Flipped Find Best Move",
        "3k5/9/1s+P+P+P4/9/P2+b5/KP+b6/9/9/9 w -",
        3,
        SuccessKind::Scenario,
        FailureKind::Scenario,
        nullptr, 0,
        scen_17_2, 3,
        "", PieceType::Pawn, Color::Sente, false
    },
};

} // namespace

std::vector<LearnStage> const& all_stages() {
    static std::vector<LearnStage> stages = {
        {
            "intro",
            "Introduction",
            "Basics",
            levels_0,
            3
        },
        {
            "king",
            "The King",
            "The most important piece",
            levels_1,
            3
        },
        {
            "gold",
            "The Gold",
            "It Moves In Any Direction Except Diagonally Back",
            levels_2,
            5
        },
        {
            "silver",
            "The Silver",
            "It Moves Either Forward Or Diagonally Back",
            levels_3,
            6
        },
        {
            "knight",
            "The Knight",
            "It Moves In An L Shape",
            levels_4,
            4
        },
        {
            "lance",
            "The Lance",
            "It Moves Straight Forward",
            levels_5,
            4
        },
        {
            "bishop",
            "The Bishop",
            "It Moves Diagonally",
            levels_6,
            5
        },
        {
            "rook",
            "The Rook",
            "It Moves In Straight Lines",
            levels_7,
            5
        },
        {
            "pawn",
            "The Pawn",
            "It Moves Forward Only",
            levels_8,
            4
        },
        {
            "capture",
            "Capture",
            "Take the enemy pieces",
            levels_9,
            6
        },
        {
            "protection",
            "Protection",
            "Keep Your Pieces Safe",
            levels_10,
            13
        },
        {
            "check1",
            "Deliver check!",
            "Attack The Opponents King",
            levels_11,
            5
        },
        {
            "out-of-check",
            "Out of check",
            "Defend so the opponent cannot capture freely.",
            levels_12,
            12
        },
        {
            "checkmate1",
            "Checkmate in one!",
            "Defeat The Opponents King",
            levels_13,
            16
        },
        {
            "setup",
            "Board setup",
            "How The Game Starts",
            levels_14,
            9
        },
        {
            "drop",
            "Piece drops",
            "Reuse Captured Pieces",
            levels_15,
            9
        },
        {
            "value",
            "Piece value",
            "Evaluate Piece Strength",
            levels_16,
            9
        },
        {
            "repetition",
            "Repetition",
            "Fourfold Repetition Is A Draw Except",
            levels_17,
            3
        },
    };
    return stages;
}

} // namespace learn
} // namespace shogi
