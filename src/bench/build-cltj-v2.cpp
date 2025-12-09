#include <iostream>
#include <index/cltj_index_spo_lite_v2.hpp>

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
        std::string index_name = dataset + ".cltj-v2";
        vector<cltj::spo_triple> D;

        std::ifstream ifs(dataset);
        uint32_t s, p , o;
        cltj::spo_triple spo;
        do {
            ifs >> s >> p >> o;
            if(ifs.fail()) break;
            spo[0] = s; spo[1] = p; spo[2] = o;
            D.emplace_back(spo);

        } while (!ifs.eof());
        std::cout << "D.size()=" << D.size() << std::endl;

        D.shrink_to_fit();
        std::cout << "Dataset: " << 3*D.size()*sizeof(::uint32_t) << " bytes." << std::endl;
        //sdsl::memory_monitor::start();

        auto start = timer::now();
        cltj::compact_ltj_v2 index(D);
        auto stop = timer::now();

        //sdsl::memory_monitor::stop();

        sdsl::store_to_file(index, index_name);

        cout << "Index saved" << endl;
        cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
        cout << sdsl::memory_monitor::peak() << " bytes." << endl;
        // ti.indexNewTable(file_name);

        // ind.save();
    }
    catch(const char *msg){
        cerr<<msg<<endl;
    }
    return 0;
}