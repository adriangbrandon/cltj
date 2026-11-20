/*
 * test-timeout-period.cpp
 *
 * Tests for the timeout check of ltj_algorithm (join, join_v2, join_str and cltj_ids::query): the clock
 * is read once every ltj_algorithm::timeout_check_period calls to search, so results must not depend on
 * it, and the timeout must still fire. Self-contained: datasets are generated in memory with a fixed
 * seed (the only file written is a small temporary one for the cltj_ids tests).
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
#include <unistd.h>
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

static const uint64_t LIMITS[] = {0, 1, 1000};

template<class Index, class Iterator, class Veo>
static void run_index(const std::string &name, const std::vector<cltj::spo_triple> &data0) {
    typedef ltj::ltj_algorithm<Iterator, Veo> algorithm_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;

    std::vector<cltj::spo_triple> D = data0; //the index constructor reorders D
    Index index(D);

    for (int variant = 0; variant < 2; ++variant) {
        const std::string vname = name + (variant == 0 ? "/join" : "/join_v2");
        auto run = [&](query_type &q, uint64_t limit, uint64_t timeout) {
            results_type res;
            algorithm_type ltj(&q, &index);
            if (variant == 0) ltj.join(res, limit, timeout);
            else ltj.join_v2(res, limit, timeout);
            return (uint64_t) res.size();
        };

        bool same = true, nonzero = false, zero = false;
        for (const char *qs : QUERIES) {
            query_type q = ::util::rdf::ids::get_query(qs);
            const uint64_t expected = naive_count(q, data0);
            if (expected > 0) nonzero = true; else zero = true;
            for (uint64_t limit : LIMITS) {
                const uint64_t exp_l = (limit > 0 && expected > limit) ? limit : expected;
                for (uint64_t timeout : {(uint64_t) 600, (uint64_t) 0}) {
                    const uint64_t r = run(q, limit, timeout);
                    if (r != exp_l) {
                        same = false;
                        std::cout << "  query '" << qs << "' limit " << limit << " timeout " << timeout
                                  << ": got " << r << " expected " << exp_l << std::endl;
                    }
                }
            }
        }
        check(nonzero && zero, vname + ": test queries include empty and non-empty results");
        check(same, vname + ": same results as naive with timeout 600 and 0, limit 0/1/1000");
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
        results_type res;
        algorithm_type ltj(&q, &index);
        auto t0 = std::chrono::steady_clock::now();
        if (variant == 0) ltj.join(res, 0, 1); else ltj.join_v2(res, 0, 1);
        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        check(secs < 4.0 && res.size() > 0,
              name + (variant == 0 ? "/join" : "/join_v2") + " timeout 1s: "
              + std::to_string(secs) + " s, " + std::to_string(res.size()) + " results");
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
        ::util::results_collector<typename Ids::tuple_type> rl;
        ids.query(qs, rl, 1, 600);
        if (rl.size() != (expected > 1 ? 1 : expected)) ok = false;
    }
    check(ok, name + "::query(str, res, limit, timeout) equals naive");
}

//join_str directly on a static index, with in-memory dictionaries (ids i <-> "e<i>" / "p<i>")
template<class Index, class Iterator, class Veo>
static void run_join_str(const std::string &name, const std::vector<cltj::spo_triple> &data0) {
    typedef ltj::ltj_algorithm<Iterator, Veo> algorithm_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_str_type> results_type;
    std::vector<cltj::spo_triple> D = data0;
    Index index(D);
    std::map<std::string, uint64_t> map_so, map_p;
    for (uint64_t i = 1; i <= 61; ++i) map_so.insert({"e" + std::to_string(i), i});
    for (uint64_t i = 1; i <= 5; ++i) map_p.insert({"p" + std::to_string(i), i});
    dict::basic_map dso(map_so), dp(map_p);

    bool same = true;
    for (const char *qs : QUERIES) {
        query_type q = ::util::rdf::ids::get_query(qs);
        const uint64_t expected = naive_count(q, data0);
        std::vector<bool> in_p(256, false);
        for (const auto &tp : q) if (tp.p_is_variable()) in_p[tp.term_p.value] = true;
        for (uint64_t timeout : {(uint64_t) 600, (uint64_t) 0}) {
            results_type res;
            algorithm_type ltj(&q, &index);
            ltj.join_str(res, in_p, dso, dp, 0, timeout);
            if (res.size() != expected) {
                same = false;
                std::cout << "  query '" << qs << "' timeout " << timeout << ": got " << res.size()
                          << " expected " << expected << std::endl;
            }
        }
    }
    check(same, name + "/join_str: same results as naive with timeout 600 and 0");
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

    run_index<cltj::compact_ltj, it_lite, veo_a_lite>("compact_ltj/adaptive", D);
    run_index<cltj::compact_ltj, it_lite, veo_s_lite>("compact_ltj/simple", D);
    run_index<cltj::compact_ltj_metatrie, it_meta, veo_a_meta>("compact_ltj_metatrie/adaptive", D);
    run_index<cltj::compact_ltj_metatrie, it_meta, veo_s_meta>("compact_ltj_metatrie/simple", D);

    run_timeout<cltj::compact_ltj, it_lite, veo_a_lite>("compact_ltj/adaptive", Dbig);
    run_timeout<cltj::compact_ltj, it_lite, veo_s_lite>("compact_ltj/simple", Dbig);
    run_timeout<cltj::compact_ltj_metatrie, it_meta, veo_a_meta>("compact_ltj_metatrie/adaptive", Dbig);
    run_timeout<cltj::compact_ltj_metatrie, it_meta, veo_s_meta>("compact_ltj_metatrie/simple", Dbig);

    //API with ids (needs a file with the dataset)
    const std::string file = tmp_name();
    {
        std::ofstream ofs(file);
        for (const auto &t : D) ofs << t[0] << " " << t[1] << " " << t[2] << "\n";
    }
    run_api_ids<cltj::cltj_ids_static>("cltj_ids_static", file, D);
    run_api_ids<cltj::cltj_ids_static_global>("cltj_ids_static_global", file, D);
    run_api_ids<cltj::xcltj_ids_static>("xcltj_ids_static", file, D);
    run_api_ids<cltj::xcltj_ids_static_global>("xcltj_ids_static_global", file, D);
    std::remove(file.c_str());

    //join_str on static indexes with in-memory dictionaries
    run_join_str<cltj::compact_ltj, it_lite, veo_a_lite>("compact_ltj/adaptive", D);
    run_join_str<cltj::compact_ltj_metatrie, it_meta, veo_a_meta>("compact_ltj_metatrie/adaptive", D);

    std::cout << n_ok << " checks OK, " << n_fail << " FAIL" << std::endl;
    return n_fail == 0 ? 0 : 1;
}
