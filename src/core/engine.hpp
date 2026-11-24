#ifndef SHOGI_ENGINE_HPP
#define SHOGI_ENGINE_HPP

/**
 * @file engine.hpp
 * @brief Opponent backends: abstract Engine, USI process, random legal mover.
 */

#include "board.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace shogi {

/**
 * Abstract USI-ish opponent.
 * Concrete engines: Fairy-Stockfish process, or a lightweight random mover.
 * @note Hierarchy uses @c override / @c final for C++11 clarity.
 */
class Engine {
public:
    virtual ~Engine() {}

    /** Human-readable name for status lines. */
    virtual char const* name() const = 0;

    /** Prepare / handshake. Throws @c std::runtime_error on hard failure. */
    virtual void start() = 0;
    virtual void shutdown() = 0;
    virtual bool running() const = 0;

    /** Ask for a move from SFEN; result via @ref poll_bestmove. */
    virtual void go_sfen(std::string const& sfen, int movetime_ms) = 0;

    /** Non-blocking: true if a bestmove (or resign) arrived. */
    virtual bool poll_bestmove(std::string& usi_or_resign) = 0;

    virtual void stop_search() = 0;
};

/**
 * External USI engine (Fairy-Stockfish) over stdin/stdout.
 * A reader thread fills the bestmove under a mutex; the UI thread only polls / sends.
 */
class UsiEngine final : public Engine {
public:
    /**
     * @param exe_path Empty → @ref default_engine_path.
     * @param skill_level Fairy-Stockfish Skill_Level option (0..20-ish).
     */
    explicit UsiEngine(std::string exe_path = std::string(), int skill_level = 5);
    ~UsiEngine() override;

    UsiEngine(UsiEngine const&) = delete;
    UsiEngine& operator=(UsiEngine const&) = delete;

    char const* name() const override { return "Fairy-Stockfish (USI)"; }

    void start() override;
    void shutdown() override;
    bool running() const override { return running_; }

    void go_sfen(std::string const& sfen, int movetime_ms) override;
    bool poll_bestmove(std::string& usi_or_resign) override;
    void stop_search() override;

private:
    void writer_send(std::string const& line);
    void reader_loop();
    void close_handles();

    std::string exe_path_;
    int skill_level_;
    bool running_;
    std::atomic<bool> stop_reader_;
    std::thread reader_;

    std::mutex mu_;
    std::condition_variable cv_;
    bool have_bestmove_;
    std::string bestmove_;
    bool thinking_;

#ifdef _WIN32
    HANDLE proc_;
    HANDLE stdin_wr_;
    HANDLE stdout_rd_;
#endif
};

/**
 * Joke / demo engine: picks among legal moves with a pluggable picker.
 * Useful for difficulty stubs without spawning a process.
 */
class RandomEngine final : public Engine {
public:
    /** Maps legal-move count → chosen index. */
    using MovePicker = std::function<std::size_t(std::size_t /*count*/)>;

    /** @param picker Empty → uniform @c rand() pick. */
    explicit RandomEngine(MovePicker picker = MovePicker());

    char const* name() const override { return "Random legal moves"; }

    void start() override;
    void shutdown() override;
    bool running() const override { return running_; }

    void go_sfen(std::string const& sfen, int movetime_ms) override;
    bool poll_bestmove(std::string& usi_or_resign) override;
    void stop_search() override;

private:
    MovePicker picker_;
    bool running_;
    bool have_bestmove_;
    std::string bestmove_;
};

/** Opponent backends the menu can pick. */
enum class EngineKind {
    FairyStockfish, ///< Real USI process
    RandomLegal     ///< Uniform random among legal moves
};

/** Default Fairy-Stockfish relative path under @c engine/. */
char const* default_engine_path();

/**
 * Build an Engine for the chosen kind.
 * @param kind      FairyStockfish or RandomLegal.
 * @param exe_path Optional override for FairyStockfish.
 */
std::unique_ptr<Engine> make_engine(EngineKind kind,
                                    std::string const& exe_path = std::string());

/** Same as @ref make_engine (@ref EngineKind::FairyStockfish). */
std::unique_ptr<Engine> make_default_engine();

} // namespace shogi

#endif
