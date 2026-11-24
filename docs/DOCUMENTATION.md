# Shogi — InfoC++11 NHF2 dokumentáció

Kétjátékos / engine elleni Shogi (japán sakk) C++11 + SDL2 felülettel.
A szabálymotor tiszta C++11/STL; az SDL csak az `src/ui` rétegben van.

## Cél és elfogadási illeszkedés

A program a tárgy NHF2 elvárásainak megfelelően **modern C++11** eszközöket használ
(STL, lambda, okos pointer, osztályhierarchia, move, `std::function`, szálak),
és **nem** kézi `new`/`delete`-et a játéklogikában, **nem** saját tárolót az STL helyett,
**nem** functor osztályt lambda helyett.

Részletes API: Doxygen (`Doxyfile` a projekt gyökerében) → `docs/api/html/index.html`.

```bat
doxygen Doxyfile
```

## Architektúra

```
src/main.cpp          alkalmazás-ciklus (Menu / Game / Learn)
src/core/             szabályok, játszma, notation, learn, engine  (SDL nélkül)
src/ui/               SDL ablak, bemenet, menü, tutorial UI
tests/                unit tesztek (SDL nélkül)
engine/               Fairy-Stockfish USI exe (külső)
```

| Modul | Felelősség |
|-------|------------|
| `Board` / `Move` / `Piece` | Állás, lépés, bábu érték típusok |
| `Rules` | Lépésgenerálás, sakk/matt, nifu, uchifuzume |
| `Game` | Játszmaállapot, history/undo/redo, handicap, óra, kifu |
| `Engine` hierarchia | Ellenfél: USI folyamat vagy random legális |
| `learn::*` | Tutorial curriculum, SFEN/USI, progress |
| `ui::*` | SDL RAII, nézet, controller, menü |

A UI a `Game` / `Engine` publikus API-ját hívja; a core nem ismer SDL-t.

## Modern C++ elemek (rövid térkép)

| Eszköz | Hol | Miért |
|--------|-----|--------|
| `enum class` | `types.hpp`, státuszok | Erős típusok |
| STL tárolók + `algorithm` + lambda | `rules`, `game`, UI | Szűrés, másolás, keresés |
| `std::unique_ptr` + deleter | `Engine`, `sdl_raii.hpp` | RAII ownership |
| Virtuális hierarchia + `override` / `final` | `Engine` → `UsiEngine` / `RandomEngine` | Bővíthető ellenfél |
| `std::function` | `Game::MoveGenerator`, `RandomEngine::MovePicker` | Hard/normal filter, pluggable picker |
| Move ctor / `std::move` | `Board`, `Game::set_position` | Nagy állás másolásának elkerülése |
| `std::thread` / `mutex` / `atomic` | `UsiEngine` reader | Nem blokkoló USI I/O |
| Kivételek | engine start, SDL `ensure` | Hibák a UI status szövegébe |

C++11-ben nincs szabványos `std::make_unique`; a factory `std::unique_ptr<T>(new T(...))`
formát használ — ez okos pointer ownership, nem kézi `delete`.

## Játékmenet (rövid)

- **Menü:** piece set, clock, handicap, takebacks, hard mode, keyboard-only; New game / Tutorial / Engine / Quit.
- **Nyilak nyomvatartása:** a kijelölés léptetése throttelve (`accept_nav_key`, alapból **150 ms** / lépés; lásd `src/ui/key_repeat.hpp`).
- **Chess clock / handicap:** menü kapcsolók; New game / Engine előtt setup dialog (idő 0–180 perc + 0–180 s increment, nem mindkettő 0; uwate + rangkülönbség 1–10). Billentyűzet: ↑/↓ soronként, ←/→ a Sente|Gote és Start|Cancel páron belül; Enter/Space megerősít (uwate választás mint a piece set). Esc zár. Óra + handicap meta a kifu `#` soraiban.
- **Undo/redo handicapnál:** a visszajátszás a komaochi kezdőállásról indul (`show_ply`), nem a teljes tábláról.
- **Furigoma:** New game és Engine esetén (handicap nélkül) 5 gyalog dönti el, ki kezd; engine ellen az ember mindig Sente. Egér: táblakattintás. Keyboard-only: oldalsó menü fókusz, **Tab** eltünteti és a táblára teszi a fókuszt. Overlay alatt Load/New/Menu aktív; Resign/Save zárt (sötét + locked-Engine hover).
- **Hard mode:** pszeudo-legális lépések; szabálytalan kísérlet / királyütés dönt.
- **Engine:** felugró — Fairy-Stockfish (USI) vagy Random legal move.
- **Ablak:** átméretezhető + F11 / Alt+Enter fullscreen; `SDL_RenderSetLogicalSize` (1280×960) — az egéreseményeket az SDL mapeli, ne kétszer.
- **Tutorial:** Lishogi-szerű stage/level, csillagok (`learn_scores.txt`).
- **Kifu:** Western notation mentés/betöltés (`saves/`).

Szabályok (röviden): nifu, dead-end drop, uchifuzume, sakkban hagyás tiltása (normál mód),
kötelező/opcionális promóció, sennichite, örökös sakk, **dzsisógi** (mindkét király behatolt →
**Claim impasse** gomb; pontszám: Bástya/Futó 5, többi 1, király 0; 24 alatt veszít, mindkettő ≥24
döntetlen). Ha a claim nem elérhető, a helyén koma-szerű üres sáv van (Menu alatt, réssel).

- **Resign / New / Claim impasse:** első kattintás / Enter pirosra „élesíti”; második megerősít.
  Nyíl/hover nem szünteti meg; máshova kattintás, más gomb aktiválása, vagy keyboard-only **Tab** igen.

## Build

Lásd a gyökér `README.md` (Code::Blocks / parancssor, SDL linkelés, working directory).

Unit tesztek (how / input / expected / actual + összesítő):

```bat
g++ -std=c++11 -Wall -Isrc -o bin/Debug/test_core.exe tests/test_core.cpp src/core/*.cpp
bin\Debug\test_core.exe
```

Lefedettség (`gcov`): `tools\run_tests_coverage.bat` (vagy `.ps1`) → `docs/coverage_report.txt`; részletes notes: `gcov/`.

## Források

- Piece sprite setek: [Little-Mage/ShogiGUI-piece-sets](https://github.com/Little-Mage/ShogiGUI-piece-sets)
- Learn adatok: [WandererXII/lila `ui/learn`](https://github.com/WandererXII/lila/tree/master/ui/learn) (AGPL-3.0)
- Engine: Fairy-Stockfish largeboard Windows build (`engine/`; a csomagban benne van;
  pótlás: [downloads](https://fairy-stockfish.github.io/download/) →
  `fairy-stockfish-largeboard_x86-64-modern.exe`)
