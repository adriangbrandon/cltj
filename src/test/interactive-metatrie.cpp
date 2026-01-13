//
// Created by adrian on 16/12/24.
//

#include <iostream>
#include <index/cltj_index_metatrie_dyn.hpp>
#include <query/ltj_iterator_metatrie.hpp>
#include <query/ltj_algorithm.hpp>
#include <util/rdf_util.hpp>

template<class index_scheme_type, class trait_type>
void query(std::vector<ltj::triple_pattern> &q, index_scheme_type &graph) {

        typedef ltj::ltj_iterator_metatrie_dyn<index_scheme_type, uint8_t, uint64_t> iterator_type;
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
        algorithm_type ltj(&q, &graph);
        ltj.join(res, 0, 600);
        auto stop = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        cout << res.size() << ";" << time << endl;

        // cout << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;

        //cout << "RESULTS QUERY " << count << ": " << number_of_results << endl;
}

int main(int argc, char *argv[]) {
    //typedef ring::c_ring ring_type;
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <dataset>" << std::endl;
        return 0;
    }

    std::string dataset      = argv[1];
    std::string index_name   = dataset + ".cltj-mt-dyn";


    std::cout << "Index: " << index_name << std::endl;
    cltj::compact_ltj_metatrie_dyn graph;
    sdsl::load_from_file(graph, index_name);
    cltj::spo_triple triple;
    triple[0] = 28731448; triple[1]= 2831; triple[2] = 29469816;
    graph.insert(triple);

    std::string line;
    while(true) {
        std::getline(std::cin, line);
        if(line == "quit") break;
        if(line == "insert") {
            std::getline(std::cin, line);
            auto t = ::util::rdf::ids::get_triple(line);
            graph.insert(t);
        }else if (line == "delete") {
            std::getline(std::cin, line);
            auto t = ::util::rdf::ids::get_triple(line);
            graph.remove(t);
        }else if (line == "query") {
            std::getline(std::cin, line);
            auto q = ::util::rdf::ids::get_query(line);
            query<cltj::compact_ltj_metatrie_dyn, ltj::util::trait_size>(q, graph);
        }
    }

    return 0;
}
