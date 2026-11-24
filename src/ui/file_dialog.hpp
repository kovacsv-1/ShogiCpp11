#ifndef SHOGI_FILE_DIALOG_HPP
#define SHOGI_FILE_DIALOG_HPP

/**
 * @file file_dialog.hpp
 * @brief Timestamped kifu paths and native open dialog (Windows).
 */

#include <string>

namespace shogi {
namespace ui {

/** @c saves/shogiYYYY-MM-DDThh-mm.kifu from local time (creates @c saves/ if needed). */
std::string make_timestamped_kifu_name();

/**
 * Native open-file dialog filtered to @c *.kifu.
 * @param[out] path_out Selected path.
 * @return False if cancelled.
 */
bool open_kifu_dialog(std::string& path_out);

} // namespace ui
} // namespace shogi

#endif
