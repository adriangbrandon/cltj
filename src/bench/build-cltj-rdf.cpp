    #include <iostream>
#include <index/cltj_index_spo_dyn.hpp>
#include <dict/dict_map.hpp>
#include <regex>
#include <util/rdf_util.hpp>

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

int main(int argc, char **argv){
    try{

        if(argc != 2){
            cout<< argv[0] << " <dataset>" <<endl;
            return 0;
        }

        std::string dataset = argv[1];
        std::string index_name = dataset + ".cltj.idx";
        std::string dict_so_name = dataset + ".cltj.so";
        std::string dict_p_name = dataset + ".cltj.p";

        std::ifstream ifs(dataset);
        cltj::spo_triple spo;
        std::string line;

        vector<cltj::spo_triple> D;
        {
            dict::basic_map dict_so;
            dict::basic_map dict_p;
            auto start = timer::now();
            do {
                std::getline(ifs, line);
                if(line.empty()) break;
                auto spo_str = ::util::rdf::str::get_triple(line);
                spo[0] = dict_so.get_or_insert(spo_str[0]);
                spo[1] = dict_p.get_or_insert(spo_str[1]);
                spo[2] = dict_so.get_or_insert(spo_str[2]);
                D.emplace_back(spo);

            } while (!ifs.eof());
            auto stop = timer::now();
            cout << "Mapping: " << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
            D.shrink_to_fit();
            std::cout << "Dataset: " << 3*D.size()*sizeof(::uint32_t) << " bytes." << std::endl;
            sdsl::store_to_file(dict_p, dict_p_name);
            sdsl::store_to_file(dict_so, dict_so_name);
            std::cout << "Size dictP : " << sdsl::size_in_bytes(dict_p) << " (bytes)" << std::endl;
            std::cout << "Size dictSO: " << sdsl::size_in_bytes(dict_so) << " (bytes)" << std::endl;
        }
        //sdsl::memory_monitor::start();

        auto start = timer::now();
        cltj::compact_dyn_ltj index(D);
        auto stop = timer::now();

        //sdsl::memory_monitor::stop();

        sdsl::store_to_file(index, index_name);
        std::cout << "Size index : " << sdsl::size_in_bytes(index) << " (bytes)" << std::endl;

        cout << "Index saved" << endl;
        cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
        // ti.indexNewTable(file_name);

        // ind.save();
    }
    catch(const char *msg){
        cerr<<msg<<endl;
    }
    return 0;
}