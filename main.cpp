// modquantum: CHSH Bell test and BB84 QKD simulations built on Quantum++ (qpp).
//   modquantum chsh [--trials N] [--shots N] [--seed S] [--out DIR]
//   modquantum qkd  [--qubits N] [--sample K] [--eve P] [--flip P] [--threshold T]
//                   [--sessions M] [--seed S] [--out DIR]
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "bb84.hpp"
#include "chsh.hpp"

static void usage() {
    std::cout <<
        "usage:\n"
        "  modquantum chsh [--trials 100] [--shots 2000] [--seed 0] [--out results]\n"
        "  modquantum qkd  [--qubits 100] [--sample 10] [--eve 0] [--flip 0] [--threshold 0.11]\n"
        "                  [--sessions 1] [--seed 0] [--out results]\n"
        "shots = measurement shots PER SETTING PAIR; seeds are seed, seed+1, ... one per trial/session\n";
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 2; }
    const std::string cmd = argv[1];
    std::string out = "results";
    mq::ChshConfig cc;
    mq::Bb84Config bc;
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](const char* name) -> const char* {
            if (i + 1 >= argc) { std::cerr << "missing value for " << name << '\n'; std::exit(2); }
            return argv[++i];
        };
        if (a == "--out") out = next("--out");
        else if (a == "--trials") cc.trials = std::atoi(next("--trials"));
        else if (a == "--shots") cc.shots = std::atoi(next("--shots"));
        else if (a == "--seed") { auto s = static_cast<std::uint32_t>(std::strtoul(next("--seed"), nullptr, 10)); cc.seed0 = s; bc.seed0 = s; }
        else if (a == "--qubits") bc.qubits = std::atoi(next("--qubits"));
        else if (a == "--sample") bc.sample = std::atoi(next("--sample"));
        else if (a == "--eve") bc.eve = std::atof(next("--eve"));
        else if (a == "--flip") bc.flip = std::atof(next("--flip"));
        else if (a == "--threshold") bc.abort_threshold = std::atof(next("--threshold"));
        else if (a == "--sessions") bc.sessions = std::atoi(next("--sessions"));
        else { std::cerr << "unknown option " << a << '\n'; usage(); return 2; }
    }
    if (cmd == "chsh") return mq::run_chsh(cc, out);
    if (cmd == "qkd") return mq::run_bb84(bc, out);
    usage();
    return 2;
}
