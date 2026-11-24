#include "file_dialog.hpp"

#include <ctime>
#include <cstdio>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#else
#include <sys/stat.h>
#endif

namespace shogi {
namespace ui {

namespace {

void ensure_saves_dir() {
#ifdef _WIN32
    CreateDirectoryA("saves", nullptr);
#else
    mkdir("saves", 0755);
#endif
}

} // namespace

std::string make_timestamped_kifu_name() {
    ensure_saves_dir();
    std::time_t now = std::time(nullptr);
    std::tm* local_tm = std::localtime(&now);
    char buf[64];
    if (!local_tm) {
        return "saves/shogi.kifu";
    }
    std::strftime(buf, sizeof(buf), "saves/shogi%Y-%m-%dT%H-%M.kifu", local_tm);
    return std::string(buf);
}

bool open_kifu_dialog(std::string& path_out) {
#ifdef _WIN32
    ensure_saves_dir();
    char file[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "Kifu files (*.kifu)\0*.kifu\0All files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Load kifu";
    ofn.lpstrInitialDir = "saves";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameA(&ofn)) {
        return false;
    }
    path_out = file;
    return true;
#else
    (void)path_out;
    return false;
#endif
}

} // namespace ui
} // namespace shogi
