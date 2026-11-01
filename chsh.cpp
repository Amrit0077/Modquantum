#include "chsh.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "qpp/qpp.hpp"

namespace mq {
namespace {

constexpr double kPi = 3.14159265358979323846;

// O(theta) = cos(theta) Z + sin(theta) X
qpp::cmat observable(double deg) {
    const double t = deg * kPi / 180.0;
    return std::cos(t) * qpp::gt.Z + std::sin(t) * qpp::gt.X;
}

// Samples `shots` joint outcomes of O(x) (x) O(y) on the two-qubit state psi
// (Born rule, via qpp::measure) and returns the empirical correlator, i.e. the
// mean of the products of the +/-1 outcomes.
double correlator(const qpp::ket& psi, double xdeg, double ydeg, int shots) {
    using namespace qpp;
    const cmat Ox = observable(xdeg), Oy = observable(ydeg);
    const dyn_col_vect<realT> ex = hevals(Ox), ey = hevals(Oy); // ascending: -1, +1
    const cmat Vx = hevects(Ox), Vy = hevects(Oy);
    const cmat V = kron(Vx, Vy); // joint eigenbasis; column index = 2*s + t
    long long sum = 0;
    for (int i = 0; i < shots; ++i) {
        const idx outcome = std::get<RES>(measure(psi, V));
        const idx s = outcome / 2, t = outcome % 2;
        sum += (ex[s] * ey[t] > 0) ? 1 : -1;
    }
    return static_cast<double>(sum) / static_cast<double>(shots);
}

} // namespace

double chsh_theory(const ChshConfig& cfg) {
    using namespace qpp;
    const ket psi = st.b00; // |Phi+> = (|00> + |11>)/sqrt(2)
    const cmat A = observable(cfg.a), A2 = observable(cfg.a2);
    const cmat B = observable(cfg.b), B2 = observable(cfg.b2);
    auto ex = [&](const cmat& X, const cmat& Y) {
        return (adjoint(psi) * kron(X, Y) * psi).value().real();
    };
    return ex(A, B) + ex(A, B2) + ex(A2, B) - ex(A2, B2);
}

ChshTrial run_chsh_trial(const ChshConfig& cfg, int trial) {
    using namespace qpp;
    ChshTrial r;
    r.trial = trial;
    r.seed = cfg.seed0 + static_cast<std::uint32_t>(trial);
    RandomDevices::get_instance().get_prng().seed(r.seed); // reproducible per-trial seed

    const ket psi = st.b00; // |Phi+>
    r.E[0] = correlator(psi, cfg.a, cfg.b, cfg.shots);
    r.E[1] = correlator(psi, cfg.a, cfg.b2, cfg.shots);
    r.E[2] = correlator(psi, cfg.a2, cfg.b, cfg.shots);
    r.E[3] = correlator(psi, cfg.a2, cfg.b2, cfg.shots);
    r.S = r.E[0] + r.E[1] + r.E[2] - r.E[3];
    return r;
}

int run_chsh(const ChshConfig& cfg, const std::string& outdir) {
    std::filesystem::create_directories(outdir);
    const double theory = chsh_theory(cfg);
    std::cout << ">> CHSH test on |Phi+>: " << cfg.trials << " trials, " << cfg.shots
              << " shots per setting pair (" << 4 * cfg.shots << " per trial, "
              << static_cast<long long>(cfg.trials) * 4 * cfg.shots << " in total)\n"
              << ">> Angles: a=" << cfg.a << " a'=" << cfg.a2 << " b=" << cfg.b << " b'=" << cfg.b2
              << " (deg), seeds " << cfg.seed0 << ".." << cfg.seed0 + cfg.trials - 1 << '\n'
              << ">> Theoretical S (from the state) = " << std::setprecision(6) << theory
              << ", 2*sqrt(2) = " << 2 * std::sqrt(2.0) << '\n';

    const auto t0 = std::chrono::steady_clock::now();
    std::vector<ChshTrial> res;
    res.reserve(cfg.trials);
    std::ofstream csv(outdir + "/chsh_trials.csv");
    csv << "trial,seed,E_ab,E_abp,E_apb,E_apbp,S,verdict\n" << std::fixed << std::setprecision(6);
    for (int i = 0; i < cfg.trials; ++i) {
        ChshTrial t = run_chsh_trial(cfg, i);
        res.push_back(t);
        csv << t.trial << ',' << t.seed << ',' << t.E[0] << ',' << t.E[1] << ',' << t.E[2] << ','
            << t.E[3] << ',' << t.S << ',' << (t.S > 2.0 ? "ENTANGLED" : "NOT_ENTANGLED") << '\n';
        std::cout << "trial " << std::setw(3) << i << "  S = " << std::fixed << std::setprecision(4)
                  << t.S << (t.S > 2.0 ? "  ENTANGLED" : "  NOT ENTANGLED") << '\n';
    }
    const double secs =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    // summary
    const int n = static_cast<int>(res.size());
    double mean = 0;
    for (auto& t : res) mean += t.S;
    mean /= n;
    double var = 0;
    for (auto& t : res) var += (t.S - mean) * (t.S - mean);
    const double sd = n > 1 ? std::sqrt(var / (n - 1)) : 0.0;
    auto mn = std::min_element(res.begin(), res.end(),
                               [](auto& x, auto& y) { return x.S < y.S; });
    auto mx = std::max_element(res.begin(), res.end(),
                               [](auto& x, auto& y) { return x.S < y.S; });
    int c2 = 0, c280 = 0, c285 = 0;
    std::array<double, 4> mE{};
    for (auto& t : res) {
        c2 += t.S > 2.0;
        c280 += t.S > 2.80;
        c285 += t.S > 2.85;
        for (int k = 0; k < 4; ++k) mE[k] += t.E[k] / n;
    }
    const double tsir = 2 * std::sqrt(2.0);
    std::ostringstream o;
    o << std::fixed << std::setprecision(4);
    o << "CHSH summary\n"
      << "trials                   " << n << '\n'
      << "shots_per_setting_pair   " << cfg.shots << '\n'
      << "total_shots              " << static_cast<long long>(n) * 4 * cfg.shots << '\n'
      << "seed_range               " << cfg.seed0 << ".." << cfg.seed0 + n - 1 << '\n'
      << "theory_S                 " << theory << '\n'
      << "mean_S                   " << mean << '\n'
      << "sd_S (n-1)               " << sd << '\n'
      << "sem_S                    " << sd / std::sqrt(static_cast<double>(n)) << '\n'
      << "min_S                    " << mn->S << " (trial " << mn->trial << ")\n"
      << "max_S                    " << mx->S << " (trial " << mx->trial << ")\n"
      << "tsirelson_2sqrt2         " << tsir << '\n'
      << "mean_S_minus_tsirelson   " << mean - tsir << '\n'
      << "trials_S_gt_2            " << c2 << '/' << n << '\n'
      << "trials_S_gt_2.80         " << c280 << '/' << n << '\n'
      << "trials_S_gt_2.85         " << c285 << '/' << n << '\n'
      << "mean_E(a,b)              " << mE[0] << '\n'
      << "mean_E(a,b')             " << mE[1] << '\n'
      << "mean_E(a',b)             " << mE[2] << '\n'
      << "mean_E(a',b')            " << mE[3] << '\n'
      << "runtime_seconds          " << std::setprecision(2) << secs << '\n';
    std::cout << '\n' << o.str();
    std::ofstream(outdir + "/chsh_summary.txt") << o.str();
    return c2 == n ? 0 : 1;
}

} // namespace mq
