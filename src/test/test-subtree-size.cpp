/*
 * test-subtree-size.cpp
 *
 * Tests for the weights of trait_size (the "star" mode of the VEO), in particular
 * subtree_size_fixed1: with one fixed component, the weight of any free variable is the number of
 * triples whose fixed component has the fixed value. Compares with a brute-force count for the static
 * indexes (compact_ltj + ltj_iterator_lite, compact_ltj_metatrie + ltj_iterator_metatrie) and, as a
 * reference, for the dynamic ones. Also checks subtree_size_fixed2 and the results of ltj_algorithm
 * against a naive evaluator. Self-contained: datasets are generated in memory with a fixed seed.
 * Returns != 0 if any check fails.
 */

#include <iostream>
#include <random>
#include <set>
#include <map>
#include <string>
#include <api/cltj_ids.hpp>

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

typedef std::vector<cltj::spo_triple> data_type;
typedef std::vector<ltj::triple_pattern> query_type;

static data_type to_data(const std::set<cltj::spo_triple> &S) {
    return data_type(S.begin(), S.end());
}

//Random dataset: predicates 1..n_preds (all present), entities 1..n_ent; a few entities (1..3 and
//n_ent-2..n_ent) have a high degree and appear as the first/last value of each component
static data_type gen_random(std::mt19937 &gen, uint32_t n_triples, uint32_t n_ent, uint32_t n_preds) {
    std::set<cltj::spo_triple> S;
    std::uniform_int_distribution<uint32_t> ent(1, n_ent), pred(1, n_preds), hub(0, 5), coin(0, 99);
    auto pick = [&]() -> uint32_t {
        if (coin(gen) < 20) { //hubs
            uint32_t h = hub(gen);
            return h < 3 ? h + 1 : n_ent - (h - 3);
        }
        return ent(gen);
    };
    for (uint32_t p = 1; p <= n_preds; ++p) S.insert({{ent(gen), p, ent(gen)}});
    S.insert({{1, 1, n_ent}});
    S.insert({{n_ent, n_preds, 1}});
    while (S.size() < n_triples) S.insert({{pick(), pred(gen), pick()}});
    return to_data(S);
}

//Edge cases (predicates are 1..P contiguous in all of them)
static data_type gen_same_subject() {
    std::set<cltj::spo_triple> S;
    for (uint32_t p = 1; p <= 4; ++p)
        for (uint32_t o = 3; o <= 3 + p; ++o) S.insert({{7, p, o}});
    return to_data(S);
}

static data_type gen_all_single() { //every value of every component has exactly one triple (P = n)
    std::set<cltj::spo_triple> S;
    for (uint32_t i = 1; i <= 6; ++i) S.insert({{i, i, 100 + i}});
    return to_data(S);
}

//Sparse ids (up to 2^24) so that the rank/succ structures of the first level are exercised
static data_type gen_sparse(std::mt19937 &gen) {
    std::set<cltj::spo_triple> S;
    const uint32_t big = (1u << 24) - 1;
    uint32_t pool[12] = {1, 2, 3, 40, 977, 5000, 65535, 65536, 1000003, 8388608, big - 1, big};
    std::uniform_int_distribution<uint32_t> pick(0, 11), pred(1, 3);
    for (uint32_t p = 1; p <= 3; ++p) S.insert({{pool[pick(gen)], p, pool[pick(gen)]}});
    S.insert({{1, 1, big}});
    S.insert({{big, 3, 1}});
    while (S.size() < 70) S.insert({{pool[pick(gen)], pred(gen), pool[pick(gen)]}});
    return to_data(S);
}

//A single predicate (P = 1): stars and chains
static data_type gen_one_pred(std::mt19937 &gen) {
    std::set<cltj::spo_triple> S;
    std::uniform_int_distribution<uint32_t> ent(1, 15);
    for (uint32_t i = 1; i < 15; ++i) S.insert({{i, 1, i + 1}}); //chain
    for (uint32_t i = 2; i <= 15; ++i) S.insert({{1, 1, i}});    //star
    while (S.size() < 50) S.insert({{ent(gen), 1, ent(gen)}});
    return to_data(S);
}

static data_type gen_two_triples() {
    std::set<cltj::spo_triple> S;
    S.insert({{1, 1, 2}});
    S.insert({{2, 2, 1}});
    return to_data(S);
}

//Naive evaluator (see test-timeout-period)
static uint64_t naive_rec(const query_type &q, size_t i, const data_type &D, std::vector<uint64_t> &bind) {
    if (i == q.size()) return 1;
    uint64_t cnt = 0;
    const ltj::triple_pattern &tp = q[i];
    for (const auto &t : D) {
        const bool isv[3] = {tp.s_is_variable(), tp.p_is_variable(), tp.o_is_variable()};
        const uint64_t val[3] = {tp.term_s.value, tp.term_p.value, tp.term_o.value};
        std::vector<uint64_t> assigned;
        bool ok = true;
        for (int k = 0; k < 3 && ok; ++k) {
            if (!isv[k]) {
                ok = (t[k] == val[k]);
            } else if (bind[val[k]] == 0) {
                bind[val[k]] = t[k];
                assigned.push_back(val[k]);
            } else {
                ok = (bind[val[k]] == t[k]);
            }
        }
        if (ok) cnt += naive_rec(q, i + 1, D, bind);
        for (auto a : assigned) bind[a] = 0;
    }
    return cnt;
}

static uint64_t naive_count(const query_type &q, const data_type &D) {
    std::vector<uint64_t> bind(256, 0);
    return naive_rec(q, 0, D, bind);
}

//Accumulates comparisons of a group, printing only the first mismatches
struct group {
    uint64_t total = 0, bad = 0;
    void compare(uint64_t got, uint64_t expected, const std::string &ctx) {
        ++total;
        if (got != expected) {
            if (++bad <= 3) std::cout << "  " << ctx << ": got " << got << " expected " << expected << std::endl;
        }
    }
    void report(const std::string &what) const {
        check(total > 0 && bad == 0, what + " (" + std::to_string(total - bad) + "/" + std::to_string(total)
                                     + " comparisons)");
    }
};

static const char *COMP[3] = {"s", "p", "o"};

//Weight of the free component k (0=s, 1=p, 2=o) as seen by the VEO
template<class Iterator>
static uint64_t weight(Iterator &it, int k) {
    if (k == 0) return ltj::util::trait_size::subject(it);
    if (k == 1) return ltj::util::trait_size::predicate(it);
    return ltj::util::trait_size::object(it);
}

static uint64_t count_eq(const data_type &D, int c, uint64_t v) {
    uint64_t n = 0;
    for (const auto &t : D) if (t[c] == v) ++n;
    return n;
}

static std::vector<uint64_t> values_of(const data_type &D, int c) {
    std::set<uint64_t> V;
    for (const auto &t : D) V.insert(t[c]);
    return std::vector<uint64_t>(V.begin(), V.end());
}

//Case 1: one constant given in the pattern (fixed by process_constants)
template<class Index, class Iterator>
static void case1(const std::string &name, Index &index, const data_type &D) {
    group g;
    for (int c = 0; c < 3; ++c) {
        for (uint64_t v : values_of(D, c)) {
            std::string str;
            if (c == 0) str = std::to_string(v) + " ?x ?y";
            else if (c == 1) str = "?x " + std::to_string(v) + " ?y";
            else str = "?x ?y " + std::to_string(v);
            query_type q = ::util::rdf::ids::get_query(str);
            Iterator it(&q[0], &index);
            const std::string ctx = str + " (" + COMP[c] + "=" + std::to_string(v) + ")";
            g.compare(it.nfixed, 1, ctx + " nfixed");
            const uint64_t expected = count_eq(D, c, v);
            for (int k = 0; k < 3; ++k) {
                if (k == c) continue;
                g.compare(weight(it, k), expected, ctx + " weight of " + COMP[k]);
            }
        }
    }
    g.report(name + " case 1: subtree_size_fixed1 with a constant == #triples");
}

//Case 2: the component is fixed by down() after a leap
template<class Index, class Iterator>
static void case2(const std::string &name, Index &index, const data_type &D) {
    group g;
    query_type q = ::util::rdf::ids::get_query("?x ?y ?z");
    const uint8_t vars[3] = {(uint8_t) q[0].term_s.value, (uint8_t) q[0].term_p.value,
                             (uint8_t) q[0].term_o.value};
    for (int c = 0; c < 3; ++c) {
        Iterator it(&q[0], &index);
        std::set<uint64_t> seen;
        uint64_t cur = -1ULL;
        while (true) {
            const uint64_t v = it.leap(vars[c], cur);
            if (v == 0) break;
            if (!seen.insert(v).second) { g.compare(0, 1, "leap does not advance"); break; }
            it.down(vars[c], v);
            const std::string ctx = std::string("?x ?y ?z, down ") + COMP[c] + "=" + std::to_string(v);
            g.compare(it.nfixed, 1, ctx + " nfixed");
            const uint64_t expected = count_eq(D, c, v);
            for (int k = 0; k < 3; ++k) {
                if (k == c) continue;
                g.compare(weight(it, k), expected, ctx + " weight of " + COMP[k]);
            }
            it.up(vars[c]);
            cur = v + 1;
        }
        const auto vals = values_of(D, c);
        g.compare(seen.size(), vals.size(), std::string("number of ") + COMP[c] + " values visited");
    }
    g.report(name + " case 2: subtree_size_fixed1 after down == #triples");
}

//Case 3: two constants (subtree_size_fixed2)
template<class Index, class Iterator>
static void case3(const std::string &name, Index &index, const data_type &D) {
    group g;
    for (int free_c = 0; free_c < 3; ++free_c) {
        std::set<std::pair<uint64_t, uint64_t>> pairs;
        int a = free_c == 0 ? 1 : 0, b = free_c == 2 ? 1 : 2;
        for (const auto &t : D) pairs.insert({t[a], t[b]});
        for (const auto &pr : pairs) {
            std::string term[3];
            term[free_c] = "?x";
            term[a] = std::to_string(pr.first);
            term[b] = std::to_string(pr.second);
            const std::string str = term[0] + " " + term[1] + " " + term[2];
            query_type q = ::util::rdf::ids::get_query(str);
            Iterator it(&q[0], &index);
            uint64_t expected = 0;
            for (const auto &t : D) if (t[a] == pr.first && t[b] == pr.second) ++expected;
            g.compare(it.nfixed, 2, str + " nfixed");
            g.compare(weight(it, free_c), expected, str + " weight of " + COMP[free_c]);
        }
    }
    g.report(name + " case 3: subtree_size_fixed2 with two constants == #triples");
}

//Case 5: one component fixed (level 1) and the iterator has changed state in that level, as the adaptive
//VEO does: after a leap/seek on a free variable (choose_trie changes m_trie_i, maybe to an odd trie) and
//after going down to level 2 and back up
template<class Index, class Iterator>
static void case5(const std::string &name, Index &index, const data_type &D) {
    group g;
    for (int c = 0; c < 3; ++c) {
        for (uint64_t v : values_of(D, c)) {
            std::string str;
            if (c == 0) str = std::to_string(v) + " ?x ?y";
            else if (c == 1) str = "?x " + std::to_string(v) + " ?y";
            else str = "?x ?y " + std::to_string(v);
            query_type q = ::util::rdf::ids::get_query(str);
            const uint64_t expected = count_eq(D, c, v);
            const uint8_t vars[3] = {(uint8_t) q[0].term_s.value, (uint8_t) q[0].term_p.value,
                                     (uint8_t) q[0].term_o.value};
            for (int f = 0; f < 3; ++f) { //free component whose variable is moved
                if (f == c) continue;
                const std::string ctx = str + " (" + COMP[c] + "=" + std::to_string(v) + ") on " + COMP[f];
                //a) leap to the minimum, then seek to the next value, checking the weights each time
                {
                    Iterator it(&q[0], &index);
                    const uint64_t v1 = it.leap(vars[f]);
                    g.compare(v1 != 0, true, ctx + " leap finds a value");
                    for (int k = 0; k < 3; ++k)
                        if (k != c) g.compare(weight(it, k), expected, ctx + " after leap, weight of " + COMP[k]);
                    if (v1 != 0) {
                        it.leap(vars[f], v1 + 1);
                        for (int k = 0; k < 3; ++k)
                            if (k != c) g.compare(weight(it, k), expected, ctx + " after seek, weight of " + COMP[k]);
                    }
                    g.compare(it.nfixed, 1, ctx + " nfixed after leap");
                }
                //b) leap, down to level 2, up to level 1
                {
                    Iterator it(&q[0], &index);
                    const uint64_t v1 = it.leap(vars[f]);
                    if (v1 != 0) {
                        it.down(vars[f], v1);
                        it.up(vars[f]);
                    }
                    g.compare(it.nfixed, 1, ctx + " nfixed after down/up");
                    for (int k = 0; k < 3; ++k)
                        if (k != c) g.compare(weight(it, k), expected, ctx + " after down/up, weight of " + COMP[k]);
                }
            }
        }
    }
    g.report(name + " case 5: subtree_size_fixed1 after leap/seek and down/up == #triples");
}

static const char *QUERIES[] = {
    "?x 1 ?y . ?y 2 ?z",
    "?x 1 ?y . ?y 2 ?z . ?z 1 ?x",
    "?x 1 ?y . ?x 2 ?z . ?x 1 ?w",
    "?x 1 ?y . ?z 2 ?y . ?w 1 ?y",
    "1 ?p ?o",
    "?s 2 2",
    "1 1 ?o",
    "?x ?p ?y . ?y ?q ?z",
    "?x ?p ?y . ?z ?p ?y",
    "?x 1 ?y . ?y 2 ?x",
    "?x 2 999",
    "?x 1 ?y . ?y 2 999"
};

//Case 4: results of ltj_algorithm with trait_size vs naive
template<class Index, class Iterator, class Veo>
static void case4(const std::string &name, const data_type &D) {
    typedef ltj::ltj_algorithm<Iterator, Veo> algorithm_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;
    data_type D0 = D; //the index constructor reorders D
    Index index(D0);
    group g;
    uint64_t nonzero = 0, zero = 0;
    for (const char *qs : QUERIES) {
        query_type q = ::util::rdf::ids::get_query(qs);
        const uint64_t expected = naive_count(q, D);
        if (expected > 0) ++nonzero; else ++zero;
        for (int variant = 0; variant < 2; ++variant) {
            results_type res;
            algorithm_type ltj(&q, &index);
            if (variant == 0) ltj.join(res, 0, 600); else ltj.join_v2(res, 0, 600);
            g.compare(res.size(), expected, std::string(qs) + (variant == 0 ? " join" : " join_v2"));
        }
    }
    check(nonzero > 0 && zero > 0, name + " case 4: test queries include empty and non-empty results");
    g.report(name + " case 4: results == naive");
}

template<class Index, class Iterator, class VeoA, class VeoS>
static void run_all(const std::string &name, const std::string &dname, const data_type &D) {
    const std::string full = name + "/" + dname;
    data_type D0 = D; //the index constructor reorders D
    Index index(D0);
    case1<Index, Iterator>(full, index, D);
    case2<Index, Iterator>(full, index, D);
    case3<Index, Iterator>(full, index, D);
    case5<Index, Iterator>(full, index, D);
    case4<Index, Iterator, VeoA>(full + "/adaptive", D);
    case4<Index, Iterator, VeoS>(full + "/simple", D);
}

int main() {
    std::cout << "seed = " << SEED << std::endl;
    std::mt19937 gen(SEED);

    std::vector<std::pair<std::string, data_type>> datasets;
    datasets.push_back({"random2000", gen_random(gen, 2000, 100, 8)});
    datasets.push_back({"random-small", gen_random(gen, 60, 20, 4)});
    datasets.push_back({"same-subject", gen_same_subject()});
    datasets.push_back({"all-single", gen_all_single()});
    datasets.push_back({"two-triples", gen_two_triples()});
    datasets.push_back({"sparse-ids", gen_sparse(gen)});
    datasets.push_back({"one-predicate", gen_one_pred(gen)});

    typedef cltj::compact_ltj lite_index;
    typedef cltj::compact_ltj_metatrie meta_index;
    typedef cltj::compact_dyn_ltj dyn_index;
    typedef cltj::compact_ltj_metatrie_dyn metadyn_index;
    typedef ltj::ltj_iterator_lite<lite_index, uint8_t, uint64_t> it_lite;
    typedef ltj::ltj_iterator_metatrie<meta_index, uint8_t, uint64_t> it_meta;
    typedef ltj::ltj_iterator_dyn<dyn_index, uint8_t, uint64_t> it_dyn;
    typedef ltj::ltj_iterator_metatrie_dyn<metadyn_index, uint8_t, uint64_t> it_metadyn;
    typedef ltj::veo::veo_adaptive<it_lite, ltj::util::trait_size> va_lite;
    typedef ltj::veo::veo_simple<it_lite, ltj::util::trait_size> vs_lite;
    typedef ltj::veo::veo_adaptive<it_meta, ltj::util::trait_size> va_meta;
    typedef ltj::veo::veo_simple<it_meta, ltj::util::trait_size> vs_meta;
    typedef ltj::veo::veo_adaptive<it_dyn, ltj::util::trait_size> va_dyn;
    typedef ltj::veo::veo_simple<it_dyn, ltj::util::trait_size> vs_dyn;
    typedef ltj::veo::veo_adaptive<it_metadyn, ltj::util::trait_size> va_metadyn;
    typedef ltj::veo::veo_simple<it_metadyn, ltj::util::trait_size> vs_metadyn;

    for (const auto &ds : datasets) {
        std::cout << "dataset " << ds.first << ": " << ds.second.size() << " triples" << std::endl;
        run_all<lite_index, it_lite, va_lite, vs_lite>("compact_ltj", ds.first, ds.second);
        run_all<meta_index, it_meta, va_meta, vs_meta>("compact_ltj_metatrie", ds.first, ds.second);
        run_all<dyn_index, it_dyn, va_dyn, vs_dyn>("compact_dyn_ltj", ds.first, ds.second);
        run_all<metadyn_index, it_metadyn, va_metadyn, vs_metadyn>("compact_ltj_metatrie_dyn", ds.first, ds.second);
    }
    //A single triple is not tested: the static indexes cannot be built with it (helper::sym_root reads an
    //uninitialized value) and the dynamic ones return empty results for it (reported separately).

    std::cout << n_ok << " checks OK, " << n_fail << " FAIL" << std::endl;
    return n_fail == 0 ? 0 : 1;
}
