//
// Created by adrian on 11/12/24.
//

#include <cltj_config.hpp>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <triple_pattern.hpp>
#include <index/cltj_index_metatrie_dyn.hpp>
#include <util/file_util.hpp>
#include <util/rdf_util.hpp>
#include <query/ltj_algorithm.hpp>

typedef struct {
    cltj::spo_triple triple;
    bool insert; //true => insert; false => delete
} update_type;

typedef std::vector<ltj::triple_pattern> query_type;

//updates per query >= 1
template<class index_scheme_type, class trait_type>
void query_indel(const std::string &index, const std::vector<query_type> &queries, const std::vector<update_type> &updates,
           const uint64_t limit) {

    typedef ltj::ltj_iterator_metatrie_dyn<index_scheme_type, uint8_t, uint64_t> iterator_type;
    typedef ltj::ltj_algorithm<iterator_type,
        ltj::veo::veo_adaptive<iterator_type, trait_type> > algorithm_type;
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
            const auto &u = updates[i_up];
            if(u.insert) {
                auto start = std::chrono::high_resolution_clock::now();
                graph.insert(u.triple);
                auto stop = std::chrono::high_resolution_clock::now();
                auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
                cout << "I" << ";" << i_up << ";" << 0 << ";" << time << endl;

            }else {
                auto start = std::chrono::high_resolution_clock::now();
                graph.remove(u.triple);
                auto stop = std::chrono::high_resolution_clock::now();
                auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
                cout << "D" << ";" << i_up << ";" << 0 << ";" << time << endl;
            }
            ++i_up;

        results_type res;

        auto start = std::chrono::high_resolution_clock::now();
        algorithm_type ltj(&q, &graph);
        ltj.join(res, limit, 600);
        auto stop = std::chrono::high_resolution_clock::now();

        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        cout << "Q" << ";" << nQ << ";" << res.size() << ";" << time << endl;
        nQ++;
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
    if(argc != 5){
        std::cout<< argv[0] << " <index> <queries> <updates> <limit>" << std::endl;
        return 0;
    }

    std::string index   = argv[1];
    std::string queries = argv[2];
    std::string updates = argv[3];
    uint64_t limit      = atoll(argv[4]);


    std::cout << "===================" << std::endl;
    std::cout << "Index: " << index << std::endl;
    std::cout << "Queries: " << queries << std::endl;
    std::cout << "Updates: " << updates << std::endl;
    std::cout << "Limit: " << limit << std::endl;
    std::cout << "===================" << std::endl << std::endl;

    std::vector<query_type> qs;
    std::vector<update_type> us;
    add_queries(queries, qs);
    add_updates(updates, us);
    query_indel<cltj::compact_ltj_metatrie_dyn, ltj::util::trait_size>(index, qs, us, limit);


}