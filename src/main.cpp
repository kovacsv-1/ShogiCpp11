#include "core/game.hpp"
#include "ui/assets.hpp"
#include "ui/controller.hpp"
#include "ui/learn_ui.hpp"
#include "ui/menu.hpp"
#include "ui/sdl_raii.hpp"
#include "ui/view.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <string>

namespace {

bool file_exists(std::string const& path) {
    SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb");
    if (!rw) {
        return false;
    }
    SDL_RWclose(rw);
    return true;
}

std::string find_font() {
    char const* candidates[] = {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "assets/fonts/DejaVuSans.ttf",
    };
    for (char const* c : candidates) {
        if (file_exists(c)) {
            return c;
        }
    }
    return candidates[0];
}

} // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << '\n';
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        std::cerr << "IMG_Init warning: " << IMG_GetError() << '\n';
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init: " << TTF_GetError() << '\n';
        return 1;
    }

    try {
        Uint32 win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        shogi::ui::WindowPtr window(SDL_CreateWindow(
            "Shogi (C++11 HF2)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            shogi::ui::BoardView::window_w, shogi::ui::BoardView::window_h, win_flags));
        shogi::ui::ensure(static_cast<bool>(window), "SDL_CreateWindow");
        SDL_SetWindowMinimumSize(window.get(), shogi::ui::BoardView::window_w / 2,
                                 shogi::ui::BoardView::window_h / 2);

        shogi::ui::RendererPtr renderer(
            SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
        if (!renderer) {
            renderer.reset(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_SOFTWARE));
        }
        shogi::ui::ensure(static_cast<bool>(renderer), "SDL_CreateRenderer");
        // Letterboxed logical playfield; SDL 2 remaps mouse button/motion to these coords.
        SDL_RenderSetLogicalSize(renderer.get(), shogi::ui::BoardView::window_w,
                                 shogi::ui::BoardView::window_h);

        bool fullscreen = false;
        shogi::ui::SpriteAssets assets;
        assets.load_fonts(find_font());
        if (!assets.load_mark_sprites(renderer.get())) {
            std::cerr << "Warning: mark sprites (red/arrow/line/lineend) not fully loaded\n";
        }

        auto sets = shogi::ui::SpriteAssets::discover_piece_sets();
        if (sets.empty()) {
            std::cerr << "No piece sheets found in assets/sprites/\n";
            return 1;
        }

        shogi::ui::PieceSetMenu menu;
        if (!menu.init(renderer.get(), assets, sets.front().path)) {
            std::cerr << "Failed to load piece set: " << sets.front().path << '\n';
            return 1;
        }

        shogi::Game game;
        shogi::ui::BoardView view(assets);
        shogi::ui::InputController controller(game);
        shogi::ui::LearnUi learn_ui;

        enum class AppScreen { Menu, Game, Learn };
        AppScreen screen = AppScreen::Menu;
        bool running = true;

        while (running) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    running = false;
                    break;
                }
                if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
                    SDL_Keycode sym = e.key.keysym.sym;
                    bool alt_enter =
                        (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) &&
                        ((e.key.keysym.mod & KMOD_ALT) != 0);
                    if (sym == SDLK_F11 || alt_enter) {
                        fullscreen = !fullscreen;
                        SDL_SetWindowFullscreen(
                            window.get(),
                            fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                        continue;
                    }
                }
                if (screen == AppScreen::Menu) {
                    shogi::ui::MenuAction action = menu.handle_event(e, assets, renderer.get());
                    if (action == shogi::ui::MenuAction::Quit) {
                        running = false;
                    } else if (action == shogi::ui::MenuAction::ContinueGame) {
                        game.set_allow_review_fork(menu.allow_review_fork());
                        controller.set_keyboard_board(menu.keyboard_board());
                        controller.set_hard_mode(menu.hard_mode());
                        screen = AppScreen::Game;
                    } else if (action == shogi::ui::MenuAction::NewGame) {
                        game.set_allow_review_fork(menu.allow_review_fork());
                        controller.set_keyboard_board(menu.keyboard_board());
                        controller.set_hard_mode(menu.hard_mode());
                        controller.start_new_game(menu.game_setup());
                        menu.set_session_active(true);
                        screen = AppScreen::Game;
                    } else if (action == shogi::ui::MenuAction::Learn) {
                        learn_ui.open();
                        learn_ui.set_keyboard_board(menu.keyboard_board());
                        screen = AppScreen::Learn;
                    } else if (action == shogi::ui::MenuAction::PlayEngine) {
                        game.set_allow_review_fork(menu.allow_review_fork());
                        controller.set_keyboard_board(menu.keyboard_board());
                        controller.start_engine_game(menu.chosen_engine(), menu.game_setup());
                        menu.set_session_active(true);
                        screen = AppScreen::Game;
                    }
                } else if (screen == AppScreen::Learn) {
                    shogi::ui::LearnAction la = learn_ui.handle_event(e, view);
                    if (la == shogi::ui::LearnAction::QuitApp) {
                        running = false;
                    } else if (la == shogi::ui::LearnAction::BackToMenu) {
                        menu.set_keyboard_board(learn_ui.keyboard_board());
                        menu.init(renderer.get(), assets, assets.current_path());
                        screen = AppScreen::Menu;
                    }
                } else {
                    if (!controller.handle_event(e, view)) {
                        running = false;
                    } else if (controller.consume_open_menu()) {
                        menu.set_session_active(true);
                        menu.set_allow_review_fork(game.allow_review_fork());
                        menu.set_keyboard_board(controller.keyboard_board());
                        menu.set_hard_mode(controller.hard_mode());
                        menu.init(renderer.get(), assets, assets.current_path());
                        screen = AppScreen::Menu;
                    }
                }
            }

            if (screen == AppScreen::Game) {
                controller.tick();
            }

            if (screen == AppScreen::Menu) {
                menu.draw(renderer.get(), assets);
            } else if (screen == AppScreen::Learn) {
                learn_ui.draw(renderer.get(), view, assets);
            } else {
                controller.render(renderer.get(), view);
            }
            SDL_Delay(16);
        }
    } catch (std::exception const& ex) {
        std::cerr << "Fatal: " << ex.what() << '\n';
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
