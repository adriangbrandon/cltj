//
// Created by adrian on 19/12/24.
//

#include <cltj_config.hpp>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <triple_pattern.hpp>
#include <util/file_util.hpp>
#include <util/rdf_util.hpp>
#include <query/ltj_algorithm.hpp>
#include <index/cltj_index_metatrie_dyn.hpp>

typedef struct {
    cltj::spo_triple triple;
    bool insert; //true => insert; false => delete
} update_type;

typedef std::vector<ltj::triple_pattern> query_type;

//updates per query >= 1
template<class index_scheme_type, class trait_type>
void query_upq(const std::string &index, const std::vector<query_type> &queries, const std::vector<update_type> &updates,
           const uint64_t limit, const uint64_t upq) { //upq = updates per query

    typedef ltj::ltj_iterator_metatrie_dyn<index_scheme_type, uint8_t, uint64_t> iterator_type;
#if ADAPTIVE
    typedef ltj::ltj_algorithm<iterator_type,
        ltj::veo::veo_adaptive<iterator_type, trait_type> > algorithm_type;

#else
    typedef ltj::ltj_algorithm<iterator_type,
            ltj::veo::veo_simple<iterator_type, trait_type>> algorithm_type;
#endif
    //typedef std::vector<typename algorithm_type::tuple_type> results_type;
    //typedef algorithm_type::results_type results_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;

    index_scheme_type graph;
    sdsl::load_from_file(graph, index);
    std::cout << "Index loaded : " << sdsl::size_in_bytes(graph) << " bytes." << std::endl;

    uint64_t i_up = 0; //index of update
    uint64_t nQ = 0;
    for(auto &q : queries) {
        //Before each query run upq updates
        for(uint64_t i = 0; i < upq; ++i) {
            const auto &u = updates[i_up];
            if(u.insert) {
                graph.insert(u.triple);
            }else {
                graph.remove(u.triple);
            }
            ++i_up;
        }

        results_type res;

        auto start = std::chrono::high_resolution_clock::now();
        algorithm_type ltj(&q, &graph);
        ltj.join(res, limit, 600);
        auto stop = std::chrono::high_resolution_clock::now();

        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        cout << nQ << ";" << res.size() << ";" << time << endl;
        nQ++;
    }
}

//updates per query < 1 (upq)
//qpu = 1/upq
template<class index_scheme_type, class trait_type>
void test(const std::string &index, const std::vector<update_type> &updates) { //qpu = queries per update

    typedef ltj::ltj_iterator_metatrie_dyn<index_scheme_type, uint8_t, uint64_t> iterator_type;
#if ADAPTIVE
    typedef ltj::ltj_algorithm<iterator_type,
        ltj::veo::veo_adaptive<iterator_type, trait_type> > algorithm_type;

#else
    typedef ltj::ltj_algorithm<iterator_type,
            ltj::veo::veo_simple<iterator_type, trait_type>> algorithm_type;
#endif
    //typedef std::vector<typename algorithm_type::tuple_type> results_type;
    //typedef algorithm_type::results_type results_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;


    index_scheme_type graph;
    sdsl::load_from_file(graph, index);
    std::cout << "Index loaded : " << sdsl::size_in_bytes(graph) << " bytes." << std::endl;

    uint64_t i_up = 0; //index of update
    uint64_t nQ = 0;
    std::cout << "Updates: " << updates.size() << std::endl;
    for(const auto &u : updates) {
        //After upq queries we run an update
        std::cout << "Update " << i_up << std::endl;
        if(u.insert) {
            graph.insert(u.triple);
            if(!graph.check()) {
                std::cout << "Check false after inserting D[" << i_up << "]=(" << u.triple[0] << ", " << u.triple[1] << ", " << u.triple[2] << ")" << std::endl;
                exit(0);
            }
            /*if(graph.test_exists(u.triple) == 0) {
                std::cout << "Error inserting D[" << i_up << "]=(" << u.triple[0] << ", " << u.triple[1] << ", " << u.triple[2] << ") appears in "
                          << "0 tries." << std::endl;
                exit(0);
            }*/

        }else {
            graph.remove(u.triple);
            if(!graph.check()) {
                std::cout << "Check false after removing D[" << i_up << "]=(" << u.triple[0] << ", " << u.triple[1] << ", " << u.triple[2] << ")" << std::endl;
                exit(0);
            }
           /* if(graph.test_exists(u.triple) > 0) {
                std::cout << "Error removing D[" << i_up << "]=(" << u.triple[0] << ", " << u.triple[1] << ", " << u.triple[2] << ") appears in "
                          << "0 tries." << std::endl;
                exit(0);
            }*/
        }
        ++i_up;
    }
}

void add_queries(const std::string &from, std::vector<query_type> &queries) {
    std::vector<std::string> str_queries;
    bool result = ::util::file::get_file_content(from, str_queries);
    if(result) {
        for(const auto &q : str_queries) {
            std::unordered_map<std::string, uint8_t> hash_table_vars;
            query_type query;
            std::vector<std::string> tokens_query = ::util::rdf::tokenizer(q, '.');
            for (std::string &token: tokens_query) {
                auto triple_pattern = ::util::rdf::ids::get_triple_pattern(token, hash_table_vars);
                query.push_back(triple_pattern);
            }
            queries.push_back(query);
        }
    }
}

void add_updates(const std::string &from, std::vector<update_type> &updates) {
    std::vector<std::string> lines;
    bool result = ::util::file::get_file_content(from, lines);
    if(result) {
        for(uint64_t i = 0; i < lines.size(); i+=2) {
            update_type update;
            update.insert = (lines[i] == "Insert");
            update.triple = ::util::rdf::ids::get_triple(lines[i+1]);
            updates.push_back(update);
        }
    }
}

int main(int argc, char **argv) {
    if(argc != 3){
        std::cout<< argv[0] << " <index> <updates>" << std::endl;
        return 0;
    }

    std::string index   = argv[1];
    std::string updates = argv[2];

    std::cout << "===================" << std::endl;
    std::cout << "Index: " << index << std::endl;
    std::cout << "Updates: " << updates << std::endl;
    std::cout << "===================" << std::endl << std::endl;

    std::vector<query_type> qs;
    std::vector<update_type> us;
    add_updates(updates, us);
    test<cltj::compact_ltj_metatrie_dyn, ltj::util::trait_size>(index, us);
}



