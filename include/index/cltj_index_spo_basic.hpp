//
// Created by Adrián on 19/10/23.
//

#ifndef CLTJ_INDEX_SPO_BASIC_HPP
#define CLTJ_INDEX_SPO_BASIC_HPP


#include <trie/cltj_compact_trie.hpp>
#include <trie/cltj_uncompact_trie.hpp>
#include <cltj_helper.hpp>

namespace cltj {

    template<class Trie>
    class cltj_index_spo_basic {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef Trie trie_type;

    private:
        std::array<trie_type, 6> m_tries;

        void copy(const cltj_index_spo_basic &o){
            m_tries = o.m_tries;
        }

    public:

        const std::array<trie_type, 6> &tries = m_tries;
        cltj_index_spo_basic() = default;

        cltj_index_spo_basic(const vector<spo_triple> &D){
            for(size_type i = 0; i < 6; ++i){
                std::sort(D.begin(), D.end(), comparator_order(i));
                std::vector<uint32_t> syms;
                std::vector<size_type> lengths;
                helper::sym_level(D,  spo_orders[i], 0, 3, syms, lengths);
                m_tries[i] = trie_type(syms, lengths);
            }
        }

        //! Copy constructor
        cltj_index_spo_basic(const cltj_index_spo_basic &o) {
            copy(o);
        }

        //! Move constructor
        cltj_index_spo_basic(cltj_index_spo_basic &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_index_spo_basic &operator=(const cltj_index_spo_basic &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_index_spo_basic &operator=(cltj_index_spo_basic &&o) {
            if (this != &o) {
                m_tries = std::move(o.m_tries);
            }
            return *this;
        }

        void swap(cltj_index_spo_basic &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_tries, o.m_tries);
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
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            for(auto & trie : m_tries){
                trie.load(in);
            }

        }

    };

    typedef cltj::cltj_index_spo_basic<cltj::compact_trie> compact_basic_ltj;
    typedef cltj::cltj_index_spo_basic<cltj::uncompact_trie> uncompact_basic_ltj;

}

#endif //CLTJ_CLTJ_INDEX_ADRIAN_HPP
