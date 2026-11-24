#ifndef SHOGI_KEY_REPEAT_HPP
#define SHOGI_KEY_REPEAT_HPP

/**
 * @file key_repeat.hpp
 * @brief Throttle SDL key-repeat for arrow hold-to-step navigation.
 */

#include <SDL2/SDL.h>

namespace shogi {
namespace ui {

/**
 * Accept an arrow keydown for navigation.
 * Initial press always accepted; auto-repeats are throttled (default 150 ms).
 * @param is_repeat @c e.key.repeat != 0
 * @param interval_ms Minimum ms between accepted steps while held (default 150).
 */
inline bool accept_nav_key(bool is_repeat, int interval_ms = 150) {
    static Uint32 last_ms = 0;
    Uint32 now = SDL_GetTicks();
    if (!is_repeat) {
        last_ms = now;
        return true;
    }
    if (now - last_ms < static_cast<Uint32>(interval_ms)) {
        return false;
    }
    last_ms = now;
    return true;
}

} // namespace ui
} // namespace shogi

#endif
