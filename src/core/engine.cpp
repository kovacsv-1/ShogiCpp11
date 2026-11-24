#include "engine.hpp"

#include "learn.hpp"
#include "rules.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <vector>

namespace shogi {
namespace {

void trim_cr(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
        s.pop_back();
    }
}

std::size_t default_random_pick(std::size_t count) {
    if (count == 0) {
        return 0;
    }
    return static_cast<std::size_t>(std::rand()) % count;
}

} // namespace

char const* default_engine_path() {
    return "engine/fairy-stockfish-largeboard_x86-64-modern.exe";
}

std::unique_ptr<Engine> make_engine(EngineKind kind, std::string const& exe_path) {
    if (kind == EngineKind::RandomLegal) {
        return std::unique_ptr<Engine>(new RandomEngine());
    }
    return std::unique_ptr<Engine>(new UsiEngine(
        exe_path.empty() ? default_engine_path() : exe_path, 5));
}

std::unique_ptr<Engine> make_default_engine() {
    return make_engine(EngineKind::FairyStockfish);
}

// --- UsiEngine -----------------------------------------------------------------

UsiEngine::UsiEngine(std::string exe_path, int skill_level)
    : exe_path_(exe_path.empty() ? default_engine_path() : std::move(exe_path)),
      skill_level_(skill_level),
      running_(false),
      stop_reader_(false),
      have_bestmove_(false),
      thinking_(false)
#ifdef _WIN32
      ,
      proc_(nullptr),
      stdin_wr_(nullptr),
      stdout_rd_(nullptr)
#endif
{
}

UsiEngine::~UsiEngine() {
    shutdown();
}

void UsiEngine::close_handles() {
#ifdef _WIN32
    if (stdin_wr_) {
        CloseHandle(stdin_wr_);
        stdin_wr_ = nullptr;
    }
    if (stdout_rd_) {
        CloseHandle(stdout_rd_);
        stdout_rd_ = nullptr;
    }
    if (proc_) {
        TerminateProcess(proc_, 0);
        CloseHandle(proc_);
        proc_ = nullptr;
    }
#endif
}

void UsiEngine::shutdown() {
    if (!running_ && !reader_.joinable()) {
        return;
    }
    stop_reader_ = true;
    try {
        writer_send("quit");
    } catch (...) {
    }
#ifdef _WIN32
    if (proc_) {
        WaitForSingleObject(proc_, 500);
    }
#endif
    if (reader_.joinable()) {
        reader_.join();
    }
    close_handles();
    running_ = false;
    thinking_ = false;
}

void UsiEngine::writer_send(std::string const& line) {
#ifdef _WIN32
    if (!stdin_wr_) {
        throw std::runtime_error("engine stdin closed");
    }
    std::string payload = line + "\n";
    DWORD written = 0;
    if (!WriteFile(stdin_wr_, payload.data(), static_cast<DWORD>(payload.size()), &written,
                   nullptr)) {
        throw std::runtime_error("engine write failed");
    }
#else
    (void)line;
    throw std::runtime_error("USI engine only implemented on Windows");
#endif
}

void UsiEngine::reader_loop() {
#ifdef _WIN32
    std::string buf;
    char chunk[256];
    while (!stop_reader_) {
        DWORD n = 0;
        BOOL ok = ReadFile(stdout_rd_, chunk, sizeof(chunk), &n, nullptr);
        if (!ok || n == 0) {
            break;
        }
        buf.append(chunk, chunk + n);
        for (;;) {
            std::size_t pos = buf.find('\n');
            if (pos == std::string::npos) {
                break;
            }
            std::string line = buf.substr(0, pos);
            buf.erase(0, pos + 1);
            trim_cr(line);
            if (line.compare(0, 9, "bestmove ") == 0) {
                std::string rest = line.substr(9);
                std::size_t sp = rest.find(' ');
                std::string mv = (sp == std::string::npos) ? rest : rest.substr(0, sp);
                {
                    std::lock_guard<std::mutex> lock(mu_);
                    bestmove_ = mv;
                    have_bestmove_ = true;
                    thinking_ = false;
                }
                cv_.notify_all();
            }
        }
    }
#endif
}

void UsiEngine::start() {
    shutdown();
    stop_reader_ = false;
    have_bestmove_ = false;
    thinking_ = false;

#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE stdin_rd = nullptr;
    HANDLE stdout_wr = nullptr;
    if (!CreatePipe(&stdin_rd, &stdin_wr_, &sa, 0)) {
        throw std::runtime_error("CreatePipe stdin failed");
    }
    if (!SetHandleInformation(stdin_wr_, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(stdin_rd);
        close_handles();
        throw std::runtime_error("SetHandleInformation stdin failed");
    }
    if (!CreatePipe(&stdout_rd_, &stdout_wr, &sa, 0)) {
        CloseHandle(stdin_rd);
        close_handles();
        throw std::runtime_error("CreatePipe stdout failed");
    }
    if (!SetHandleInformation(stdout_rd_, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(stdin_rd);
        CloseHandle(stdout_wr);
        close_handles();
        throw std::runtime_error("SetHandleInformation stdout failed");
    }

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdin_rd;
    si.hStdOutput = stdout_wr;
    si.hStdError = stdout_wr;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::string cmd = "\"" + exe_path_ + "\"";
    std::vector<char> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back('\0');

    BOOL created =
        CreateProcessA(nullptr, cmd_buf.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                       nullptr, &si, &pi);
    CloseHandle(stdin_rd);
    CloseHandle(stdout_wr);
    if (!created) {
        close_handles();
        throw std::runtime_error("failed to start engine: " + exe_path_);
    }
    CloseHandle(pi.hThread);
    proc_ = pi.hProcess;

    reader_ = std::thread(&UsiEngine::reader_loop, this);
    running_ = true;

    writer_send("usi");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    writer_send("setoption name Threads value 1");
    writer_send("setoption name Hash value 64");
    writer_send("setoption name Skill_Level value " + std::to_string(skill_level_));
    writer_send("isready");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
#else
    throw std::runtime_error("USI engine only implemented on Windows");
#endif
}

void UsiEngine::go_sfen(std::string const& sfen, int movetime_ms) {
    if (!running_) {
        throw std::runtime_error("engine not running");
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        have_bestmove_ = false;
        bestmove_.clear();
        thinking_ = true;
    }
    writer_send("position sfen " + sfen);
    writer_send("go movetime " + std::to_string(movetime_ms < 50 ? 50 : movetime_ms));
}

bool UsiEngine::poll_bestmove(std::string& usi_or_resign) {
    std::lock_guard<std::mutex> lock(mu_);
    if (!have_bestmove_) {
        return false;
    }
    usi_or_resign = bestmove_;
    have_bestmove_ = false;
    return true;
}

void UsiEngine::stop_search() {
    if (!running_) {
        return;
    }
    try {
        writer_send("stop");
    } catch (...) {
    }
}

// --- RandomEngine --------------------------------------------------------------

RandomEngine::RandomEngine(MovePicker picker)
    : picker_(picker ? std::move(picker) : MovePicker(default_random_pick)),
      running_(false),
      have_bestmove_(false) {
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }
}

void RandomEngine::start() {
    running_ = true;
    have_bestmove_ = false;
    bestmove_.clear();
}

void RandomEngine::shutdown() {
    running_ = false;
    have_bestmove_ = false;
}

void RandomEngine::go_sfen(std::string const& sfen, int /*movetime_ms*/) {
    if (!running_) {
        throw std::runtime_error("RandomEngine not running");
    }
    Board board;
    Color side = Color::Sente;
    if (!learn::parse_sfen(sfen, board, side)) {
        bestmove_ = "resign";
        have_bestmove_ = true;
        return;
    }
    std::vector<Move> legal = Rules::generate_legal(board, side);
    if (legal.empty()) {
        bestmove_ = "resign";
        have_bestmove_ = true;
        return;
    }
    std::size_t idx = picker_(legal.size());
    if (idx >= legal.size()) {
        idx = legal.size() - 1;
    }
    bestmove_ = learn::to_usi_move(board, legal[idx]);
    have_bestmove_ = true;
}

bool RandomEngine::poll_bestmove(std::string& usi_or_resign) {
    if (!have_bestmove_) {
        return false;
    }
    usi_or_resign = bestmove_;
    have_bestmove_ = false;
    return true;
}

void RandomEngine::stop_search() {
    // Instant engine — nothing to cancel.
}

} // namespace shogi
