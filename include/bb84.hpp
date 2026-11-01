// BB84 quantum key distribution simulation built on Quantum++ (qpp).
#pragma once
#include <cstdint>
#include <string>

namespace mq {

struct Bb84Config {
    int qubits = 100;          // qubits sent by Alice
    int sample = 10;           // sifted bits sacrificed for QBER estimation
    double abort_threshold = 0.11; // abort if estimated QBER is greater than this
    double eve = 0.0;          // probability that Eve intercepts-and-resends a qubit
    double flip = 0.0;         // probability of a bit-flip (X) channel error per qubit
    int sessions = 1;          // number of independent sessions
    std::uint32_t seed0 = 0;   // session i uses seed seed0 + i
    bool verbose = true;       // per-qubit table (single session only)
};

struct Bb84Result {
    int sifted = 0;
    int sample = 0;
    int sample_errors = 0;
    double qber_est = 0.0;
    bool aborted = false;
    int raw_key_len = 0;       // sifted bits remaining after the QBER sample is removed
    int raw_key_errors = 0;    // ground truth: positions where Alice and Bob differ
};

Bb84Result run_bb84_session(const Bb84Config& cfg, std::uint32_t seed, std::string* log);

// Runs cfg.sessions sessions; writes <outdir>/qkd_session.txt (single session) or
// <outdir>/qkd_sessions.csv and a summary line (many sessions).
int run_bb84(const Bb84Config& cfg, const std::string& outdir);

} // namespace mq
