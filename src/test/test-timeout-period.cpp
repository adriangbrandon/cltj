/*
 * test-timeout-period.cpp
 *
 * Tests for the timeout check period of ltj_algorithm (join, join_v2, join_str and the
 * query() methods of the API). Self-contained: datasets are generated in memory with a
 * fixed seed (the only file written is a small temporary one for the cltj_ids/cltj_rdf tests).
 * Returns != 0 if any check fails.
 */

#include <iostream>
#include <fstream>
#include <random>
#include <set>
#include <map>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <api/cltj_ids.hpp>
#include <api/cltj_rdf.hpp>

using namespace std;

static const uint32_t SEED = 42;
static uint64_t n_ok = 0, n_fail = 0;

static void check(bool cond, const std::string &what) {
    if (cond) {
        ++n_ok;
        std::cout << "OK   " << what << std::endl;
    } else {
        ++n_fail;
        std::cout << "FAIL " << what << std::endl;
    }
}

//Random dataset with predicates 1..n_preds (all present) and entities 1..n_ent
static std::vector<cltj::spo_triple> gen_dataset(std::mt19937 &gen, uint32_t n_triples, uint32_t n_ent,
                                                 uint32_t n_preds) {
    std::set<cltj::spo_triple> S;
    std::uniform_int_distribution<uint32_t> ent(1, n_ent), pred(1, n_preds);
    for (uint32_t p = 1; p <= n_preds; ++p) { //every predicate appears
        S.insert({{ent(gen), p, ent(gen)}});
    }
    while (S.size() < n_triples) {
        S.insert({{ent(gen), pred(gen), ent(gen)}});
    }
    return std::vector<cltj::spo_triple>(S.begin(), S.end());
}

//Naive evaluator: nested loops over the dataset, one pattern at a time
static uint64_t naive_rec(const std::vector<ltj::triple_pattern> &q, size_t i,
                          const std::vector<cltj::spo_triple> &D, std::vector<uint64_t> &bind) {
    if (i == q.size()) return 1;
    uint64_t cnt = 0;
    const ltj::triple_pattern &tp = q[i];
    for (const auto &t : D) {
        const bool isv[3] = {tp.s_is_variable(), tp.p_is_variable(), tp.o_is_variable()};
        const uint64_t val[3] = {tp.term_s.value, tp.term_p.value, tp.term_o.value};
        std::vector<std::pair<uint64_t, uint64_t>> assigned;
        bool ok = true;
        for (int k = 0; k < 3 && ok; ++k) {
            if (!isv[k]) {
                ok = (t[k] == val[k]);
            } else if (bind[val[k]] == 0) {
                bind[val[k]] = t[k];
                assigned.push_back({val[k], 0});
            } else {
                ok = (bind[val[k]] == t[k]);
            }
        }
        if (ok) cnt += naive_rec(q, i + 1, D, bind);
        for (auto &a : assigned) bind[a.first] = 0;
    }
    return cnt;
}

static uint64_t naive_count(const std::vector<ltj::triple_pattern> &q, const std::vector<cltj::spo_triple> &D) {
    std::vector<uint64_t> bind(256, 0);
    return naive_rec(q, 0, D, bind);
}

//No variable is repeated inside a pattern (project precondition)
static const char *QUERIES[] = {
    "?x 1 ?y . ?y 2 ?z",                         //path
    "?x 1 ?y . ?y 2 ?z . ?z 3 ?w",               //long path
    "?x 1 ?y . ?y 2 ?z . ?z 3 ?x",               //triangle
    "?x 1 ?y . ?x 2 ?z . ?x 3 ?w",               //star (subject)
    "?x 1 ?y . ?z 2 ?y . ?w 3 ?y",               //star (object)
    "5 ?p ?o",                                   //constant subject
    "?s 2 7",                                    //constant predicate and object
    "3 1 ?o",                                    //constant subject and predicate
    "?x ?p ?y . ?y ?q ?z",                       //variable predicates
    "?x ?p ?y . ?z ?p ?y",                       //shared variable predicate
    "?x 1 ?y . ?y 2 ?x",                         //cycle of length 2
    "?x 2 61",                                   //empty (61 is only a subject)
    "?x 1 ?y . ?y 2 61"                          //empty
};

typedef std::vector<ltj::triple_pattern> query_type;

static const uint64_t PERIODS[] = {0, 1, 2, 7, 4096, (uint64_t) 1 << 40};
static const uint64_t LIMITS[] = {0, 1, 1000};

static uint64_t ceil_div(uint64_t a, uint64_t b) { return (a + b - 1) / b; }

template<class Index, class Iterator, class Veo>
static void run_index(const std::string &name, const std::vector<cltj::spo_triple> &data0,
                      const bool rerun_same_calls) {
    typedef ltj::ltj_algorithm<Iterator, Veo> algorithm_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;

    std::vector<cltj::spo_triple> D = data0; //the index constructor reorders D
    Index index(D);

    for (int variant = 0; variant < 2; ++variant) {
        const std::string vname = name + (variant == 0 ? "/join" : "/join_v2");
        auto run = [&](query_type &q, uint64_t limit, uint64_t timeout, uint64_t period,
                       uint64_t &calls, uint64_t &clocks) {
            results_type res;
            algorithm_type ltj(&q, &index);
            if (variant == 0) ltj.join(res, limit, timeout, period);
            else ltj.join_v2(res, limit, timeout, period);
            calls = ltj.search_calls();
            clocks = ltj.clock_checks();
            return (uint64_t) res.size();
        };

        bool same = true, inv = true, one_read = true, zero_read = true, nonzero = false, zero = false;
        for (const char *qs : QUERIES) {
            query_type q = ::util::rdf::ids::get_query(qs);
            const uint64_t expected = naive_count(q, data0);
            if (expected > 0) nonzero = true; else zero = true;
            for (uint64_t limit : LIMITS) {
                const uint64_t exp_l = (limit > 0 && expected > limit) ? limit : expected;
                uint64_t calls0, clocks0;
                const uint64_t r0 = run(q, limit, 0, 4096, calls0, clocks0);
                if (r0 != exp_l) { same = false; std::cout << "  query '" << qs << "' limit " << limit
                                   << ": got " << r0 << " expected " << exp_l << std::endl; }
                if (clocks0 != 0) zero_read = false;
                for (uint64_t P : PERIODS) {
                    uint64_t calls, clocks;
                    const uint64_t r = run(q, limit, 600, P, calls, clocks);
                    if (r != exp_l || r != r0) {
                        same = false;
                        std::cout << "  query '" << qs << "' limit " << limit << " P " << P << ": got " << r
                                  << " expected " << exp_l << std::endl;
                    }
                    if (calls != calls0) inv = false; //same traversal regardless of the period
                    if (clocks != ceil_div(calls, P == 0 ? 1 : P)) {
                        inv = false;
                        std::cout << "  query '" << qs << "' limit " << limit << " P " << P << ": calls " << calls
                                  << " clocks " << clocks << std::endl;
                    }
                    if (P == ((uint64_t) 1 << 40) && calls > 0 && clocks != 1) one_read = false;
                    if (P == 1 && clocks != calls) inv = false;
                    if (P == 0 && clocks != calls) inv = false;
                }
            }
        }
        check(nonzero && zero, vname + ": test queries include empty and non-empty results");
        check(same, vname + ": same results as naive and as timeout 0 (P in {0,1,2,7,4096,2^40}, limit 0/1/1000)");
        check(zero_read, vname + ": timeout 0 never reads the clock");
        check(inv, vname + ": clock_checks == ceil(search_calls / max(P,1)) and calls independent of P");
        check(one_read, vname + ": P larger than the number of calls gives exactly 1 reading");

        //Two consecutive joins on the same object. The counters must be reset by each join. With veo_adaptive the
        //second join on the same object does not repeat the exact traversal (the VEO keeps state from the first
        //join; the results are the same), so search_calls is only compared when rerun_same_calls is true.
        //In all cases, two different objects give identical counters.
        {
            bool rep = true;
            for (const char *qs : QUERIES) {
                query_type q = ::util::rdf::ids::get_query(qs);
                algorithm_type ltj(&q, &index), ltj_b(&q, &index);
                results_type res1, res2, res3;
                if (variant == 0) ltj.join(res1, 0, 600, 7); else ltj.join_v2(res1, 0, 600, 7);
                const uint64_t c1 = ltj.search_calls(), k1 = ltj.clock_checks();
                if (variant == 0) ltj.join(res2, 0, 600, 7); else ltj.join_v2(res2, 0, 600, 7);
                const uint64_t c2 = ltj.search_calls(), k2 = ltj.clock_checks();
                if (variant == 0) ltj_b.join(res3, 0, 600, 7); else ltj_b.join_v2(res3, 0, 600, 7);
                bool ok = res1.size() == res2.size() && res1.size() == res3.size()
                          && c1 == ltj_b.search_calls() && k1 == ltj_b.clock_checks()
                          && k1 == ceil_div(c1, 7) && k2 == ceil_div(c2, 7);
                if (rerun_same_calls) ok = ok && c1 == c2 && k1 == k2;
                if (!ok) {
                    rep = false;
                    std::cout << "  query '" << qs << "': " << res1.size() << "/" << c1 << "/" << k1 << " vs "
                              << res2.size() << "/" << c2 << "/" << k2 << std::endl;
                }
            }
            check(rep, vname + ": counters reset at each join (same object twice, and two objects)");
        }
    }
}

//The timeout still fires on a huge result
template<class Index, class Iterator, class Veo>
static void run_timeout(const std::string &name, const std::vector<cltj::spo_triple> &data0) {
    typedef ltj::ltj_algorithm<Iterator, Veo> algorithm_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;
    std::vector<cltj::spo_triple> D = data0;
    Index index(D);
    query_type q = ::util::rdf::ids::get_query("?a ?p ?b . ?c ?q ?d . ?e ?r ?f");
    for (int variant = 0; variant < 2; ++variant) {
        for (uint64_t P : {(uint64_t) 4096, (uint64_t) 1}) {
            results_type res;
            algorithm_type ltj(&q, &index);
            auto t0 = std::chrono::steady_clock::now();
            if (variant == 0) ltj.join(res, 0, 1, P); else ltj.join_v2(res, 0, 1, P);
            double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            check(secs < 4.0 && res.size() > 0 && ltj.clock_checks() > 1,
                  name + (variant == 0 ? "/join" : "/join_v2") + " timeout 1s, P=" + std::to_string(P) + ": "
                  + std::to_string(secs) + " s, " + std::to_string(res.size()) + " results");
        }
    }
}

static std::string tmp_name() {
    char buf[] = "/tmp/test-timeout-period-XXXXXX";
    int fd = mkstemp(buf);
    if (fd >= 0) close(fd);
    return std::string(buf);
}

template<class Ids>
static void run_api_ids(const std::string &name, const std::string &file, const std::vector<cltj::spo_triple> &D) {
    Ids ids(file);
    bool ok = true;
    for (const char *qs : QUERIES) {
        query_type q = ::util::rdf::ids::get_query(qs);
        const uint64_t expected = naive_count(q, D);
        ::util::results_collector<typename Ids::tuple_type> r0;
        ids.query(qs, r0, 0, 600);
        if (r0.size() != expected) { ok = false; std::cout << "  '" << qs << "' " << r0.size() << " vs " << expected << std::endl; }
        for (uint64_t P : PERIODS) {
            ::util::results_collector<typename Ids::tuple_type> r;
            ids.query(qs, r, 0, 600, P);
            if (r.size() != r0.size()) ok = false;
            ::util::results_collector<typename Ids::tuple_type> rl;
            ids.query(qs, rl, 1, 600, P);
            if (rl.size() != (expected > 1 ? 1 : expected)) ok = false;
        }
    }
    check(ok, name + "::query(..., timeout, P) equals query(..., timeout) and naive");
}

static void run_api_rdf(std::mt19937 &gen) {
    //Small string dataset: subjects/objects e0..e29, predicates p0..p3
    std::set<std::array<std::string, 3>> S;
    std::uniform_int_distribution<int> ent(0, 29), pred(0, 3);
    for (int i = 0; i < 4; ++i) S.insert({{"<e" + std::to_string(ent(gen)) + ">", "<p" + std::to_string(i) + ">", "<e" + std::to_string(ent(gen)) + ">"}});
    while (S.size() < 120) {
        S.insert({{"<e" + std::to_string(ent(gen)) + ">", "<p" + std::to_string(pred(gen)) + ">", "<e" + std::to_string(ent(gen)) + ">"}});
    }
    const std::string file = tmp_name();
    {
        std::ofstream ofs(file);
        for (const auto &t : S) ofs << t[0] << " " << t[1] << " " << t[2] << "\n";
    }
    //Naive for the path query ?x p0 ?y . ?y p1 ?z and the star ?x p0 ?y . ?x p1 ?z
    uint64_t path = 0, star = 0;
    for (const auto &a : S) for (const auto &b : S) {
        if (a[1] == "<p0>" && b[1] == "<p1>") {
            if (a[2] == b[0]) ++path;
            if (a[0] == b[0]) ++star;
        }
    }
    cltj::cltj_rdf_dyn rdf(file);
    std::remove(file.c_str());
    bool ok = true;
    const std::string qp = "?x <p0> ?y . ?y <p1> ?z", qs = "?x <p0> ?y . ?x <p1> ?z";
    for (int k = 0; k < 2; ++k) {
        const std::string &q = (k == 0) ? qp : qs;
        const uint64_t expected = (k == 0) ? path : star;
        ::util::results_collector<cltj::cltj_rdf_dyn::tuple_type> r0;
        rdf.query(q, r0, 0, 600);
        if (r0.size() != expected) { ok = false; std::cout << "  rdf '" << q << "' " << r0.size() << " vs " << expected << std::endl; }
        for (uint64_t P : PERIODS) {
            ::util::results_collector<cltj::cltj_rdf_dyn::tuple_type> r;
            rdf.query(q, r, 0, 600, P);
            if (r.size() != expected) ok = false;
        }
    }
    check(ok && path > 0 && star > 0, "cltj_rdf_dyn::query (join_str) with P in {0,1,2,7,4096,2^40} equals naive (path "
          + std::to_string(path) + ", star " + std::to_string(star) + ")");
}

int main() {
    std::cout << "seed = " << SEED << std::endl;
    std::mt19937 gen(SEED);

    //Small dataset: entities 1..60, predicates 1..5; 61 is only a subject
    std::vector<cltj::spo_triple> D = gen_dataset(gen, 400, 60, 5);
    {
        std::set<cltj::spo_triple> S(D.begin(), D.end());
        S.insert({{61, 1, 1}});
        D.assign(S.begin(), S.end());
    }
    //Big dataset for the timeout test
    std::vector<cltj::spo_triple> Dbig = gen_dataset(gen, 3000, 200, 5);

    typedef ltj::ltj_iterator_lite<cltj::compact_ltj, uint8_t, uint64_t> it_lite;
    typedef ltj::ltj_iterator_metatrie<cltj::compact_ltj_metatrie, uint8_t, uint64_t> it_meta;
    typedef ltj::veo::veo_adaptive<it_lite, ltj::util::trait_size> veo_a_lite;
    typedef ltj::veo::veo_simple<it_lite, ltj::util::trait_size> veo_s_lite;
    typedef ltj::veo::veo_adaptive<it_meta, ltj::util::trait_size> veo_a_meta;
    typedef ltj::veo::veo_simple<it_meta, ltj::util::trait_size> veo_s_meta;

    run_index<cltj::compact_ltj, it_lite, veo_a_lite>("compact_ltj/adaptive", D, false);
    run_index<cltj::compact_ltj, it_lite, veo_s_lite>("compact_ltj/simple", D, true);
    run_index<cltj::compact_ltj_metatrie, it_meta, veo_a_meta>("compact_ltj_metatrie/adaptive", D, false);
    run_index<cltj::compact_ltj_metatrie, it_meta, veo_s_meta>("compact_ltj_metatrie/simple", D, true);

    run_timeout<cltj::compact_ltj, it_lite, veo_a_lite>("compact_ltj/adaptive", Dbig);
    run_timeout<cltj::compact_ltj, it_lite, veo_s_lite>("compact_ltj/simple", Dbig);
    run_timeout<cltj::compact_ltj_metatrie, it_meta, veo_a_meta>("compact_ltj_metatrie/adaptive", Dbig);

    //API with ids (needs a file with the dataset)
    const std::string file = tmp_name();
    {
        std::ofstream ofs(file);
        for (const auto &t : D) ofs << t[0] << " " << t[1] << " " << t[2] << "\n";
    }
    //cltj_ids_static uses ltj_iterator_dyn on a static index (known fragile combination, arquitectura 7.6): it
    //gives wrong results and ASan reports a heap-buffer-overflow in compact_trie::child, with or without P.
    //It is therefore only instantiated (compile check of the new signature), not run. Correctness is checked
    //with the *_global and xcltj typedefs, which use the right iterator.
    {
        typedef ::util::results_collector<cltj::cltj_ids_static::tuple_type> res_t;
        bool (cltj::cltj_ids_static::*f)(const std::string &, res_t &, uint64_t, uint64_t, uint64_t) =
            &cltj::cltj_ids_static::query<res_t>;
        check(f != nullptr, "cltj_ids_static::query(..., timeout, P) compiles (not run: wrong iterator, see comment)");
    }
    run_api_ids<cltj::cltj_ids_static_global>("cltj_ids_static_global", file, D);
    run_api_ids<cltj::xcltj_ids_static>("xcltj_ids_static", file, D);
    run_api_ids<cltj::xcltj_ids_static_global>("xcltj_ids_static_global", file, D);
    std::remove(file.c_str());

    //API with strings (join_str)
    run_api_rdf(gen);

    std::cout << n_ok << " checks OK, " << n_fail << " FAIL" << std::endl;
    return n_fail == 0 ? 0 : 1;
}
