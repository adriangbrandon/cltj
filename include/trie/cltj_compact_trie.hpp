//
// Compact trie of CompactLTJ: topology T (one bit per edge) plus labels L.
//
// Labels L are stored with one width per level, and searched with galloping
// (exponential) search from the start of the range, which the iterators keep at
// the position where the previous leap ended.
//
// The tries of cltj_index_spo_lite and cltj_index_metatrie store two levels (the
// first one is implicit or a bitvector): the labels of level 2 come first in L,
// followed by those of level 3. With a single width the level of predicates paid
// the width of subjects/objects (e.g. 29 bits instead of 13 in Wikidata). Here L
// is split into m_seq2 (positions [0, n2)) and m_seq3 (positions [n2, n)), each
// bit-compressed on its own. The topology (T, select0, succ0) is unchanged.
//

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
#include <cltj_helper.hpp>

namespace cltj {

    class compact_trie {

    public:

        typedef uint64_t size_type;
        typedef uint32_t value_type;

    private:
        sdsl::bit_vector m_bv;
        sdsl::int_vector<> m_seq2;   // labels of the level-2 edges
        sdsl::int_vector<> m_seq3;   // labels of the level-3 edges (empty in partial tries)
        size_type m_n2 = 0;          // number of level-2 edges
        cds::succ_support_v<0> m_succ0;
        sdsl::select_support_mcl<0> m_select0;

        void copy(const compact_trie &o) {
            m_bv = o.m_bv;
            m_seq2 = o.m_seq2;
            m_seq3 = o.m_seq3;
            m_n2 = o.m_n2;
            m_succ0 = o.m_succ0;
            m_succ0.set_vector(&m_bv);
            m_select0 = o.m_select0;
            m_select0.set_vector(&m_bv);
        }

        //! Galloping search for the first position k in [i, f] with seq[k - off] >= val.
        //! Probes i, i+1, i+3, i+7, ... and finishes with binary search.
        template<class Seq>
        static std::pair<uint32_t, uint64_t> gallop(const Seq &seq, uint64_t off, uint32_t val,
                                                    uint64_t i, uint64_t f) {
            uint64_t lo = i, step = 1, hi = i;
            if (seq[i - off] >= val) return std::make_pair((uint32_t) seq[i - off], i);
            while (true) {
                hi = lo + step;
                if (hi >= f) { hi = f; break; }
                if (seq[hi - off] >= val) break;
                lo = hi;
                step <<= 1;
            }
            // seq[lo] < val; the answer is in (lo, hi] if seq[hi] >= val
            if (seq[hi - off] < val) return std::make_pair(0, f + 1);
            ++lo;
            while (lo < hi) {
                uint64_t mid = (lo + hi) / 2;
                if (seq[mid - off] < val) lo = mid + 1; else hi = mid;
            }
            return std::make_pair((uint32_t) seq[lo - off], lo);
        }

    public:

        const sdsl::bit_vector &bv = m_bv;

        compact_trie() = default;

        //! syms: labels of level 2 followed by those of level 3; lengths: degrees of
        //! the nodes, the first n1 being the level-1 nodes (whose children are level 2).
        compact_trie(const std::vector<uint32_t> &syms, const std::vector<size_type> &lengths, size_type n1) {
            m_bv = sdsl::bit_vector(syms.size() + 1, 1);
            m_bv[0] = 0;
            uint64_t pos_bv = 0;
            m_n2 = 0;
            for (size_type k = 0; k < lengths.size(); ++k) {
                pos_bv = pos_bv + lengths[k];
                m_bv[pos_bv] = 0;
                if (k < n1) m_n2 += lengths[k];
            }
            m_seq2 = sdsl::int_vector<>(m_n2);
            m_seq3 = sdsl::int_vector<>(syms.size() - m_n2);
            for (size_type j = 0; j < m_n2; ++j) m_seq2[j] = syms[j];
            for (size_type j = m_n2; j < syms.size(); ++j) m_seq3[j - m_n2] = syms[j];
            sdsl::util::bit_compress(m_seq2);
            sdsl::util::bit_compress(m_seq3);
            sdsl::util::init_support(m_succ0, &m_bv);
            sdsl::util::init_support(m_select0, &m_bv);
        }

        //! All labels with a single width (used by cltj_index_spo_basic)
        compact_trie(const std::vector<uint32_t> &syms, const std::vector<size_type> &lengths)
                : compact_trie(syms, lengths, lengths.size()) {}

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
                m_seq2 = std::move(o.m_seq2);
                m_seq3 = std::move(o.m_seq3);
                m_n2 = o.m_n2;
                m_succ0 = std::move(o.m_succ0);
                m_succ0.set_vector(&m_bv);
                m_select0 = std::move(o.m_select0);
                m_select0.set_vector(&m_bv);
            }
            return *this;
        }

        void swap(compact_trie &o) {
            std::swap(m_bv, o.m_bv);
            std::swap(m_seq2, o.m_seq2);
            std::swap(m_seq3, o.m_seq3);
            std::swap(m_n2, o.m_n2);
            sdsl::util::swap_support(m_succ0, o.m_succ0, &m_bv, &o.m_bv);
            sdsl::util::swap_support(m_select0, o.m_select0, &m_bv, &o.m_bv);
        }

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

        //! Label of the edge at position i of L
        inline value_type access(size_type i) const {
            return (i < m_n2) ? m_seq2[i] : m_seq3[i - m_n2];
        }

        //! Smallest position k in [i, f] with L[k] >= val, and L[k]; (0, f+1) if none.
        //! [i, f] is (a suffix of) the child range of one node, hence inside one level.
        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint64_t i, uint64_t f) const {
            if (f < m_n2) return gallop(m_seq2, 0, val, i, f);
            return gallop(m_seq3, m_n2, val, i, f);
        }

        //! Interface used by the iterators (deg = degree of the node, cur = cursor of the
        //! position, only meaningful for tries that store long nodes apart, such as compact_trie_ef)
        inline uint64_t access(uint64_t i, uint64_t, uint64_t &) const { return access(i); }
        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint64_t i, uint64_t f, uint64_t,
                                                         uint64_t &) const {
            return binary_search_seek(val, i, f);
        }
        void append_children(uint64_t beg, uint64_t deg, std::vector<uint64_t> &out) const {
            for (uint64_t j = beg; j < beg + deg; ++j) out.emplace_back(access(j));
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
            written_bytes += m_seq2.serialize(out, child, "seq2");
            written_bytes += m_seq3.serialize(out, child, "seq3");
            written_bytes += sdsl::write_member(m_n2, out, child, "n2");
            written_bytes += m_succ0.serialize(out, child, "succ0");
            written_bytes += m_select0.serialize(out, child, "select0");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_bv.load(in);
            m_seq2.load(in);
            m_seq3.load(in);
            sdsl::read_member(m_n2, in);
            m_succ0.load(in, &m_bv);
            m_select0.load(in, &m_bv);
        }

    };

    //! compact_trie needs to know where level 2 ends
    template<>
    struct trie_builder<compact_trie> {
        static compact_trie build(const std::vector<uint32_t> &syms, const std::vector<uint64_t> &lengths,
                                  uint64_t n1) {
            return compact_trie(syms, lengths, n1);
        }
    };

}

#endif //CLTJ_COMPACT_TRIE_H
