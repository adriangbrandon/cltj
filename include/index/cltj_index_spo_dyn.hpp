//
// Created by Adrián on 19/10/23.
//

#ifndef CLTJ_INDEX_SPO_DYN_HPP
#define CLTJ_INDEX_SPO_DYN_HPP


#include <trie/cltj_compact_trie_dyn.hpp>
#include <cltj_helper.hpp>
#include <sdsl/wt_helper.hpp>

namespace cltj {

    template<class Trie>
    class cltj_index_spo_dyn {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef struct {
            bool removed = false;
            std::array<bool, 3> rem_in_dict = {false, false, false}; //ids to remove in the dictionary
        } remove_info_type;
        typedef Trie trie_type;

    private:
        std::array<trie_type, 6> m_tries;
        std::array<size_type, 3> m_gaps;
        size_type m_n_triples = 0;



        void copy(const cltj_index_spo_dyn &o){
            m_tries = o.m_tries;
            m_gaps = o.m_gaps;
            m_n_triples = o.m_n_triples;
        }

    public:

        const std::array<trie_type, 6> &tries = m_tries;
        const std::array<size_type, 3> &gaps = m_gaps;
        const size_type &n_triples = m_n_triples;
        cltj_index_spo_dyn() = default;

        cltj_index_spo_dyn(vector<spo_triple> &D){
            if(D.empty()) return;
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
            m_n_triples = D.size();
        }

        cltj_index_spo_dyn(vector<spo_triple>::iterator beg, vector<spo_triple>::iterator end){
            if(std::distance(beg, end) == 0) return;
            for(size_type i = 0; i < 6; ++i){
                std::sort(beg, end, comparator_order(i));
                std::vector<uint32_t> syms;
                std::vector<size_type> lengths;
                auto beg_level = (i & 0x1);
                helper::sym_level(beg, end, spo_orders[i], beg_level, 3, syms, lengths);
                if(i & 0x1 == 0){
                    m_gaps[i/2] = lengths[0];
                }
                m_tries[i] = trie_type(syms, lengths);
            }
            m_n_triples = std::distance(beg, end);
        }

        //! Copy constructor
        cltj_index_spo_dyn(const cltj_index_spo_dyn &o) {
            copy(o);
        }

        //! Move constructor
        cltj_index_spo_dyn(cltj_index_spo_dyn &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_index_spo_dyn &operator=(const cltj_index_spo_dyn &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_index_spo_dyn &operator=(cltj_index_spo_dyn &&o) {
            if (this != &o) {
                m_tries = std::move(o.m_tries);
                m_gaps = std::move(o.m_gaps);
                m_n_triples = std::move(o.m_n_triples);
            }
            return *this;
        }

        void swap(cltj_index_spo_dyn &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_tries, o.m_tries);
            std::swap(m_gaps, o.m_gaps);
            std::swap(m_n_triples, o.m_n_triples);
        }

        inline trie_type* get_trie(size_type i){
            return &m_tries[i];
        }


        bool insert(const spo_triple &triple) {
            if(!m_n_triples) {
                for(size_type i = 0; i < m_tries.size(); ++i) {
                    m_tries[i] = trie_type(triple, spo_orders[i], i & 0x1, 3);
                }
                m_gaps = {1,1,1};
                ++m_n_triples;
                return true;
            }
            typedef struct {
                size_type pos;
                bool first_child; //pos contains the first_child of the current level
                bool ins;
            } state_type ;
            std::array<state_type, 4> states;
            std::array<bool, 3> inc_gaps = {false, false, false};
            states[0].pos = 0; states[0].first_child = false; states[0].ins = false;
            size_type b, e, gap;
            for(size_type i = 0; i < m_tries.size(); ++i) {
                //std::cout << "Dealing with: " << i << std::endl;
                //m_tries[i].print();
                bool skip_level = i & 0x1;
                bool insert = false;
                for(size_type l = skip_level; l < 3; ++l) {
                    gap = 1;
                    if(skip_level) gap = (l==1) ? 0 : m_gaps[i/2];
                    if(!states[l].ins) {
                        b = (l==0) ? 0 : m_tries[i].child(states[l].pos, 1, gap);
                        e = b+m_tries[i].children(b)-1;
                        auto p = m_tries[i].next(b, e, triple[spo_orders[i][l]]);
                        states[l+1].pos = p.second;
                        states[l+1].first_child = (b==p.second); //first position
                        states[l+1].ins =  p.first != triple[spo_orders[i][l]]; //insert
                        insert = p.first != triple[spo_orders[i][l]];
                    } else {
                        states[l+1].pos = m_tries[i].child(states[l].pos, 1, gap);
                        states[l+1].first_child = false; //it is not the first child of the current range
                        states[l+1].ins = true;
                    }
                }
                if(i == 0 && !insert) return false;
                for(int64_t j = 3; j >= 1+skip_level; --j) {
                    //When the triple is not found in the previous level, it means that we are in the first child (1-bit) of the current level.
                    //Otherwise, we have to add a new child to the current level, thus we add a 0-bit.
                    if(states[j].ins) {
                        //std::cout << "insert at: " << states[j].pos << "[" << states[j-1].ins << ", " << states[j].first_child << "]" << std::endl;
                        m_tries[i].insert(states[j].pos, triple[spo_orders[i][j-1]], states[j-1].ins, states[j].first_child);
                        //m_tries[i].print();
                        if(j == 1) inc_gaps[i/2] = true;
                    }
                }
            }
            for(auto i = 0; i < 3; ++i) {
                m_gaps[i] += inc_gaps[i]; //updating gaps because of insertions in the first level
            }
            ++m_n_triples;
            return true;
            //m_tries[4].print();
            /*for(uint64_t i = 0; i < 6; ++i) {
                m_tries[i].print();
            }*/
        }


        bool remove(const spo_triple &triple) {
            if(!m_n_triples) return false;
            typedef struct {
                size_type pos;
                bool first_child;
                bool rem;
            } state_type ;
            std::array<bool, 3> dec_gaps = {false, false, false};
            std::array<state_type, 4> states;
            states[0].pos = 0; states[0].first_child = true; states[0].rem = false;
            size_type b, e, gap;
            for(size_type i = 0; i < m_tries.size(); ++i) {
            //for(size_type i = 2; i < 3; ++i) {
                bool skip_level = i & 0x1;
                for(size_type l = skip_level; l < 3; ++l) {
                    gap = 1;
                    if(skip_level) gap = (l==1) ? 0 : m_gaps[i/2];
                    b = (l==0) ? 0 : m_tries[i].child(states[l].pos, 1, gap);
                    e = b+m_tries[i].children(b)-1;
                    auto p = m_tries[i].next(b, e, triple[spo_orders[i][l]]);
                    if(p.first != triple[spo_orders[i][l]]) {
                        //m_tries[i].print();
                        return false;
                    }
                    states[l+1].pos = p.second;
                    //states[l+1].first_child = (b==bs.first);
                    states[l+1].rem = (b==e);
                }
                bool rem = true;
                for(int64_t j = 3; j >= 1+skip_level; --j) {
                    if(rem) {
                        //m_tries[i].remove(states[j].pos, states[j].first_child);
                        m_tries[i].remove(states[j].pos, !states[j].rem);
                        if(j == 1) {
                            dec_gaps[i/2] = true;
                        }
                    }
                    rem &= states[j].rem;
                }
                //std::cout << "Fin: " << i << std::endl;
            }
            for(auto i = 0; i < 3; ++i) {
                m_gaps[i] -= dec_gaps[i]; //updating gaps because of deletions in the first level
            }
            --m_n_triples;
            return true;
        }

        remove_info_type remove_and_report(const spo_triple &triple) {
            remove_info_type res;
            if(!m_n_triples) return res;
            typedef struct {
                size_type pos;
                bool first_child;
                bool rem;
            } state_type ;
            std::array<bool, 3> dec_gaps = {false, false, false}; //nodes that are candidates to be removed
            std::array<state_type, 4> states;
            states[0].pos = 0; states[0].first_child = true; states[0].rem = false;
            size_type b, e, gap;
            for(size_type i = 0; i < m_tries.size(); ++i) {
            //for(size_type i = 2; i < 3; ++i) {
                bool skip_level = i & 0x1;
                for(size_type l = skip_level; l < 3; ++l) {
                    gap = 1;
                    if(skip_level) gap = (l==1) ? 0 : m_gaps[i/2];
                    b = (l==0) ? 0 : m_tries[i].child(states[l].pos, 1, gap);
                    e = b+m_tries[i].children(b)-1;
                    auto p = m_tries[i].next(b, e, triple[spo_orders[i][l]]);
                    if(p.first != triple[spo_orders[i][l]]) {
                        //m_tries[i].print();
                        return res;
                    }
                    states[l+1].pos = p.second;
                    //states[l+1].first_child = (b==bs.first);
                    states[l+1].rem = (b==e);
                }
                bool rem = true;
                for(int64_t j = 3; j >= 1+skip_level; --j) {
                    if(rem) {
                        //m_tries[i].remove(states[j].pos, states[j].first_child);
                        m_tries[i].remove(states[j].pos, !states[j].rem);
                        if(j == 1) {
                            dec_gaps[i/2] = true;
                        }
                    }
                    rem &= states[j].rem;
                }
                //std::cout << "Fin: " << i << std::endl;
            }

            //Update degrees in roots
            for(size_type i = 0; i < 3; ++i) {
                m_gaps[i] -= dec_gaps[i]; //updating gaps because of deletions in the first level
            }

            //Compute nodes to remove
            if(dec_gaps[0]) {
                //Check if the node exists as object
                auto p = m_tries[4].next(0, m_gaps[2], triple[0]);
                res.rem_in_dict[0] = (p.first != triple[0]);
            }
            res.rem_in_dict[1] = dec_gaps[1];
            if(triple[0] != triple[2] && dec_gaps[2]) {
                //Check if the node exists as subject
                auto p = m_tries[0].next(0, m_gaps[0], triple[2]);
                res.rem_in_dict[2] = (p.first != triple[2]);
            }
            --m_n_triples;
            res.removed = true;
            return res;
        }

        //Checks if the triple exists, it returns the number of tries where it appears
        //The only possible outputs should be 6 or 0.
        uint64_t test_exists(spo_triple &triple) {
            if(m_n_triples == 0) return 0;
            size_type r = 0;
            std::array<size_type, 4> states;
            states[0] = 0;
            size_type b, e, gap, l;
            bool exists_l0 = true;
            for(size_type i = 0; i < m_tries.size(); ++i) {
                bool skip_level = i & 0x1;
                if(skip_level && !exists_l0) continue;
                for(l = skip_level; l < 3; ++l) {
                    gap = 1;
                    if(skip_level) gap = (l==1) ? 0 : m_gaps[i/2];
                    b = (l==0) ? 0 : m_tries[i].child(states[l], 1, gap);
                    e = b+m_tries[i].children(b)-1;
                    auto bs = m_tries[i].next(b, e, triple[spo_orders[i][l]]); //aqui
                    if(bs.first != triple[spo_orders[i][l]]) {
                        if(l == 0) exists_l0 = false;
                        break;
                    }
                    states[l+1] = bs.second;
                    if(l == 0) exists_l0 = true;
                }
                r += (l == 3);
            }
            return r;
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
            written_bytes += sdsl::write_member(m_n_triples, out, child, "n_triples");
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
            sdsl::read_member(m_n_triples, in);
        }

        void split() {
            for(uint64_t i = 0; i < 6; ++i) {
                m_tries[i].split();
            }
        }

        void flatten() {
            for(uint64_t i = 0; i < 6; ++i) {
                m_tries[i].flatten();
            }
        }

        void print() {
            for(uint64_t i = 0; i < 6; ++i) {
                m_tries[i].print();
            }
        }

        bool check() {
            bool ok = true;
            for(uint64_t i = 0; i < 6; ++i) {
                ok &= m_tries[i].check();
            }
            return ok;
        }

        bool check_last() {
            bool ok = true;
            for(uint64_t i = 0; i < 6; ++i) {
                ok &= m_tries[i].check_last();
            }
            return ok;
        }

        void check_print() {
            bool ok = true;
            for(uint64_t i = 0; i < 6; ++i) {
                std::cout << "------ TRIE=" << i << " ------"<< std::endl;
                m_tries[i].check_print();
                std::cout << std::endl;
            }
        }

        void check_last_print() {
            bool ok = true;
            for(uint64_t i = 0; i < 6; ++i) {
                std::cout << "------ TRIE=" << i << " ------"<< std::endl;
                m_tries[i].check_last_print();
                std::cout << std::endl;
            }
        }

    };

    typedef cltj::cltj_index_spo_dyn<cltj::compact_trie_dyn<>> compact_dyn_ltj;
   // typedef cltj::cltj_index_spo_dyn<cltj::uncompact_trie_v2> uncompact_dyn_ltj;

}

#endif //CLTJ_CLTJ_INDEX_ADRIAN_HPP
