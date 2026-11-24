#include "assets.hpp"

#include <SDL2/SDL_image.h>

#include <algorithm>

namespace shogi {
namespace ui {
namespace {

/*
 * Little-Mage / ShogiGUI layout (8 cols × 4 rows):
 *
 *   K  R  B  G  S  N  L  P     dark, point up   (Sente)
 *   K' R+ B+    S+ N+ L+ P+
 *   K  R  B  G  S  N  L  P     light, point down (Gote)
 *   K' R+ B+    S+ N+ L+ P+
 *
 * K' = alternate king (玉); Gold has no promoted glyph (empty cell).
 */

int piece_column(PieceType t) {
    switch (t) {
    case PieceType::King:   return 0;
    case PieceType::Rook:   return 1;
    case PieceType::Bishop: return 2;
    case PieceType::Gold:   return 3;
    case PieceType::Silver: return 4;
    case PieceType::Knight: return 5;
    case PieceType::Lance:  return 6;
    case PieceType::Pawn:   return 7;
    }
    return 0;
}

void sheet_row_col(Piece const& p, int& row, int& col) {
    col = piece_column(p.type);
    bool dark = (p.color == Color::Sente); // dark / up = Sente at bottom
    int base = dark ? 0 : 2;

    if (p.type == PieceType::King) {
        // Sente: K (王), Gote: K' (玉) on the promoted row of that colour block
        row = dark ? 0 : 3;
        col = 0;
        return;
    }

    if (p.promoted && can_promote(p.type) && p.type != PieceType::Gold) {
        row = base + 1;
        return;
    }
    row = base;
}

bool file_readable(std::string const& path) {
    SDL_RWops* rw = SDL_RWFromFile(path.c_str(), "rb");
    if (!rw) {
        return false;
    }
    SDL_RWclose(rw);
    return true;
}

} // namespace

std::string SpriteAssets::label_from_path(std::string const& path) {
    std::string name = path;
    auto slash = name.find_last_of("/\\");
    if (slash != std::string::npos) {
        name = name.substr(slash + 1);
    }
    auto dot = name.find_last_of('.');
    if (dot != std::string::npos) {
        name = name.substr(0, dot);
    }
    // Friendly labels
    if (name == "1kanji") {
        return "1 Kanji";
    }
    if (name == "2kanji_orangain") {
        return "2 Kanji";
    }
    if (name == "valdivia-2D") {
        return "Valdivia 2D";
    }
    if (name == "valdivia-3D") {
        return "Valdivia 3D";
    }
    if (name == "notation") {
        return "Notation";
    }
    if (name == "blank") {
        return "Blank";
    }
    return name;
}

std::vector<PieceSetInfo> SpriteAssets::discover_piece_sets() {
    // Display order: (Koma), kanji/valdivia/intl/western, Notation, Blank
    char const* files[] = {
        "assets/sprites/\xE9\xA7\x92.png", // 駒.png — secret slot 0
        "assets/sprites/1kanji.png",
        "assets/sprites/2kanji_orangain.png",
        "assets/sprites/valdivia-3D.png",
        "assets/sprites/valdivia-2D.png",
        "assets/sprites/International.png",
        "assets/sprites/Western.png",
        "assets/sprites/notation.png",
        "assets/sprites/blank.png",
    };

    std::vector<PieceSetInfo> out;
    for (char const* f : files) {
        if (!file_readable(f)) {
            continue;
        }
        PieceSetInfo info;
        info.path = f;
        info.secret = false;
        info.learn_gated = false;
        std::string path(f);
        if (path.find("\xE9\xA7\x92") != std::string::npos) {
            info.label = "Koma";
            info.secret = true;
        } else if (path.find("blank.png") != std::string::npos) {
            info.label = "Blank";
            info.learn_gated = true;
        } else {
            info.label = label_from_path(f);
        }
        out.push_back(info);
    }
    return out;
}

bool SpriteAssets::load_fonts(std::string const& font_path) {
    font_.reset(TTF_OpenFont(font_path.c_str(), 24));
    font_small_.reset(TTF_OpenFont(font_path.c_str(), 16));
    font_tiny_.reset(TTF_OpenFont(font_path.c_str(), 12));
    return true;
}

TexturePtr SpriteAssets::load_texture(SDL_Renderer* renderer, char const* path) {
    SDL_Surface* surf = IMG_Load(path);
    if (!surf) {
        return TexturePtr();
    }
    SurfacePtr owned(surf);
    TexturePtr tex(SDL_CreateTextureFromSurface(renderer, owned.get()));
    if (tex) {
        SDL_SetTextureBlendMode(tex.get(), SDL_BLENDMODE_BLEND);
    }
    return tex;
}

bool SpriteAssets::load_mark_sprites(SDL_Renderer* renderer) {
    mark_red_ = load_texture(renderer, "assets/sprites/red.png");
    mark_arrow_ = load_texture(renderer, "assets/sprites/arrow.png");
    mark_line_ = load_texture(renderer, "assets/sprites/line.png");
    mark_lineend_ = load_texture(renderer, "assets/sprites/lineend.png");
    return mark_red_ && mark_arrow_ && mark_line_ && mark_lineend_;
}

bool SpriteAssets::load_piece_set(SDL_Renderer* renderer, std::string const& sheet_path) {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // bilinear when scaling
    SDL_Surface* surf = IMG_Load(sheet_path.c_str());
    if (!surf) {
        return false;
    }
    SurfacePtr owned(surf);
    TexturePtr tex(SDL_CreateTextureFromSurface(renderer, owned.get()));
    if (!tex) {
        return false;
    }

    sheet_ = std::move(tex);
    cell_w_ = owned->w / 8;
    cell_h_ = owned->h / 4;
    if (cell_w_ <= 0 || cell_h_ <= 0) {
        sheet_.reset();
        return false;
    }
    current_path_ = sheet_path;
    current_label_ = label_from_path(sheet_path);
    if (sheet_path.find("\xE9\xA7\x92") != std::string::npos) {
        current_label_ = "Koma";
    }
    return true;
}

SDL_Rect SpriteAssets::piece_rect(Piece const& p) const {
    int row = 0;
    int col = 0;
    sheet_row_col(p, row, col);
    SDL_Rect r{col * cell_w_, row * cell_h_, cell_w_, cell_h_};
    return r;
}

SDL_Rect SpriteAssets::fit_in(SDL_Rect bounds) const {
    if (cell_w_ <= 0 || cell_h_ <= 0) {
        return bounds;
    }
    float aspect = static_cast<float>(cell_w_) / static_cast<float>(cell_h_);
    int w = bounds.w;
    int h = static_cast<int>(w / aspect);
    if (h > bounds.h) {
        h = bounds.h;
        w = static_cast<int>(h * aspect);
    }
    SDL_Rect dst{bounds.x + (bounds.w - w) / 2, bounds.y + (bounds.h - h) / 2, w, h};
    return dst;
}

int SpriteAssets::draw_preview(SDL_Renderer* renderer, int x, int y, int piece_h,
                               int row_gap) const {
    if (!sheet_ || cell_w_ <= 0 || cell_h_ <= 0) {
        return 0;
    }

    // Keep native aspect; size by height so Western ~81x90 stays sharp.
    int piece_w = static_cast<int>(piece_h * (static_cast<float>(cell_w_) / cell_h_));
    int gap = 6;

    // Blit every cell of all 4 sheet rows (includes K' and promoted glyphs).
    for (int row = 0; row < 4; ++row) {
        int py = y + row * (piece_h + row_gap);
        for (int col = 0; col < 8; ++col) {
            SDL_Rect src{col * cell_w_, row * cell_h_, cell_w_, cell_h_};
            SDL_Rect dst{x + col * (piece_w + gap), py, piece_w, piece_h};
            SDL_RenderCopy(renderer, sheet_.get(), &src, &dst);
        }
    }
    return 4 * piece_h + 3 * row_gap;
}

} // namespace ui
} // namespace shogi
