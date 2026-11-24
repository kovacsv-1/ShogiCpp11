#ifndef SHOGI_ASSETS_HPP
#define SHOGI_ASSETS_HPP

/**
 * @file assets.hpp
 * @brief Fonts, piece sheets, and mark sprites (SDL textures via RAII).
 */

#include "../core/piece.hpp"
#include "sdl_raii.hpp"

#include <string>
#include <vector>

namespace shogi {
namespace ui {

/** Metadata for one piece-set entry in the menu list. */
struct PieceSetInfo {
    std::string path;
    std::string label;
    bool secret;      // hidden spacer until Konami unlock
    bool learn_gated; // visible but locked until all Learn levels cleared

    PieceSetInfo() : secret(false), learn_gated(false) {}
};

/** Loaded fonts + piece/mark textures for the UI. */
class SpriteAssets {
public:
    bool load_fonts(std::string const& font_path);

    /** Load a Little-Mage / ShogiGUI 8×4 piece sheet (PNG). */
    bool load_piece_set(SDL_Renderer* renderer, std::string const& sheet_path);

    /** Load red / arrow / line / lineend mark sprites (optional; returns false if any missing). */
    bool load_mark_sprites(SDL_Renderer* renderer);

    SDL_Texture* sheet() const { return sheet_.get(); }
    SDL_Texture* mark_red() const { return mark_red_.get(); }
    SDL_Texture* mark_arrow() const { return mark_arrow_.get(); }
    SDL_Texture* mark_line() const { return mark_line_.get(); }
    SDL_Texture* mark_lineend() const { return mark_lineend_.get(); }
    TTF_Font* font() const { return font_.get(); }
    TTF_Font* font_small() const { return font_small_.get(); }
    TTF_Font* font_tiny() const { return font_tiny_.get(); }

    int cell_w() const { return cell_w_; }
    int cell_h() const { return cell_h_; }
    std::string const& current_path() const { return current_path_; }
    std::string const& current_label() const { return current_label_; }

    SDL_Rect piece_rect(Piece const& p) const;

    /** Destination rect that fits the native piece aspect into `bounds` (centered). */
    SDL_Rect fit_in(SDL_Rect bounds) const;

    /**
     * Draw all 4 sheet rows (unpromoted + promoted for both colours) at near-native size.
     * @param renderer SDL renderer.
     * @param x        Left edge of the preview.
     * @param y        Top edge of the preview.
     * @param piece_h  Target piece height in pixels.
     * @param row_gap  Extra pixels between rows.
     * @return Height used in pixels.
     */
    int draw_preview(SDL_Renderer* renderer, int x, int y, int piece_h, int row_gap = 10) const;

    static std::vector<PieceSetInfo> discover_piece_sets();
    static std::string label_from_path(std::string const& path);

private:
    TexturePtr sheet_;
    TexturePtr mark_red_;
    TexturePtr mark_arrow_;
    TexturePtr mark_line_;
    TexturePtr mark_lineend_;
    FontPtr font_;
    FontPtr font_small_;
    FontPtr font_tiny_;
    int cell_w_;
    int cell_h_;
    std::string current_path_;
    std::string current_label_;

    static TexturePtr load_texture(SDL_Renderer* renderer, char const* path);
};

} // namespace ui
} // namespace shogi

#endif
