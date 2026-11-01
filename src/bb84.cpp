#include "bb84.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>

#include "qpp/qpp.hpp"

namespace mq {
namespace {

// Measures a single-qubit state in the Z (basis 0) or X (basis 1) basis.
// Returns the outcome bit and (optionally) the post-measurement state.
int measure_in_basis(const qpp::ket& q, int basis) {
    using namespace qpp;
    const cmat V = (basis == 0) ? cmat(gt.Id2) : cmat(gt.H); // columns: |0>,|1>  or  |+>,|->
    return static_cast<int>(std::get<RES>(measure(q, V)));
}

// Prepares the BB84 state for (bit, basis): |0>,|1> in Z, |+>,|-> in X.
qpp::ket prepare(int bit, int basis) {
    using namespace qpp;
    if (basis == 0) return bit == 0 ? st.z0 : st.z1;
    return bit == 0 ? st.x0 : st.x1;
}

bool coin(double p) {
    return p > 0 && std::uniform_real_distribution<double>(0.0, 1.0)(
                        qpp::RandomDevices::get_instance().get_prng()) < p;
}

} // namespace

Bb84Result run_bb84_session(const Bb84Config& cfg, std::uint32_t seed, std::string* log) {
    using namespace qpp;
    RandomDevices::get_instance().get_prng().seed(seed);
    const int n = cfg.qubits;
    std::vector<int> aBit(n), aBasis(n), bBasis(n), bRes(n);
    std::ostringstream tab;
    tab << "idx  A.bit A.basis  B.basis B.result  sifted\n";
    for (int i = 0; i < n; ++i) {
        aBit[i] = static_cast<int>(randidx(0, 1));
        aBasis[i] = static_cast<int>(randidx(0, 1));
        bBasis[i] = static_cast<int>(randidx(0, 1));

        ket q = prepare(aBit[i], aBasis[i]);               // Alice -> channel
        if (coin(cfg.eve)) {                               // Eve: intercept & resend
            const int eb = static_cast<int>(randidx(0, 1));
            const int er = measure_in_basis(q, eb);
            q = prepare(er, eb);
        }
        if (coin(cfg.flip)) q = gt.X * q;                  // bit-flip channel error
        bRes[i] = measure_in_basis(q, bBasis[i]);          // Bob measures
        tab << std::setw(3) << i << "  " << std::setw(5) << aBit[i] << "  "
            << (aBasis[i] ? 'X' : 'Z') << "        " << (bBasis[i] ? 'X' : 'Z') << "       "
            << std::setw(5) << bRes[i] << "     " << (aBasis[i] == bBasis[i] ? "yes" : "no") << '\n';
    }

    // Sifting: keep positions where the bases agree.
    std::vector<int> sifted;
    for (int i = 0; i < n; ++i)
        if (aBasis[i] == bBasis[i]) sifted.push_back(i);

    Bb84Result r;
    r.sifted = static_cast<int>(sifted.size());
    r.sample = std::min(cfg.sample, r.sifted);

    // QBER estimation on a random subset of the sifted positions.
    std::vector<int> order(sifted);
    std::shuffle(order.begin(), order.end(), RandomDevices::get_instance().get_prng());
    std::vector<char> used(n, 0);
    for (int k = 0; k < r.sample; ++k) {
        const int pos = order[k];
        used[pos] = 1;
        r.sample_errors += (aBit[pos] != bRes[pos]);
    }
    r.qber_est = r.sample > 0 ? static_cast<double>(r.sample_errors) / r.sample : 0.0;
    r.aborted = r.qber_est > cfg.abort_threshold;

    // Raw key: remaining sifted bits (no error correction / privacy amplification).
    for (int pos : sifted)
        if (!used[pos]) {
            ++r.raw_key_len;
            r.raw_key_errors += (aBit[pos] != bRes[pos]);
        }
    if (r.aborted) { r.raw_key_len = 0; }

    if (log) {
        std::ostringstream o;
        if (cfg.verbose) o << tab.str() << '\n';
        o << "qubits sent               " << n << '\n'
          << "eve_intercept_prob        " << cfg.eve << '\n'
          << "bit_flip_prob             " << cfg.flip << '\n'
          << "sifted key length         " << r.sifted << '\n'
          << "QBER sample size          " << r.sample << '\n'
          << "errors in sample          " << r.sample_errors << '\n'
          << "QBER estimate             " << std::fixed << std::setprecision(4) << r.qber_est << '\n'
          << "abort threshold           " << cfg.abort_threshold << '\n'
          << "verdict                   " << (r.aborted ? "ABORT (QBER above threshold)"
                                                        : "PROCEED (QBER not above threshold)") << '\n'
          << "raw key length            " << r.raw_key_len << '\n'
          << "errors in raw key (truth) " << r.raw_key_errors << '\n'
          << "NOTE: no error correction, privacy amplification or authentication is simulated;\n"
          << "      the raw key is NOT a secure key.\n";
        *log = o.str();
    }
    return r;
}

int run_bb84(const Bb84Config& cfg, const std::string& outdir) {
    std::filesystem::create_directories(outdir);
    if (cfg.sessions <= 1) {
        std::string log;
        run_bb84_session(cfg, cfg.seed0, &log);
        std::cout << ">> BB84 single session (seed " << cfg.seed0 << ")\n" << log;
        std::ofstream(outdir + "/qkd_session.txt") << log;
        return 0;
    }
    Bb84Config c = cfg;
    c.verbose = false;
    std::ostringstream tag;
    tag << "eve" << cfg.eve << "_flip" << cfg.flip << "_n" << cfg.qubits << "_k" << cfg.sample;
    std::ofstream csv(outdir + "/qkd_sessions_" + tag.str() + ".csv");
    csv << "session,seed,sifted,sample,sample_errors,qber_est,aborted,raw_key_len,raw_key_errors\n";
    double sSift = 0, sQ = 0, sKey = 0, sTrueQ = 0;
    int aborts = 0, trueN = 0;
    for (int i = 0; i < cfg.sessions; ++i) {
        const std::uint32_t seed = cfg.seed0 + static_cast<std::uint32_t>(i);
        Bb84Result r = run_bb84_session(c, seed, nullptr);
        csv << i << ',' << seed << ',' << r.sifted << ',' << r.sample << ',' << r.sample_errors << ','
            << std::fixed << std::setprecision(4) << r.qber_est << ',' << r.aborted << ','
            << r.raw_key_len << ',' << r.raw_key_errors << '\n';
        sSift += r.sifted; sQ += r.qber_est; aborts += r.aborted; sKey += r.raw_key_len;
        if (!r.aborted && r.raw_key_len > 0) { sTrueQ += static_cast<double>(r.raw_key_errors) / r.raw_key_len; ++trueN; }
    }
    const int N = cfg.sessions;
    std::ostringstream o;
    o << std::fixed << std::setprecision(4)
      << "BB84 ensemble: sessions=" << N << " qubits=" << cfg.qubits << " sample=" << cfg.sample
      << " eve=" << cfg.eve << " flip=" << cfg.flip << " threshold=" << cfg.abort_threshold << '\n'
      << "mean_sifted        " << sSift / N << '\n'
      << "mean_QBER_est      " << sQ / N << '\n'
      << "abort_rate         " << static_cast<double>(aborts) / N << '\n'
      << "mean_raw_key_len   " << sKey / N << '\n'
      << "mean_true_error_in_raw_key (non-aborted) " << (trueN ? sTrueQ / trueN : 0.0) << '\n';
    std::cout << o.str();
    std::ofstream(outdir + "/qkd_summary_" + tag.str() + ".txt") << o.str();
    return 0;
}

} // namespace mq
