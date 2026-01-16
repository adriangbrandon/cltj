//
// Created by Adrián on 19/10/23.
//

#ifndef CLTJ_INDEX_SPO_LITE_HPP
#define CLTJ_INDEX_SPO_LITE_HPP


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
        sdsl::bit_vector m_subjects;
        sdsl::bit_vector m_objects;
        cds::succ_support_v<1> m_succ_subj;
        cds::succ_support_v<1> m_succ_obj;
        sdsl::rank_support_v<1> m_rank_subj;
        sdsl::rank_support_v<1> m_rank_obj;

        void copy(const cltj_index_spo_lite &o){
            m_tries = o.m_tries;
            m_gaps = o.m_gaps;
            m_subjects = o.m_subjects;
            m_objects = o.m_objects;
            m_succ_subj = o.m_succ_subj;
            m_succ_obj = o.m_succ_obj;
            m_succ_subj.set_vector(&m_subjects);
            m_succ_obj.set_vector(&m_objects);
            m_rank_subj = o.m_rank_subj;
            m_rank_obj = o.m_rank_obj;
            m_rank_subj.set_vector(&m_subjects);
            m_rank_obj.set_vector(&m_objects);
        }

    public:

        const std::array<trie_type, 6> &tries = m_tries;
        const std::array<size_type, 3> &gaps = m_gaps;
        cltj_index_spo_lite() = default;

        cltj_index_spo_lite(vector<spo_triple> &D){

            for(size_type i = 0; i < 6; ++i){
                std::sort(D.begin(), D.end(), comparator_order(i));
                std::vector<uint32_t> syms_root, syms;
                std::vector<size_type> lengths;
                helper::sym_level(D,  spo_orders[i], 1, 3, syms, lengths);
                if(!(i & 0x1)){
                    helper::sym_root(D, spo_orders[i], syms_root);
                    m_gaps[i/2] = syms_root.size();
                    if (i == 0) {
                        m_subjects = sdsl::bit_vector(syms_root.back()+1, 0);
                        for(size_type j = 0; j < m_gaps[0]; ++j){
                            m_subjects[syms_root[j]] = 1;
                        }
                        sdsl::util::init_support(m_succ_subj, &m_subjects);
                        sdsl::util::init_support(m_rank_subj, &m_subjects);
                    }
                    //if (i==2) predicates -> the values are contiguous
                    if (i == 2) {
                        for (size_type j = 0; j < m_gaps[1]; ++j) {
                            if (syms_root[j] != j+1) {
                                std::cerr << "Error: predicates are not contiguous!" << std::endl;
                                std::exit(EXIT_FAILURE);
                            }
                        }
                    }
                    if (i == 4) {
                        m_objects = sdsl::bit_vector(syms_root.back()+1, 0);
                        for(size_type j = 0; j < m_gaps[2]; ++j){
                            m_objects[syms_root[j]] = 1;
                        }
                        sdsl::util::init_support(m_succ_obj, &m_objects);
                        sdsl::util::init_support(m_rank_obj, &m_objects);
                    }
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
                m_subjects = std::move(o.m_subjects);
                m_objects = std::move(o.m_objects);
                m_succ_subj = std::move(o.m_succ_subj);
                m_succ_obj = std::move(o.m_succ_obj);
                m_succ_subj.set_vector(&m_subjects);
                m_succ_obj.set_vector(&m_objects);
                m_rank_subj = std::move(o.m_rank_subj);
                m_rank_obj = std::move(o.m_rank_obj);
                m_rank_subj.set_vector(&m_subjects);
                m_rank_obj.set_vector(&m_objects);
            }
            return *this;
        }

        void swap(cltj_index_spo_lite &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_tries, o.m_tries);
            std::swap(m_gaps, o.m_gaps);
            std::swap(m_subjects, o.m_subjects);
            std::swap(m_objects, o.m_objects);
            sdsl::util::swap_support(m_succ_subj, o.m_succ_subj, &m_subjects, &o.m_subjects);
            sdsl::util::swap_support(m_succ_obj, o.m_succ_obj, &m_objects, &o.m_objects);
            sdsl::util::swap_support(m_rank_subj, o.m_rank_subj, &m_subjects, &o.m_subjects);
            sdsl::util::swap_support(m_rank_obj, o.m_rank_obj, &m_objects, &o.m_objects);
        }

        inline trie_type* get_trie(size_type i){
            return &m_tries[i];
        }

        inline size_type max_subject() const {
            return m_subjects.size()-1;
        }

        inline value_type max_object() const {
            return m_objects.size()-1;
        }

        inline value_type next_object(value_type c) {
            return m_succ_obj(c);
        }

        inline value_type next_subject(value_type c) {
            return m_succ_subj(c);
        }

        inline value_type max_predicate() const {
            return m_gaps[1];
        }

        inline value_type pos_subject(value_type c) const {
            return m_rank_subj(c+1)-1;
        }

        inline value_type pos_object(value_type c) const {
            return m_rank_obj(c+1)-1;
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
            written_bytes += m_subjects.serialize(out, child, "subjects");
            written_bytes += m_objects.serialize(out, child, "objects");
            written_bytes += m_succ_subj.serialize(out, child, "succ_subjects");
            written_bytes += m_succ_obj.serialize(out, child, "succ_objects");
            written_bytes += m_rank_subj.serialize(out, child, "rank_subjects");
            written_bytes += m_rank_obj.serialize(out, child, "rank_objects");
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
            m_subjects.load(in);
            m_objects.load(in);
            m_succ_subj.load(in, &m_subjects);
            m_succ_obj.load(in, &m_objects);
            m_rank_subj.load(in, &m_subjects);
            m_rank_obj.load(in, &m_objects);
        }

    };

    typedef cltj::cltj_index_spo_lite<cltj::compact_trie> compact_ltj;
    typedef cltj::cltj_index_spo_lite<cltj::uncompact_trie> uncompact_ltj;
    typedef cltj::cltj_index_spo_lite<cltj::compact_trie_stats> compact_ltj_stats;

}

#endif //CLTJ_INDEX_SPO_LITE_V2_HPP
