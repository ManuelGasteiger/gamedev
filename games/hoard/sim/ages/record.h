#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "data.h"
#include "state.h"

namespace ages {

constexpr int kRulesVersion = 6;    // bump with every deliberate golden change
constexpr int kReplayFormat = 1;

// every state-changing API call, recorded once at the outermost call (nested calls inside it are not inputs)
struct Recorder {
    const World* world = nullptr;
    int depth = 0;
    int next = 0;
    std::vector<std::string> lines;   // JSON lines after the header: inputs and checkpoints
};

class RecordScope {
public:
    explicit RecordScope(World& w);
    ~RecordScope();
    RecordScope(const RecordScope&) = delete;
    RecordScope& operator=(const RecordScope&) = delete;
    // the call succeeded: write the input line; a sleep also writes a checkpoint with the state hash
    void done(const char* verb, const std::string& args_json);
private:
    World& w_;
    bool counted_ = false, outer_ = false;
    int year_ = 0;
};

std::string args_of(std::initializer_list<std::pair<const char*, std::string>> strings);
std::string args_int(const char* key, long long v);
std::string args_acts(const std::vector<Act>& acts);

uint64_t data_hash(const DataJson& d);
std::string build_id();
std::string replay_header(const World& w, const Params& p, bool genesis);

// replay one recorded input; false with the reason when the verb is unknown or its call refused
bool apply_input(World& w, const Params& p, const std::string& verb, const std::string& args_json, std::string& err);

// a whole .hrp (JSON lines): loads nothing; the caller built the start world from the header (seed, genesis)
struct ReplayResult {
    bool ok = false;
    int checkpoints = 0;
    int first_bad_year = -1;
    std::string error;
};
ReplayResult replay_lines(World& w, const Params& p, const std::vector<std::string>& lines);

}
