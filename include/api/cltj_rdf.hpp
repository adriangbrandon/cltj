//
// Created by adrian on 12/12/24.
//

#ifndef CLTJ_RDF_HPP
#define CLTJ_RDF_HPP

#include <index/cltj_index_metatrie_dyn.hpp>
#include <index/cltj_index_spo_dyn.hpp>
#include <query/ltj_algorithm.hpp>
#include <dict/dict_map.hpp>
#include <util/rdf_util.hpp>

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

namespace cltj {

    template<class Index    = cltj::compact_dyn_ltj,
             class Trait    = ltj::util::trait_size,
             class Iterator = ltj::ltj_iterator_dyn<Index, uint8_t, uint64_t>,
             class Veo      = ltj::veo::veo_adaptive<Iterator, Trait>>
    class cltj_rdf {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef dict::basic_map dict_type;
        typedef Index index_type;
        typedef Trait trait_type;
        typedef Iterator iterator_type;
        typedef Veo veo_type;
        typedef ltj::ltj_algorithm<iterator_type, veo_type > algorithm_type;
        typedef typename algorithm_type::tuple_str_type tuple_type;

    private:

        dict_type  m_dict_so;
        dict_type  m_dict_p;
        index_type m_index;

        void copy(const cltj_rdf &o){
            m_dict_so = o.m_dict_so;
            m_dict_p = o.m_dict_p;
            m_index = o.m_index;
        }

        void build(const std::string &dataset){
            vector<cltj::spo_triple> D;
            std::map<std::string, uint64_t> map_so, map_p;
            std::map<std::string, uint64_t>::iterator it;
            uint64_t id_so = 0, id_p = 0;
            std::ifstream ifs(dataset);
            cltj::spo_triple spo;
            std::string line;

            std::cout << "============================================================" << std::endl;
            std::cout << "Reading data and mapping... " << std::flush;
            //STEP1: mapping for bulk construction
            auto start = timer::now();
            do {
                std::getline(ifs, line);
                if(line.empty()) break;
                auto spo_str = ::util::rdf::str::get_triple(line);
                if((it = map_so.find(spo_str[0])) == map_so.end()) {
                    map_so.insert({spo_str[0], ++id_so});
                    spo[0] = id_so;
                }else{
                    spo[0] = it->second;
                }
                if((it = map_p.find(spo_str[1])) == map_p.end()) {
                    map_p.insert({spo_str[1], ++id_p});
                    spo[1] = id_p;
                }else{
                    spo[1] = it->second;
                }
                if((it=map_so.find(spo_str[2])) == map_so.end()) {
                    map_so.insert({spo_str[2], ++id_so});
                    spo[2] = id_so;
                }else{
                    spo[2] = it->second;
                }
                D.emplace_back(spo);
            } while (!ifs.eof());
            D.shrink_to_fit();
            auto stop = timer::now();
            auto secs = duration_cast<seconds>(stop-start).count();
            std::cout << "done. [" << secs << " secs. ]" << std::endl;

            size_type size_data = 3*D.size()*sizeof(::uint32_t);

            /*for(const auto t : D) {
                std::cout << t[0] << " " << t[1] << " " << t[2] << std::endl;
            }*/
            //std::cout << "Dataset: " << 3*D.size()*sizeof(::uint32_t) << " bytes." << std::endl;

            //STEP2: Build dictionaries
            std::cout << "Building dictionaries... " << std::flush;
            start = timer::now();
            m_dict_so = dict::basic_map(map_so);
            m_dict_p  = dict::basic_map(map_p);
            stop = timer::now();
            secs = duration_cast<seconds>(stop-start).count();
            std::cout << "done. [" << secs << " secs. ]" << std::endl;

            //STEP2: Build index
            std::cout << "Building index... " << std::flush;
            start = timer::now();
            m_index = index_type(D);
            stop = timer::now();
            secs = duration_cast<seconds>(stop-start).count();
            std::cout << "done. [" << secs << " secs. ]" << std::endl;

            size_type size_dict_so = sdsl::size_in_bytes(m_dict_so);
            size_type size_dict_p = sdsl::size_in_bytes(m_dict_p);
            size_type size_index = sdsl::size_in_bytes(m_index);

            std::cout << std::endl;
            std::cout << "============================================================" << std::endl;
            std::cout << "Dict. SO : " << size_dict_so << " bytes. " << std::endl;
            std::cout << "Dict. P  : " << size_dict_p << " bytes. " << std::endl;
            std::cout << "Index    : " << size_index << " bytes. " << std::endl;
            std::cout << "Bin. data: " << size_data << " bytes. " << std::endl;
            std::cout << "Ratio    : " << (size_index) / (double) size_data * 100 << "%" << std::endl;
            std::cout << "============================================================" << std::endl;

        }

    public:

        cltj_rdf() = default;

        explicit cltj_rdf(const std::string &dataset){
            build(dataset);
        }

        template<class result_type>
        bool query(const std::string &query_str, result_type &res, size_type limit = 1000, size_type timeout_seconds = 600) {

            ::util::rdf::ht_var_id_type ht_var_id;
            std::vector<bool> var_in_p;
            std::vector<ltj::triple_pattern> query;

            std::vector<cltj::user_triple> tokens = ::util::rdf::str::get_query(query_str);
            for (cltj::user_triple &token: tokens) {
                auto p = ::util::rdf::str::get_triple_pattern(token, ht_var_id, var_in_p, m_dict_so, m_dict_p);
                if(!p.first) return false;
                query.push_back(p.second);
            }

            algorithm_type ltj(&query, &m_index);
            ltj.join_str(res, var_in_p, m_dict_so, m_dict_p, limit, timeout_seconds);

            m_dict_p.reset_cache(); //TODO: ao mellor hai que mellorar isto
            m_dict_so.reset_cache();
            return true;
        }

        bool insert(const std::string &triple) {
            auto spo_str = ::util::rdf::str::get_triple(triple);
            cltj::spo_triple spo;
            spo[0] = m_dict_so.get_or_insert(spo_str[0]);
            spo[1] = m_dict_p.get_or_insert(spo_str[1]);
            spo[2] = m_dict_so.get_or_insert(spo_str[2]);
            return m_index.insert(spo);
        }

        bool remove(const std::string &triple) {
            auto spo_str = ::util::rdf::str::get_triple(triple);
            cltj::spo_triple spo;
            spo[0] = m_dict_so.locate(spo_str[0]);
            if(!spo[0]) return false;
            spo[1] = m_dict_p.locate(spo_str[1]);
            if(!spo[1]) return false;
            spo[2] = m_dict_so.get_or_insert(spo_str[2]);
            if(!spo[2]) return false;
            auto r = m_index.remove_and_report(spo);
            if(r.removed) {
                if(r.rem_in_dict[0]) m_dict_so.eliminate(spo[0]);
                if(r.rem_in_dict[1]) m_dict_p.eliminate(spo[1]);
                if(r.rem_in_dict[2]) m_dict_so.eliminate(spo[2]);
                return true;
            }
            return false;
        }

        //! Copy constructor
        cltj_rdf(const cltj_rdf &o) {
            copy(o);
        }

        //! Move constructor
        cltj_rdf(cltj_rdf &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_rdf &operator=(const cltj_rdf &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_rdf &operator=(cltj_rdf &&o) {
            if (this != &o) {
                m_dict_so = std::move(o.m_dict_so);
                m_dict_p = std::move(o.m_dict_p);
                m_index = std::move(o.m_index);
            }
            return *this;
        }

        void swap(cltj_rdf &o) {
            std::swap(m_dict_so, o.m_dict_so);
            std::swap(m_dict_p, o.m_dict_p);
            std::swap(m_index, o.m_index);
        }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_dict_so.serialize(out, child, "dict_so");
            written_bytes += m_dict_p.serialize(out, child, "dict_p");
            written_bytes += m_index.serialize(out, child, "index");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_dict_so.load(in);
            m_dict_p.load(in);
            m_index.load(in);
        }
    };

        //Full Tries + Dynamic + VEO adaptive
        typedef cltj::cltj_rdf<> cltj_rdf_dyn;
        //Full Tries + Dynamic + VEO global
        typedef cltj::cltj_rdf<cltj::compact_dyn_ltj, ltj::util::trait_size,
                ltj::ltj_iterator_dyn<cltj::compact_dyn_ltj, uint8_t, uint64_t>,
                ltj::veo::veo_simple<ltj::ltj_iterator_dyn<cltj::compact_dyn_ltj, uint8_t, uint64_t>>> cltj_rdf_dyn_global;
        //Meta Tries + Dynamic + VEO adaptive
        typedef cltj::cltj_rdf<cltj::compact_ltj_metatrie_dyn, ltj::util::trait_size,
                ltj::ltj_iterator_metatrie_dyn<cltj::compact_ltj_metatrie_dyn, uint8_t, uint64_t>> xcltj_rdf_dyn;
        //Meta Tries + Dynamic + VEO global
        typedef cltj::cltj_rdf<cltj::compact_ltj_metatrie_dyn, ltj::util::trait_size,
                ltj::ltj_iterator_metatrie_dyn<cltj::compact_ltj_metatrie_dyn, uint8_t, uint64_t>,
                ltj::veo::veo_simple<ltj::ltj_iterator_metatrie_dyn<cltj::compact_ltj_metatrie_dyn, uint8_t, uint64_t>>> xcltj_rdf_dyn_global;

}
#endif //CLTJ_RDF_HPP
