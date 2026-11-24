# Shogi (C++11 HF2)

Kétjátékos Shogi (japán sakk) SDL2 felülettel. A szabálymotor C++11 + STL, az SDL csak az `src/ui` rétegben van.

**Dokumentáció:** [docs/DOCUMENTATION.md](docs/DOCUMENTATION.md) (architektúra, modern C++ térkép).  
API (Doxygen): a gyökérben `doxygen Doxyfile` → `docs/api/html/index.html`.

## Windows csomag

A `ShogiCpp11-windows.zip` kicsomagolása után a `Shogi.exe` indul. MinGW-t és SDL2-t nem kell telepíteni: a futáshoz kellő DLL-ek az exe mellett vannak a zipben. Az exe, a DLL-ek, az `assets\` és az `engine\` maradjanak egy mappában.

## Build (Code::Blocks + MinGW)

1. Nyisd meg a `Shogi.cbp` fájlt Code::Blocks-ban.
2. Ellenőrizd, hogy a compiler/linker utak léteznek és helyesek
3. **Project → Properties → Build targets → Execution working dir** legyen a kicsomagolt projekt gyökér (ahol a `Shogi.cbp` van), hogy az `assets/...` és a `saves/` elérhető legyen.
4. Build és Run (F9).

Fordítási kapcsoló: `-std=c++11`. Linkelt könyvtárak: `mingw32`, `SDL2main`, `SDL2`, `SDL2_image`, `SDL2_ttf`.

Parancssorból (ugyanazzal a MinGW-vel):

```bat
g++ -std=c++11 -Wall -Isrc -I<PathTo>/CodeBlocks/MinGW/include ^
  src/main.cpp src/core/*.cpp src/ui/*.cpp ^
  -o bin/Debug/Shogi.exe ^
  -L<PathTo>/CodeBlocks/MinGW/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lcomdlg32
```

## Engine (Fairy-Stockfish)

A csomag tartalmazza:

`engine/fairy-stockfish-largeboard_x86-64-modern.exe`

Ha hiányzik: töltsd le a **largeboard** Windows buildet
([Fairy-Stockfish downloads](https://fairy-stockfish.github.io/download/) vagy
[GitHub Releases](https://github.com/fairy-stockfish/Fairy-Stockfish/releases)) —
a fájlnév: `fairy-stockfish-largeboard_x86-64-modern.exe` (Shogihoz a
`largeboard` változat kell; a sima 8×8 build nem elég). Tedd az `engine/` mappába
ugyanezzel a névvel.

Menü: **Play against engine** (feloldható kóddal, vagy Learn mind max csillag) → felugró:
**Fairy-Stockfish** vagy **Random legal move**. Te mindig Sente vagy; handicap nélkül
**furigoma** dönti el, ki kezd (keyboard-only: Tab → tábla). A USI engine háttérszálon gondolkodik (~800 ms/lépés).

## Használat

- Induláskor **piece set menü**: válassz spritesheetet (preview), opcionálisan clock / handicap, majd **New game** vagy **Learn**.
- **Setup dialog** (ha clock vagy handicap be van kapcsolva): ↑/↓ soronként; ←/→ a Sente|Gote és Start|Cancel páron; Enter/Space megerősít; Esc megszakít.
- **Ablak:** átméretezhető (1280×960 logikai felület, letterbox); **F11** vagy **Alt+Enter** = teljes képernyő.
- **Lépés:** kattints egy saját báura, majd a célmezőre (zöld kiemelés = legális célok).
- **Nyilak (menü / keyboard-only tábla):** nyomvatartva léptet; ismétlés **150 ms** (`src/ui/key_repeat.hpp`).
- **Ejés (drop):** a saját kézpanel egyik bábujára, majd üres mezőre.
- **Előléptetés:** két kék mező egymás alatt/fölött (a tábla közepe felé) — eredeti a célmezőn, promótált a szomszédon.
- **Resign / Save / Load / New / Menu / Claim Impasse:** jobb oldali gombok. Resign, New és Claim Impasse: első kattintás/Enter pirosra élesít, második megerősít; nyíl/hover nem szünteti — máshova kattintás / más aktiválás / keyboard Tab igen. Claim csak jishogi helyzetben aktív (különben üres sáv). Furigoma alatt Resign/Save zárt (locked-Engine hover). Menu → set-választó.
- **Jishogi:** mindkét király behatolása után Claim Impasse → pontszám (R/B=5, egyéb=1, király=0; 24-es küszöb).
- **Save:** `saves/shogiYYYY-MM-DDThh-mm.kifu`. **Load:** fájlválasztó (`saves/` mappából). Handicap / óra meta: `#` sorok.
- **Tutorial:** lishogi.org tutorial (18 stage, ~121 szint). Csillagok gyűjtése, ütések, sakk/matt, drop, setup. Oldalgombok: nyíl + Enter/Space/kattintás; Esc stage lista.

Sprite forrás: [Little-Mage/ShogiGUI-piece-sets](https://github.com/Little-Mage/ShogiGUI-piece-sets) — elrendezés 8×4 (`K R B G S N L P`, sötét felfelé / világos lefelé).

Learn stage adatok: [WandererXII/lila `ui/learn`](https://github.com/WandererXII/lila/tree/master/ui/learn) (AGPL-3.0) — SFEN / cél / `nbMoves` port.

## Modern C++ használat

- Nincs kézi `new`/`delete` a játéklogikában; SDL erőforrások: `std::unique_ptr` + deleter (`src/ui/sdl_raii.hpp`).
- STL: `std::array`, `std::vector`, `std::map`, `std::unordered_map`, `std::algorithm` + lambdák.
- `enum class`, RAII, érték szerinti `Board`/`Piece`/`Move`, `fstream` + `string` a mentéshez.

## Modulok

| Útvonal | Szerep |
|---------|--------|
| `src/core/` | tábla, lépésgenerálás, sakk/matt, nifu, uchifuzume, sennichite, notation, SFEN, learn |
| `src/ui/` | SDL ablak, sprite atlas, egér, kézpanel, learn UI |
| `assets/sprites/*.png` | piece set spritesheetek + UI jelek (red/arrow/line) |
| `tests/test_core.cpp` | unit tesztek (szabályok, hard mode, furigoma, handicap undo) SDL nélkül |

## Unit tesztek

Részletes kimenet (suite, how, input, expected, actual, pass/fail összesítő):

```bat
g++ -std=c++11 -Wall -Isrc -o bin/Debug/test_core.exe tests/test_core.cpp src/core/*.cpp
bin\Debug\test_core.exe
```

Kódlefedettség (`gcov`, `src/core`):

```bat
tools\run_tests_coverage.bat
```

vagy: `powershell -File tools\run_tests_coverage.ps1`

Jelentés: `docs/coverage_report.txt` (+ `bin/Debug/test_core_last_run.txt`).  
Részletes `.gcov` fájlok: `gcov/` (nem a projektgyökér).