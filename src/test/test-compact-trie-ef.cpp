// Randomized test of compact_trie_ef against compact_trie: access, append_children,
// binary_search_seek with and without cursor, LTJ-like chains of leaps and serialization.
// Usage: test-compact-trie-ef [threshold]   (8, 16, 64 or 1024; all by default)
#include <trie/cltj_compact_trie.hpp>
#include <trie/cltj_compact_trie_ef.hpp>
#include <random>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>
template<uint64_t T>
void run(uint64_t &checks, uint64_t &errors, std::mt19937_64 &rng, int trials) {
    for (int trial = 0; trial < trials; ++trial) {
        std::vector<uint32_t> syms; std::vector<uint64_t> lengths;
        uint64_t n1 = 1 + rng() % (T >= 1024 ? 6 : 30), n2nodes = 0;
        auto add_node = [&](uint64_t d, uint64_t U) {
            std::vector<uint32_t> v;
            if (rng() % 4 == 0) { uint32_t b = 1 + rng() % U; for (uint64_t j = 0; j < d; ++j) v.push_back(b + j); }
            else { if (U < 4 * d) U = 4 * d; std::set<uint32_t> st; while (st.size() < d) st.insert(1 + rng() % U); v.assign(st.begin(), st.end()); }
            std::sort(v.begin(), v.end()); syms.insert(syms.end(), v.begin(), v.end()); lengths.push_back(d);
        };
        auto deg = [&]() -> uint64_t { uint64_t r = rng() % 10; return r < 5 ? 1 + rng() % 4 : (r < 8 ? T - 2 + rng() % 5 : 1 + rng() % (8 * T)); };
        for (uint64_t k = 0; k < n1; ++k) { uint64_t d = deg(); add_node(d, (rng() % 2) ? 5000 : (1u << 26)); n2nodes += d; }
        uint64_t n3 = std::min<uint64_t>(n2nodes, T >= 1024 ? 400 : n2nodes);
        for (uint64_t k = 0; k < n2nodes; ++k) add_node(k < n3 ? deg() : 1, (rng() % 2) ? 300 + 16 * T : (1u << 28));
        cltj::compact_trie a(syms, lengths, n1);
        cltj::compact_trie_ef_t<T> e0(syms, lengths, n1);
        std::stringstream ss; e0.serialize(ss); cltj::compact_trie_ef_t<T> e; e.load(ss);
        uint64_t beg = 0;
        for (auto d : lengths) {
            uint64_t end = beg + d - 1;
            // access with and without degree/cursor, and append_children
            std::vector<uint64_t> ch; e.append_children(beg, d, ch);
            ++checks; if (ch.size() != d) ++errors;
            for (uint64_t p = beg; p <= end; ++p) {
                uint64_t cur = ~0ULL; ++checks;
                if (e.access(p) != syms[p] || e.access(p, d, cur) != syms[p] || ch[p - beg] != syms[p] || a.access(p) != syms[p]) {
                    if (++errors < 5) std::cout << "access p=" << p << "\n";
                }
            }
            // random seeks with unknown cursor
            for (int q = 0; q < 30; ++q) {
                uint64_t i = beg + rng() % d;
                uint32_t val = (rng() % 2) ? syms[beg + rng() % d] + (rng() % 3) - 1 : rng() % (1u << 29);
                uint64_t cur = ~0ULL;
                auto r1 = a.binary_search_seek(val, i, end), r2 = (q & 1) ? e.binary_search_seek(val, i, end, d, cur) : e.binary_search_seek(val, i, end);
                ++checks;
                if (!((r1.second == r2.second) && (r1.second > end || r1.first == r2.first)) && ++errors < 8) std::cout << "seek T=" << T << "\n";
            }
            // LTJ-like: first access, then leaps chaining the cursor as the iterator does
            for (int run = 0; run < 3; ++run) {
                uint64_t cur = ~0ULL, pos = beg;
                uint32_t v = e.access(beg, d, cur);
                while (true) {
                    uint32_t val = v + 1 + ((rng() % 3) ? 0 : rng() % (run == 2 ? 100000 : 50));
                    auto r1 = a.binary_search_seek(val, pos, end);
                    auto r2 = e.binary_search_seek(val, pos, end, d, cur);
                    ++checks;
                    if (!((r1.second == r2.second) && (r1.second > end || r1.first == r2.first))) { if (++errors < 8) std::cout << "chain T=" << T << std::endl; break; }
                    if (r1.second > end) break;
                    pos = r1.second; v = r1.first;
                }
            }
            beg = end + 1;
        }
    }
}
int main(int argc, char **argv) {
    std::mt19937_64 rng(5); uint64_t checks = 0, errors = 0;
    int only = argc > 1 ? atoi(argv[1]) : 0;
    if (!only || only == 8) { run<8>(checks, errors, rng, 150); std::cout << "T=8 " << checks << " " << errors << std::endl; }
    if (!only || only == 16) { run<16>(checks, errors, rng, 150); std::cout << "T=16 " << checks << " " << errors << std::endl; }
    if (!only || only == 64) { run<64>(checks, errors, rng, 40); std::cout << "T=64 " << checks << " " << errors << std::endl; }
    if (!only || only == 1024) { run<1024>(checks, errors, rng, 6); std::cout << "T=1024 " << checks << " " << errors << std::endl; }
    std::cout << checks << " checks, " << errors << " errors\n";
}
