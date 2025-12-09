#ifndef CLTJ_COMPACT_TRIE_H
#define CLTJ_COMPACT_TRIE_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <sdsl/vectors.hpp>
#include <sdsl/select_support_mcl.hpp>
#include <cds/succ_support_v.hpp>

namespace cltj {

    class compact_trie {


    public:

        typedef uint64_t size_type;
        typedef uint32_t value_type;

    private:
        sdsl::bit_vector m_bv;
        sdsl::int_vector<> m_seq;
        //sdsl::rank_support_v<1> m_rank1;
        cds::succ_support_v<0> m_succ0;
        sdsl::select_support_mcl<0> m_select0;

        void copy(const compact_trie &o) {
            m_bv = o.m_bv;
            m_seq = o.m_seq;
            //m_rank1 = o.m_rank1;
            //m_rank1.set_vector(&m_bv);
            m_succ0 = o.m_succ0;
            m_succ0.set_vector(&m_bv);
            m_select0 = o.m_select0;
            m_select0.set_vector(&m_bv);
        }

        /*inline size_type rank0(const size_type i) const {
            return i - m_rank1(i);
        }*/

    public:

        const sdsl::int_vector<> &seq = m_seq;
        const sdsl::bit_vector &bv = m_bv;

        compact_trie() = default;

        compact_trie(const std::vector<uint32_t> &syms, const std::vector<size_type> &lengths) {
            m_bv = sdsl::bit_vector(syms.size()+1, 1);
            m_seq = sdsl::int_vector<>(syms.size());
            m_bv[0] = 0;
            uint64_t pos_bv = 0, pos_seq = 0;
            for(const auto &sym : syms) {
                m_seq[pos_seq] = sym;
                ++pos_seq;
            }
            for(const auto &len : lengths) {
                pos_bv = pos_bv + len;
                m_bv[pos_bv] = 0;
            }

            //m_bv.resize(pos_bv+1);
            //m_seq.resize(pos_seq);
           // std::cout << "n_nodes=" << syms.size() << " pos_seq=" << pos_seq << " pos_bv=" << pos_bv << std::endl;

            sdsl::util::bit_compress(m_seq);
            //std::cout << "width of trie seq = " << (uint64_t) m_seq.width() << std::endl;
            sdsl::util::init_support(m_succ0, &m_bv);
            sdsl::util::init_support(m_select0, &m_bv);

        }

        //! Copy constructor
        compact_trie(const compact_trie &o) {
            copy(o);
        }

        //! Move constructor
        compact_trie(compact_trie &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        compact_trie &operator=(const compact_trie &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        compact_trie &operator=(compact_trie &&o) {
            if (this != &o) {
                m_bv = std::move(o.m_bv);
                m_seq = std::move(o.m_seq);
                //m_rank1 = std::move(o.m_rank1);
                m_succ0 = std::move(o.m_succ0);
                //m_rank1.set_vector(&m_bv);
                m_succ0.set_vector(&m_bv);
                m_select0 = std::move(o.m_select0);
                m_select0.set_vector(&m_bv);
            }
            return *this;
        }

        void swap(compact_trie &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_bv, o.m_bv);
            std::swap(m_seq, o.m_seq);
            sdsl::util::swap_support(m_succ0, o.m_succ0, &m_bv, &o.m_bv);
            sdsl::util::swap_support(m_select0, o.m_select0, &m_bv, &o.m_bv);
        }


        /*
            Receives index in bit vector
            Returns index of next 0
        */
        /*inline uint32_t succ0(uint32_t it) const{
            return m_select0(rank0(it) + 1);
        }*/

        /*
            Receives index of current node and the child that is required
            Returns index of the nth child of current node
        */
        inline size_type child(uint32_t it, uint32_t n, uint32_t gap = 1) const {
            return m_select0(it + gap + n);
        }

        inline size_type nodeselect(uint32_t it, uint32_t gap = 1) const {
            return child(it, 1, gap);
        }
    
        /*
            Receives index of node whos children we want to count
            Returns how many children said node has
        */
        size_type children(size_type i) const {
            return m_succ0(i + 1) - i;
        }

        size_type first_child(size_type i) const {
            return i;
        }


        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
            if (m_seq[f] < val) return std::make_pair(0, f + 1);
            uint32_t mid;
            while (i < f) {
                mid = (i + f) / 2;
                if (m_seq[mid] < val) {
                    i = mid + 1;
                } else {
                    f = mid;
                }
            }
            return std::make_pair(m_seq[i], i);
        }


        void print() const {
            for (auto i = 0; i < m_bv.size(); ++i) {
                std::cout << (uint) m_bv[i];
            }
            std::cout << std::endl;
        }

        //! Serializes the data structure into the given ostream
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_bv.serialize(out, child, "bv");
            written_bytes += m_seq.serialize(out, child, "seq");
            written_bytes += m_succ0.serialize(out, child, "succ0");
            written_bytes += m_select0.serialize(out, child, "select0");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_bv.load(in);
            m_seq.load(in);
            m_succ0.load(in, &m_bv);
            m_select0.load(in, &m_bv);
        }

    };
}
#endif