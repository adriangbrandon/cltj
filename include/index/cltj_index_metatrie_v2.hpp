//
// Created by Diego Arroyuelo on 20/09/24.
//

#ifndef CLTJ_INDEX_METATRIE_V2_HPP
#define CLTJ_INDEX_METATRIE_V2_HPP

#include <trie/cltj_compact_trie.hpp>
#include <cltj_config.hpp>
#include <cltj_helper.hpp>

namespace cltj {

    template<class Trie>
    class cltj_index_metatrie_v2 {

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



        trie_type create_full_trie(vector<spo_triple> &D, uint8_t order){

            std::sort(D.begin(), D.end(), comparator_order(order));

            uint64_t c0 = 1, cur_value = D[0][spo_orders[order][0]];
            std::vector<uint64_t> v0;
            std::vector<uint64_t> seq;

            for (uint64_t i = 1; i < D.size(); i++) {
                if (D[i][spo_orders[order][0]] != D[i-1][spo_orders[order][0]]) {
                    seq.push_back(cur_value);
                    cur_value = D[i][spo_orders[order][0]];
                    v0.push_back(c0);
                    c0 = 1;
                } else c0++;
            }
            seq.push_back(cur_value);
            v0.push_back(c0);
            
            uint64_t c1, c2;
            std::vector<uint64_t> v1, v2;

            for (uint64_t i = 0, k = 0; i < v0.size(); i++) {
                c1 = 1;
                c2 = 1;
                for (uint64_t j = 1; j < v0[i]; j++) {
                    k++;
                    if (D[k][spo_orders[order][1]] != D[k-1][spo_orders[order][1]]) {
                        v2.push_back(c2);
                        seq.push_back(D[k-1][spo_orders[order][1]]);
                        c2 = 1;
                        c1++;
                    } else c2++;
                }
                seq.push_back(D[k][spo_orders[order][1]]);
                v2.push_back(c2);
                v1.push_back(c1);
                k++;
            }
            uint64_t c = v0.size()+1; 

            for (uint64_t i = 0; i < v1.size(); i++)
                c += v1[i];

            for (uint64_t i = 0; i < v2.size(); i++)
                c += v2[i];

            sdsl::bit_vector bv = sdsl::bit_vector(c, 1);
            sdsl::int_vector<> seq_compact = sdsl::int_vector<>(seq.size()+D.size()+1);

            std::cout << "Full bv.size()=" << bv.size() << " seq=" << seq_compact.size() << std::endl;

            uint64_t j = 0;

            bv[0] = 0;
            bv[v0.size()] = 0;
            j = v0.size();

            for (uint64_t i = 0; i < v1.size(); i++) {
                j += v1[i];
                bv[j] = 0;
            }

            for (uint64_t i = 0; i < v2.size(); i++) {
                j += v2[i];
                bv[j] = 0;
            }

            for (j = 0; j < seq.size(); j++)
                seq_compact[j] = seq[j];

            for (uint64_t i = 0; i < D.size(); i++)
                seq_compact[j++] = D[i][spo_orders[order][2]];
            seq_compact[j] = 0; //mock

/*            cout << "*** Trie para el orden " << (uint64_t)order << " ***" << endl;
            for (uint64_t i = 0; i < bv.size(); i++) {
                cout << bv[i];
            }
            cout << endl;
            for (uint64_t i = 0; i < seq_compact.size(); i++) {
                cout << seq_compact[i] << " ";
            }
            cout << endl;
*/
            return trie_type(bv, seq_compact);            
        }


        trie_type create_partial_trie(vector<spo_triple> &D, uint8_t order){

            std::sort(D.begin(), D.end(), comparator_order(order));

            uint64_t c0 = 1;
            std::vector<uint64_t> v0;
            std::vector<uint64_t> seq;

            for (uint64_t i = 1; i < D.size(); i++) {
                if (D[i][spo_orders[order][0]] != D[i-1][spo_orders[order][0]]) {
                    v0.push_back(c0);
                    c0 = 1;
                } else c0++;
            }
            v0.push_back(c0);

            uint64_t c1, c2;
            std::vector<uint64_t> v1, v2;

            for (uint64_t i = 0, k = 0; i < v0.size(); i++) {
                c1 = 1;
                c2 = 1;
                for (uint64_t j = 1; j < v0[i]; j++) {
                    k++;
                    if (D[k][spo_orders[order][1]] != D[k-1][spo_orders[order][1]]) {
                        v2.push_back(c2);
                        seq.push_back(D[k-1][spo_orders[order][1]]);
                        c2 = 1;
                        c1++;
                    } else c2++;
                }
                seq.push_back(D[k][spo_orders[order][1]]);
                v2.push_back(c2);
                v1.push_back(c1);
                k++;
            }

            uint64_t c = 0;
            for (uint64_t i = 0; i < v1.size(); i++)
                c += v1[i];

            sdsl::bit_vector bv = sdsl::bit_vector(c+1, 1);
            sdsl::int_vector<> seq_compact(seq.size()+1);

            std::cout << "Partial bv.size()=" << bv.size() << " seq=" << seq_compact.size() << std::endl;

            bv[0] = 0;
            uint64_t j = 1;
            for (uint64_t i = 0; i < v1.size(); i++) {
                j += v1[i];
                bv[j-1] = 0;
            }

            for (j = 0; j < seq.size(); j++)
                seq_compact[j] = seq[j];
            seq_compact[seq.size()] = 0; //mock
/*
            cout << "*** Trie para el orden " << (uint64_t)order << " ***" << endl;
            for (uint64_t i = 0; i < bv.size(); i++) {
                cout << bv[i];
            }
            cout << endl;
            for (uint64_t i = 0; i < seq_compact.size(); i++) {
                cout << seq_compact[i] << " ";
            }
            cout << endl;
*/          
            return trie_type(bv, seq_compact);
        }

        void copy(const cltj_index_metatrie_v2 &o){
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
        cltj_index_metatrie_v2() = default;

        /*cltj_index_metatrie_v2(vector<spo_triple> &D) {
            m_tries[0] = create_full_trie(D, 0);  // trie for SPO
            m_tries[1] = create_partial_trie(D, 1); // trie for SOP
	        m_tries[2] = create_full_trie(D, 2);  // trie for POS
	        m_tries[3] = create_partial_trie(D, 3); // trie for PSO
	        m_tries[4] = create_full_trie(D, 4);  // trie for OSP
	        m_tries[5] = create_partial_trie(D, 5); // trie for OPS
        }*/

        cltj_index_metatrie_v2(vector<spo_triple> &D) {
            for(size_type i = 0; i < 6; ++i){
                std::sort(D.begin(), D.end(), comparator_order(i));
                std::vector<uint32_t> syms_root, syms;
                std::vector<size_type> lengths;
                auto end_level = 2 + (i % 2 == 0);
                helper::sym_level(D,  spo_orders[i], 1, end_level, syms, lengths);
                if(i % 2 == 0){
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
        cltj_index_metatrie_v2(const cltj_index_metatrie_v2 &o) {
            copy(o);
        }

        //! Move constructor
        cltj_index_metatrie_v2(cltj_index_metatrie_v2 &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_index_metatrie_v2 &operator=(const cltj_index_metatrie_v2 &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_index_metatrie_v2 &operator=(cltj_index_metatrie_v2 &&o) {
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

        bool insert(const spo_triple &triple) {
            std::cout << "Insert operation is not supported (static version)." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        bool remove(const spo_triple &triple) {
            std::cout << "Remove operation is not supported (static version)." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        void swap(cltj_index_metatrie_v2 &o) {
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

    typedef cltj::cltj_index_metatrie_v2<cltj::compact_trie> compact_ltj_metatrie_v2;
    //typedef cltj::cltj_index_metatrie<cltj::uncompact_trie_v2> uncompact_ltj;

}

#endif 
