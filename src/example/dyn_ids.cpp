//
// Created by adrian on 17/12/24.
//


#include <api/cltj_dyn_ids.hpp>

int main(int argc, char **argv) {
    typedef cltj::cltj_dyn_ids::tuple_type tuple_type;
    {
        cltj::cltj_dyn_ids  cltj(argv[1]);
        std::string query = "?x 8 ?y . ?z 13 ?y";
        ::util::results_collector<tuple_type> res;
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << "{"<< (uint32_t) a.first << "=>" << a.second << "} ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        res.clear();
        cltj.insert("5 13 6");
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << "{"<< (uint32_t) a.first << "=>" << a.second << "} ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        res.clear();
        cltj.remove("5 13 6");
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << "{"<< (uint32_t) a.first << "=>" << a.second << "} ";
            }
            std::cout << std::endl;
        }

        sdsl::store_to_file(cltj, "cltj-star.bin");
    }
    std::cout << std::endl;
    {
        cltj::cltj_dyn_ids  cltj;
        std::cout << "Reading from file" << std::endl;
        sdsl::load_from_file(cltj, "cltj-star.bin");
        std::string query = "?x 8 ?y . ?z 13 ?y";
        ::util::results_collector<tuple_type> res;
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << "{"<< (uint32_t) a.first << "=>" << a.second << "} ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        res.clear();
        cltj.insert("5 13 6");
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << "{"<< (uint32_t) a.first << "=>" << a.second << "} ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        res.clear();
        cltj.remove("5 13 6");
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << "{"<< (uint32_t) a.first << "=>" << a.second << "} ";
            }
            std::cout << std::endl;
        }
    }
}


