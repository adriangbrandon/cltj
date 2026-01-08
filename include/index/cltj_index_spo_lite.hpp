//
// Created by Adrián on 19/10/23.
//

#ifndef CLTJ_INDEX_SPO_V2_HPP
#define CLTJ_INDEX_SPO_V2_HPP


#include <trie/cltj_compact_trie.hpp>
#include <trie/cltj_compact_trie_stats.hpp>
#include <cltj_helper.hpp>
#include <trie/cltj_uncompact_trie.hpp>
#include <cltj_config.hpp>

namespace cltj {

    template<class Trie>
    class cltj_index_spo_lite {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef Trie trie_type;

    private:
        std::array<trie_type, 6> m_tries;
        std::array<size_type, 3> m_gaps;

        void copy(const cltj_index_spo_lite &o){
            m_tries = o.m_tries;
            m_gaps = o.m_gaps;
        }

    public:

        const std::array<trie_type, 6> &tries = m_tries;
        const std::array<size_type, 3> &gaps = m_gaps;
        cltj_index_spo_lite() = default;

        cltj_index_spo_lite(vector<spo_triple> &D){

            for(size_type i = 0; i < 6; ++i){
                std::sort(D.begin(), D.end(), comparator_order(i));
                std::vector<uint32_t> syms;
                std::vector<size_type> lengths;
                helper::sym_level(D,  spo_orders[i], i % 2, 3, syms, lengths);
                if(i % 2 == 0){
                    m_gaps[i/2] = lengths[0];
                }
                m_tries[i] = trie_type(syms, lengths);
            }
        }

        //! Copy constructor
        cltj_index_spo_lite(const cltj_index_spo_lite &o) {
            copy(o);
        }

        //! Move constructor
        cltj_index_spo_lite(cltj_index_spo_lite &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_index_spo_lite &operator=(const cltj_index_spo_lite &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_index_spo_lite &operator=(cltj_index_spo_lite &&o) {
            if (this != &o) {
                m_tries = std::move(o.m_tries);
                m_gaps = std::move(o.m_gaps);
            }
            return *this;
        }

        void swap(cltj_index_spo_lite &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_tries, o.m_tries);
            std::swap(m_gaps, o.m_gaps);
        }

        inline trie_type* get_trie(size_type i){
            return &m_tries[i];
        }

        bool insert(const spo_triple &triple) {
            std::cout << "Insert operation is not supported (static version)." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        bool remove(const spo_triple &triple) {
            std::cout << "Remove operation is not supported (static version)." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            for(const auto & trie : m_tries){
                written_bytes += trie.serialize(out, child, "tries");
            }
            for(const auto & gap : m_gaps) {
                written_bytes += sdsl::write_member(gap, out, child, "gaps");
            }
            /*auto pos = sdsl::size_in_bytes(m_tries[2]);
            auto pso = sdsl::size_in_bytes(m_tries[3]);
            std::cout << "POS: " << pos << std::endl;
            std::cout << "PSO: " << pso << std::endl;
            std::cout << "POS + PSO: " << pos + pso << " bytes." << std::endl;*/
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            for(auto & trie : m_tries){
                trie.load(in);
            }
            for(auto & gap : m_gaps){
                sdsl::read_member(gap, in);
            }
        }

    };

    typedef cltj::cltj_index_spo_lite<cltj::compact_trie> compact_ltj;
    typedef cltj::cltj_index_spo_lite<cltj::uncompact_trie> uncompact_ltj;
    typedef cltj::cltj_index_spo_lite<cltj::compact_trie_stats> compact_ltj_stats;

}

#endif //CLTJ_INDEX_SPO_V2_HPP
