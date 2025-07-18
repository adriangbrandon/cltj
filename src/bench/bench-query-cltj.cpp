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
#include <triple_pattern.hpp>
#include <query/ltj_algorithm.hpp>
#include <util/time_util.hpp>
#include <util/file_util.hpp>
#include <index/cltj_index_spo_lite.hpp>

using namespace std;

//#include<chrono>
//#include<ctime>

using namespace ::util::time;


template<class index_scheme_type, class trait_type>
void query(const std::string &file, const std::string &queries, const uint64_t limit, const uint64_t timeout) {
    vector<string> dummy_queries;
    bool result = ::util::file::get_file_content(queries, dummy_queries);

    index_scheme_type graph;
    sdsl::load_from_file(graph, file);

    std::cout << "Index loaded: " << sdsl::size_in_bytes(graph) << " bytes." << std::endl;

    std::ifstream ifs;
    uint64_t nQ = 0;

    //::util::time::usage::usage_type start, stop;
    uint64_t total_elapsed_time;
    uint64_t total_user_time;

    if (result) {
        int count = 1;
        for (string &query_string: dummy_queries) {
            std::vector<ltj::triple_pattern> query = ::util::rdf::ids::get_query(query_string);

            typedef ltj::ltj_iterator_lite<index_scheme_type, uint8_t, uint64_t> iterator_type;
#if ADAPTIVE
            typedef ltj::ltj_algorithm<iterator_type,
                ltj::veo::veo_adaptive<iterator_type, trait_type> > algorithm_type;

#else
            typedef ltj::ltj_algorithm<iterator_type,
                    ltj::veo::veo_simple<iterator_type, trait_type>> algorithm_type;
#endif
            typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;
            results_type res;

            auto start = std::chrono::high_resolution_clock::now();
            algorithm_type ltj(&query, &graph);
            ltj.join_v2(res, limit, timeout);
            auto stop = std::chrono::high_resolution_clock::now();

            auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            cout << nQ << ";" << res.size() << ";" << time << endl;
            nQ++;

            count += 1;
        }
    }
}


int main(int argc, char *argv[]) {
    //typedef ring::c_ring ring_type;
    if (argc < 5) {
        std::cout << "Usage: " << argv[0] << " <index> <queries> <limit> <type> [timeout]" << std::endl;
        return 0;
    }

    std::string index = argv[1];
    std::string queries = argv[2];
    uint64_t limit = std::atoll(argv[3]);
    std::string type = argv[4];
    uint64_t timeout = 600; //in serconds
    if(argc > 5) {
        timeout = std::atoll(argv[5]);
    }

    if (type == "normal") {
        query<cltj::compact_ltj, ltj::util::trait_distinct>(index, queries, limit, timeout);
    } else if (type == "star") {
        query<cltj::compact_ltj, ltj::util::trait_size>(index, queries, limit, timeout);
    } else {
        std::cout << "Type of index: " << type << " is not supported." << std::endl;
    }


    return 0;
}

