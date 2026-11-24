#ifndef SHOGI_SDL_RAII_HPP
#define SHOGI_SDL_RAII_HPP

/**
 * @file sdl_raii.hpp
 * @brief @c std::unique_ptr deleters wrapping SDL destroy/free functions.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace shogi {
namespace ui {

/** @c SDL_DestroyWindow */
struct WindowDeleter {
    void operator()(SDL_Window* p) const {
        if (p) {
            SDL_DestroyWindow(p);
        }
    }
};

/** @c SDL_DestroyRenderer */
struct RendererDeleter {
    void operator()(SDL_Renderer* p) const {
        if (p) {
            SDL_DestroyRenderer(p);
        }
    }
};

/** @c SDL_DestroyTexture */
struct TextureDeleter {
    void operator()(SDL_Texture* p) const {
        if (p) {
            SDL_DestroyTexture(p);
        }
    }
};

/** @c TTF_CloseFont */
struct FontDeleter {
    void operator()(TTF_Font* p) const {
        if (p) {
            TTF_CloseFont(p);
        }
    }
};

/** @c SDL_FreeSurface */
struct SurfaceDeleter {
    void operator()(SDL_Surface* p) const {
        if (p) {
            SDL_FreeSurface(p);
        }
    }
};

typedef std::unique_ptr<SDL_Window, WindowDeleter> WindowPtr;
typedef std::unique_ptr<SDL_Renderer, RendererDeleter> RendererPtr;
typedef std::unique_ptr<SDL_Texture, TextureDeleter> TexturePtr;
typedef std::unique_ptr<TTF_Font, FontDeleter> FontPtr;
typedef std::unique_ptr<SDL_Surface, SurfaceDeleter> SurfacePtr;

/** Throw @c std::runtime_error with @c SDL_GetError if @p ok is false. */
inline void ensure(bool ok, char const* what) {
    if (!ok) {
        throw std::runtime_error(std::string(what) + ": " + SDL_GetError());
    }
}

} // namespace ui
} // namespace shogi

#endif
