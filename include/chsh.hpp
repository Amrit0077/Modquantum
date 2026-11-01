// CHSH Bell-inequality test on |Phi+> built on Quantum++ (qpp).
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mq {

struct ChshTrial {
    int trial = 0;
    std::uint32_t seed = 0;
    // E[0]=E(a,b)  E[1]=E(a,b')  E[2]=E(a',b)  E[3]=E(a',b')
    std::array<double, 4> E{};
    double S = 0.0;  // S = E(a,b) + E(a,b') + E(a',b) - E(a',b')
};

struct ChshConfig {
    int trials = 100;
    int shots = 2000;          // shots PER SETTING PAIR
    std::uint32_t seed0 = 0;   // trial i uses seed seed0 + i
    double a = 0.0, a2 = 90.0; // Alice's angles (degrees)
    double b = 45.0, b2 = -45.0; // Bob's angles (degrees)
};

// One trial: prepares |Phi+>, samples `shots` outcomes of O(x) (x) O(y) for each
// of the four setting pairs and returns the correlators and S.
ChshTrial run_chsh_trial(const ChshConfig& cfg, int trial);

// Ideal (theoretical) S for the configured angles on |Phi+>.
double chsh_theory(const ChshConfig& cfg);

// Runs all trials, writes <outdir>/chsh_trials.csv and <outdir>/chsh_summary.txt
int run_chsh(const ChshConfig& cfg, const std::string& outdir);

} // namespace mq
