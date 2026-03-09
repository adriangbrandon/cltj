/**
* Created by adrian on 24/10/24.
* In this experiment, we build the 80% of the dataset as a static/leaf node and the 20% remaining
* is added with insertions. Then, we measure the times of solving each query.
*/

#define CHECK 0

#include <iostream>
#include <index/cltj_index_spo_dyn.hpp>
#include <triple_pattern.hpp>
#include <query/ltj_algorithm.hpp>

using timer = std::chrono::high_resolution_clock;
std::random_device rd;
std::mt19937 g(rd());

bool get_file_content(std::string filename, std::vector<std::string> &vector_of_strings) {
    // Open the File
    std::ifstream in(filename.c_str());
    // Check if object is valid
    if (!in) {
        std::cerr << "Cannot open the File : " << filename << std::endl;
        return false;
    }
    std::string str;
    // Read the next line from File until it reaches the end.
    while (getline(in, str)) {
        // Line contains string of length > 0 then save it in vector
        if (str.size() > 0)
            vector_of_strings.push_back(str);
    }
    //Close The File
    in.close();
    return true;
}

std::string ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(' ');
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(' ');
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

std::vector<std::string> tokenizer(const std::string &input, const char &delimiter) {
    std::stringstream stream(input);
    std::string token;
    std::vector<std::string> res;
    while (getline(stream, token, delimiter)) {
        res.emplace_back(trim(token));
    }
    return res;
}

bool is_variable(std::string &s) {
    return (s.at(0) == '?');
}

uint8_t get_variable(std::string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars) {
    auto var = s.substr(1);
    auto it = hash_table_vars.find(var);
    if (it == hash_table_vars.end()) {
        uint8_t id = hash_table_vars.size();
        hash_table_vars.insert({var, id});
        return id;
    } else {
        return it->second;
    }
}

uint64_t get_constant(std::string &s) {
    return std::stoull(s);
}

ltj::triple_pattern get_triple(std::string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars) {
    std::vector<std::string> terms = tokenizer(s, ' ');

    ltj::triple_pattern triple;
    if (is_variable(terms[0])) {
        triple.var_s(get_variable(terms[0], hash_table_vars));
    } else {
        triple.const_s(get_constant(terms[0]));
    }
    if (is_variable(terms[1])) {
        triple.var_p(get_variable(terms[1], hash_table_vars));
    } else {
        triple.const_p(get_constant(terms[1]));
    }
    if (is_variable(terms[2])) {
        triple.var_o(get_variable(terms[2], hash_table_vars));
    } else {
        triple.const_o(get_constant(terms[2]));
    }
    return triple;
}

std::string get_type(const std::string &file) {
    auto p = file.find_last_of('.');
    return file.substr(p + 1);
}


template<class index_scheme_type, class trait_type>
void query(std::vector<cltj::spo_triple> &D, index_scheme_type &graph, const std::string &queries,
           const uint64_t limit, const uint64_t upq, const uint64_t n) {

    std::vector<std::string> dummy_queries;
    bool result = get_file_content(queries, dummy_queries);
    std::ifstream ifs;
    uint64_t nQ = 0;

    //::util::time::usage::usage_type start, stop;
    uint64_t total_elapsed_time;
    uint64_t total_user_time;
    uint64_t i_del = 0;
    uint64_t i_ins = n;
    std::uniform_int_distribution<uint> dist1(0,1);

    if (result) {
        int count = 1;
        for (std::string &query_string: dummy_queries) {
            //vector<Term*> terms_created;
            //vector<Triple*> query;

            //Updates per query
            for(uint64_t i = 0; i < upq; ++i) {
                bool ins = dist1(rd);
                if(ins) {
                    graph.insert(D[i_ins++]);
                }else {
                    graph.remove(D[i_del++]);
                }
            }

            std::unordered_map<std::string, uint8_t> hash_table_vars;
            std::vector<ltj::triple_pattern> query;
            std::vector<std::string> tokens_query = tokenizer(query_string, '.');
            for (std::string &token: tokens_query) {
                auto triple_pattern = get_triple(token, hash_table_vars);
                query.push_back(triple_pattern);
            }


            typedef ltj::ltj_iterator_dyn<index_scheme_type, uint8_t, uint64_t> iterator_type;
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
            results_type res;

            auto start = std::chrono::high_resolution_clock::now();
            algorithm_type ltj(&query, &graph);
            ltj.join(res, limit, 600);
            auto stop = std::chrono::high_resolution_clock::now();

            /*std::unordered_map<uint8_t, std::string> ht;
            for(const auto &p : hash_table_vars){
                ht.insert({p.second, p.first});
            }*/

            //cout << "Query Details:" << endl;
            //ltj.print_query(ht);
            //ltj.print_gao(ht);
            //cout << "##########" << endl;
            //ltj.print_results(res, ht);
            auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            cout << nQ << ";" << res.size() << ";" << time << endl;
            nQ++;

            // cout << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;

            //cout << "RESULTS QUERY " << count << ": " << number_of_results << endl;
            count += 1;
        }
    }
}

int main(int argc, char **argv){
    if(argc != 6){
        std::cout<< argv[0] << " <dataset> <queries> <limit> <type> <updates>" << std::endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string queries = argv[2];
    uint64_t limit = atoll(argv[3]);
    std::string type = argv[4];
    uint64_t upq = atoll(argv[5]); //updates per query

    std::vector<cltj::spo_triple> D;

    std::ifstream ifs(dataset);
    uint32_t s, p , o;
    uint64_t k = 0;
    cltj::spo_triple spo;
    do {
        ifs >> s >> p >> o;
        spo[0] = s; spo[1] = p; spo[2] = o;
        D.emplace_back(spo);
        ++k;

    } while (!ifs.eof());

    D.shrink_to_fit();
    std::cout << "Triples: " << D.size() << "." << std::endl;
    std::cout << "Dataset: " << 3*D.size()*sizeof(::uint32_t) << " bytes." << std::endl;
    //Shuffle the dataset
    std::shuffle(D.begin(), D.end(), g);

    uint64_t n = D.size() * 0.8; //num triples in the build phase

    //Build pahse
    auto start = timer::now();
    cltj::compact_dyn_ltj index(D.begin(), D.begin()+n);
    auto end = timer::now();
    auto sec_build = std::chrono::duration_cast<std::chrono::seconds>(end-start).count();
    std::cout << "Build phase in " << sec_build << " secs." << std::endl;
    std::cout << "Index uses " << sdsl::size_in_bytes(index) << " bytes." << std::endl;
    std::cout << std::endl;
#if  CHECK
    std::cout << "\r check build: 0% (0/" << n << ")" << std::flush;
    for(uint64_t i = 0; i < n; ++i) {
        if(i % (n/1000) == 0) {
            float per = (i / (float) (n)) * 100;
            std::cout << "\r check build: " << per <<  "% (" << i << "/" << n << ")" << std::flush;
        }
        auto ap = index.test_exists(D[i]);
        if(6 > ap) {
            std::cout << std::endl;
            std::cout << "Appears in " << ap << " tries." << std::endl;
            std::cout << "Error in construction at i=" << i << "." << std::endl;
            std::cout << "(" << D[i][0] << ", " << D[i][1] << ", " << D[i][2] << ")"  << std::endl;
            exit(0);
        }
    }
    std::cout << "\r check build: 100% (" << n << "/" << n << ")" << std::endl;
#endif


    if (type == "normal") {
        query<cltj::compact_dyn_ltj, ltj::util::trait_distinct>(D, index, queries, limit, upq, n);
    } else if (type == "star") {
        query<cltj::compact_dyn_ltj, ltj::util::trait_size>(D, index, queries, limit, upq, n);
    } else {
        std::cout << "Type of index: " << type << " is not supported." << std::endl;
    }

    std::cout << "Index uses " << sdsl::size_in_bytes(index) << " bytes." << std::endl;

}