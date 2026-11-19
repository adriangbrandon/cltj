//
// Characterization test: the static cltj_ids typedefs must return the same
// number of results as a naive evaluator (nested loops over the triples).
//
// Dataset: std::mt19937 with a fixed seed, ids >= 1, contiguous predicates 1..P,
// no duplicated triples. Queries never repeat a variable inside a triple pattern.
//

#include <api/cltj_ids.hpp>
#include <unistd.h>
#include <random>
#include <set>
#include <sstream>
#include <cstdio>

namespace {

    const uint32_t SEED = 42;
    const uint32_t N_TRIPLES = 600;
    const uint32_t MAX_ID = 60;   // maximum subject/object id
    const uint32_t N_PRED = 8;    // predicates 1..N_PRED

    typedef std::array<uint32_t, 3> triple_t;

    // A term is a constant (> 0) or a variable (< 0, -1 is ?v1, ...)
    struct pattern {
        int64_t t[3];
    };
    struct query_t {
        std::string name;
        std::vector<pattern> pats;
    };

    std::string to_string(const query_t &q) {
        std::ostringstream oss;
        for (size_t i = 0; i < q.pats.size(); ++i) {
            if (i) oss << " . ";
            for (int j = 0; j < 3; ++j) {
                if (j) oss << " ";
                if (q.pats[i].t[j] < 0) oss << "?v" << -q.pats[i].t[j];
                else oss << q.pats[i].t[j];
            }
        }
        return oss.str();
    }

    uint64_t naive_rec(const std::vector<triple_t> &D, const query_t &q, size_t k,
                       std::vector<int64_t> &bind) {
        if (k == q.pats.size()) return 1;
        uint64_t cnt = 0;
        for (const auto &d : D) {
            std::vector<std::pair<size_t, int64_t>> assigned;
            bool ok = true;
            for (int j = 0; j < 3 && ok; ++j) {
                int64_t t = q.pats[k].t[j];
                if (t > 0) {
                    ok = ((int64_t) d[j] == t);
                } else {
                    size_t v = (size_t) (-t);
                    if (bind[v] == 0) {
                        bind[v] = d[j];
                        assigned.emplace_back(v, 0);
                    } else {
                        ok = (bind[v] == (int64_t) d[j]);
                    }
                }
            }
            if (ok) cnt += naive_rec(D, q, k + 1, bind);
            for (auto &a : assigned) bind[a.first] = 0;
        }
        return cnt;
    }

    uint64_t naive(const std::vector<triple_t> &D, const query_t &q) {
        std::vector<int64_t> bind(16, 0);
        return naive_rec(D, q, 0, bind);
    }

    query_t Q(const std::string &name, std::vector<pattern> pats) {
        query_t q;
        q.name = name;
        q.pats = pats;
        return q;
    }

    std::vector<query_t> make_queries() {
        const int64_t vx = -1, vy = -2, vz = -3, vw = -4, vp = -5;
        const int64_t big = MAX_ID + 1000;
        std::vector<query_t> qs;
        qs.push_back(Q("all variables", {{{vx, vp, vy}}}));
        qs.push_back(Q("constant predicate", {{{vx, 3, vy}}}));
        qs.push_back(Q("constant subject", {{{5, vp, vy}}}));
        qs.push_back(Q("constant object", {{{vx, vp, 7}}}));
        qs.push_back(Q("all constants (present)", {{{1, 1, 1}}}));
        qs.push_back(Q("path 2 (failing in master)", {{{vx, 1, vy}, {vy, 2, vz}}}));
        qs.push_back(Q("path 3", {{{vx, 1, vy}, {vy, 2, vz}, {vz, 3, vw}}}));
        qs.push_back(Q("triangle", {{{vx, 1, vy}, {vy, 2, vz}, {vz, 3, vx}}}));
        qs.push_back(Q("star by subject (failing in master)", {{{vx, 1, vy}, {vx, 2, vz}, {vx, 3, vw}}}));
        qs.push_back(Q("star by object", {{{vx, 1, vz}, {vy, 2, vz}, {vw, 3, vz}}}));
        qs.push_back(Q("shared variable predicate", {{{vx, vp, vy}, {vz, vp, vy}}}));
        qs.push_back(Q("constant subject 5 (failing in master)", {{{5, vp, vy}}}));
        qs.push_back(Q("2 patterns with constants", {{{5, vp, vy}, {vy, 2, vz}}}));
        // edge cases
        qs.push_back(Q("subject > max id", {{{big, vp, vy}}}));
        qs.push_back(Q("object > max id", {{{vx, vp, big}}}));
        qs.push_back(Q("subject = max id", {{{(int64_t) MAX_ID, vp, vy}}}));
        qs.push_back(Q("object = max id", {{{vx, vp, (int64_t) MAX_ID}}}));
        qs.push_back(Q("predicate P+1", {{{vx, (int64_t) N_PRED + 1, vy}}}));
        qs.push_back(Q("predicate P+1 in a join", {{{vx, 1, vy}, {vy, (int64_t) N_PRED + 1, vz}}}));
        qs.push_back(Q("predicate P (max)", {{{vx, (int64_t) N_PRED, vy}}}));
        qs.push_back(Q("all constants (absent)", {{{MAX_ID, N_PRED + 1, 1}}}));
        qs.push_back(Q("empty result (constant > max id in join)", {{{vx, 1, vy}, {vy, 2, big}}}));
        return qs;
    }

    std::vector<triple_t> make_dataset() {
        std::mt19937 rng(SEED);
        std::set<triple_t> S;
        // Make sure every predicate 1..P and the maximum id appear.
        for (uint32_t p = 1; p <= N_PRED; ++p) S.insert({{1 + p % MAX_ID, p, 1 + (p * 7) % MAX_ID}});
        S.insert({{MAX_ID, 1, 1}});
        S.insert({{1, 1, 1}});
        S.insert({{2, 2, MAX_ID}});
        std::uniform_int_distribution<uint32_t> id(1, MAX_ID), pr(1, N_PRED);
        while (S.size() < N_TRIPLES) S.insert({{id(rng), pr(rng), id(rng)}});
        return std::vector<triple_t>(S.begin(), S.end());
    }

    int failures = 0;

    template<class Ids>
    void run_case(const std::string &label, Ids &idx, const std::vector<triple_t> &D,
                  const std::vector<query_t> &qs) {
        typedef typename Ids::tuple_type tuple_type;
        const uint64_t limits[2] = {0, 1000};
        for (const auto &q : qs) {
            uint64_t nv = naive(D, q);
            for (uint64_t limit : limits) {
                ::util::results_collector<tuple_type> res;
                idx.query(to_string(q), res, limit, 600);
                uint64_t expected = (limit == 0) ? nv : std::min<uint64_t>(nv, limit);
                bool ok = (res.size() == expected);
                std::cout << (ok ? "OK   " : "FAIL ") << label << " | " << q.name << " | limit=" << limit
                          << " | got=" << res.size() << " expected=" << expected << std::endl;
                if (!ok) ++failures;
            }
        }
    }

    template<class Ids>
    void run_type(const std::string &label, const std::string &dataset,
                  const std::vector<triple_t> &D, const std::vector<query_t> &qs, bool roundtrip) {
        Ids idx(dataset);
        run_case<Ids>(label, idx, D, qs);
        if (roundtrip) {
            char tmpl[] = "/tmp/test-ids-static-idx-XXXXXX";
            int fd = mkstemp(tmpl);
            if (fd < 0) { std::cout << "FAIL cannot create temp file" << std::endl; ++failures; return; }
            close(fd);
            sdsl::store_to_file(idx, tmpl);
            Ids loaded;
            sdsl::load_from_file(loaded, tmpl);
            std::remove(tmpl);
            run_case<Ids>(label + " (after store/load)", loaded, D, qs);
        }
    }
}

int main() {
    std::vector<triple_t> D = make_dataset();
    std::vector<query_t> qs = make_queries();

    char tmpl[] = "/tmp/test-ids-static-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) { std::cerr << "cannot create temp file" << std::endl; return 2; }
    close(fd);
    {
        std::ofstream ofs(tmpl);
        for (const auto &d : D) ofs << d[0] << " " << d[1] << " " << d[2] << "\n";
    }
    std::string dataset = tmpl;
    std::cout << "seed=" << SEED << " triples=" << D.size() << " max_id=" << MAX_ID
              << " predicates=1.." << N_PRED << std::endl;

    run_type<cltj::cltj_ids_static>("cltj_ids_static", dataset, D, qs, true);
    run_type<cltj::cltj_ids_static_global>("cltj_ids_static_global", dataset, D, qs, false);
    run_type<cltj::xcltj_ids_static>("xcltj_ids_static", dataset, D, qs, true);
    run_type<cltj::xcltj_ids_static_global>("xcltj_ids_static_global", dataset, D, qs, false);

    std::remove(tmpl);
    std::cout << (failures == 0 ? "ALL OK" : "FAILURES: ") ;
    if (failures) std::cout << failures;
    std::cout << std::endl;
    return failures == 0 ? 0 : 1;
}
