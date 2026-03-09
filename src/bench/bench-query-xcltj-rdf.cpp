/*
 * query-index.cpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <utility>
#include <chrono>
#include <index/cltj_index_metatrie_dyn.hpp>
#include <triple_pattern.hpp>
#include <query/ltj_algorithm.hpp>
#include <dict/dict_map.hpp>
#include <util/time_util.hpp>
#include <results/results_collector.hpp>

using namespace std;

//#include<chrono>
//#include<ctime>

using namespace ::util::time;


template<class index_scheme_type, class trait_type, class map_type>
void query(const std::string &file, const std::string &queries, const uint64_t limit) {
    vector<string> dummy_queries;
    bool result = ::util::file::get_file_content(queries, dummy_queries);

    index_scheme_type graph;
    dict::basic_map dict_so;
    dict::basic_map dict_p;

    std::string index_name = file + ".cltj.idx";
    std::string dict_so_name = file + ".cltj.so";
    std::string dict_p_name = file + ".cltj.p";
    sdsl::load_from_file(graph, index_name);
    std::cout << "Index loaded : " << sdsl::size_in_bytes(graph) << " bytes." << std::endl;
    sdsl::load_from_file(dict_so, dict_so_name);
    std::cout << "DictSO loaded: " << sdsl::size_in_bytes(dict_so) << " bytes." << std::endl;
    sdsl::load_from_file(dict_p, dict_p_name);
    std::cout << "DictP loaded : " << sdsl::size_in_bytes(dict_p) << " bytes." << std::endl;

    std::ifstream ifs;
    uint64_t nQ = 0;

    //::util::time::usage::usage_type start, stop;
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
    if (result) {
        int count = 1;
        for (string &query_string: dummy_queries) {
            //vector<Term*> terms_created;
            //vector<Triple*> query;
            results_type res;
            std::vector<std::vector<std::string>> res_str;
            std::unordered_map<std::string, uint8_t> hash_table_vars;
            std::vector<bool> vars_in_p;
            std::vector<ltj::triple_pattern> query;

            auto t0 = std::chrono::high_resolution_clock::now();
            std::vector<cltj::user_triple> tokens = ::util::rdf::str::get_query(query_string);
            for (cltj::user_triple &token: tokens) {
                auto p = ::util::rdf::str::get_triple_pattern(token, hash_table_vars,
                    vars_in_p, dict_so, dict_p);
                //assumes the query is correct
                query.push_back(p.second);
            }
            auto t1 = std::chrono::high_resolution_clock::now();
            algorithm_type ltj(&query, &graph);
            ltj.join(res, limit, 600);
            auto t2 = std::chrono::high_resolution_clock::now();
            std::vector<string> tmp_str(hash_table_vars.size());
            uint64_t a = results_type::buckets;
            auto l = std::min(a, res.size());
            res_str.reserve(l);
            for(uint64_t i = 0; i < l; ++i) {
                const auto &tuple = res[i];
                for(uint64_t j = 0; j < tuple.size(); ++j) {
                    auto id = tuple[j].first;
                    if(vars_in_p[id]) {
                        tmp_str[id] = dict_p.extract(tuple[j].second);
                    }else {
                        tmp_str[id] = dict_so.extract(tuple[j].second);
                    }
                }
                res_str.push_back(tmp_str);
            }
            auto t3 = std::chrono::high_resolution_clock::now();
            auto str_to_id = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            auto run = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
            auto id_to_str = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
            auto total = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t0).count();
            cout << nQ << ";" << res.size() << ";" << str_to_id << ";" << run << ";" << id_to_str << ";" << total << endl;
            nQ++;
            //Reset caches in dictionary
            dict_so.reset_cache();
            dict_p.reset_cache();
            // cout << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;

            //cout << "RESULTS QUERY " << count << ": " << number_of_results << endl;
            count += 1;
        }
    }
}


int main(int argc, char *argv[]) {
    //typedef ring::c_ring ring_type;
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <dataset> <queries> <limit> <type>" << std::endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string queries = argv[2];
    uint64_t limit = std::atoll(argv[3]);
    std::string type = argv[4];

    if (type == "normal") {
        query<cltj::compact_ltj_metatrie_dyn, ltj::util::trait_distinct, dict::basic_map>(dataset, queries, limit);
    } else if (type == "star") {
        query<cltj::compact_ltj_metatrie_dyn, ltj::util::trait_size, dict::basic_map>(dataset, queries, limit);
    } else {
        std::cout << "Type of index: " << type << " is not supported." << std::endl;
    }


    return 0;
}

