//
// Created by Adrián on 06/12/24.
//

#ifndef CLTJ_INDEX_METATRIE_DYN_HPP
#define CLTJ_INDEX_METATRIE_DYN_HPP

#include <metatrie/cltj_compact_metatrie_dyn.hpp>
#include <cltj_config.hpp>

namespace cltj {

    template<class Trie>
    class cltj_index_metatrie_dyn {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef Trie trie_type;
        typedef struct {
            bool removed = false;
            std::array<bool, 3> rem_in_dict = {false, false, false}; //ids to remove in the dictionary
        } remove_info_type;

    private:
        std::array<trie_type, 6> m_tries;
        size_type m_n_triples = 0;

        trie_type create_full_trie(spo_triple triple, uint8_t order) {
            sdsl::int_vector<> seq(4);
            seq[0] = triple[spo_orders[order][0]];
            seq[1] = triple[spo_orders[order][1]];
            seq[2] = triple[spo_orders[order][2]];
            seq[3] = 0; //mock
            auto bv = sdsl::bit_vector(4, 1);
            return trie_type(seq, bv);
        }

        trie_type create_partial_trie(spo_triple triple, uint8_t order) {
            sdsl::int_vector<> seq(2);
            seq[0] = triple[spo_orders[order][1]];
            seq[1] = 0; //mock
            auto bv = sdsl::bit_vector(2, 1);
            return trie_type(seq, bv);
        }

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

            sdsl::bit_vector bv = sdsl::bit_vector(c, 0);
            sdsl::int_vector<> seq_compact = sdsl::int_vector<>(seq.size()+D.size()+1);

            uint64_t j = 0;

            bv[0] = 1;
            bv[v0.size()] = 1;
            j = v0.size();

            for (uint64_t i = 0; i < v1.size(); i++) {
                j += v1[i];
                bv[j] = 1;
            }

            for (uint64_t i = 0; i < v2.size(); i++) {
                j += v2[i];
                bv[j] = 1;
            }

            for (j = 0; j < seq.size(); j++)
                seq_compact[j] = seq[j];

            for (uint64_t i = 0; i < D.size(); i++)
                seq_compact[j++] = D[i][spo_orders[order][2]];
            seq_compact[j]=0; //mock

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
            return trie_type(seq_compact, bv);
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

            sdsl::bit_vector bv = sdsl::bit_vector(c+1, 0);
            sdsl::int_vector<> seq_compact = sdsl::int_vector<>(seq.size()+1);

            bv[0] = 1;
            uint64_t j = 1;
            for (uint64_t i = 0; i < v1.size(); i++) {
                j += v1[i];
                bv[j-1] = 1;
            }

            for (j = 0; j < seq.size(); j++)
                seq_compact[j] = seq[j];
            seq_compact[seq.size()] = 0; //mock
            return trie_type(seq_compact, bv);
        }

        void copy(const cltj_index_metatrie_dyn &o){
            m_tries = o.m_tries;
            m_n_triples = o.m_n_triples;
        }

    public:

        const std::array<trie_type, 6> &tries = m_tries;
        cltj_index_metatrie_dyn() = default;

        cltj_index_metatrie_dyn(vector<spo_triple> &D) {
            if(D.empty()) return;
            m_tries[0] = create_full_trie(D, 0);  // trie for SPO
            m_tries[1] = create_partial_trie(D, 1); // trie for SOP
	        m_tries[2] = create_full_trie(D, 2);  // trie for POS
	        m_tries[3] = create_partial_trie(D, 3); // trie for PSO
	        m_tries[4] = create_full_trie(D, 4);  // trie for OSP
	        m_tries[5] = create_partial_trie(D, 5); // trie for OPS
            m_n_triples = D.size();
        }

        //! Copy constructor
        cltj_index_metatrie_dyn(const cltj_index_metatrie_dyn &o) {
            copy(o);
        }

        //! Move constructor
        cltj_index_metatrie_dyn(cltj_index_metatrie_dyn &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_index_metatrie_dyn &operator=(const cltj_index_metatrie_dyn &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_index_metatrie_dyn &operator=(cltj_index_metatrie_dyn &&o) {
            if (this != &o) {
                m_tries = std::move(o.m_tries);
                m_n_triples = std::move(o.m_n_triples);
            }
            return *this;
        }

        void swap(cltj_index_metatrie_dyn &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_tries, o.m_tries);
            std::swap(m_n_triples, o.m_n_triples);
        }

        inline trie_type* get_trie(size_type i){
            return &m_tries[i];
        }

        bool insert_old(const spo_triple &triple) {
            if(!m_n_triples) {
                m_tries[0] = create_full_trie(triple, 0);
                m_tries[1] = create_partial_trie(triple, 1);
                m_tries[2] = create_full_trie(triple, 2);
                m_tries[3] = create_partial_trie(triple, 3);
                m_tries[4] = create_full_trie(triple, 4);
                m_tries[5] = create_partial_trie(triple, 5);
                for(size_type i = 0; i < m_tries.size(); i+=2) {
                    m_tries[i].inc_root_degree();
                }
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
            size_type b, e;
            for(size_type i = 0; i < m_tries.size(); ++i) {
                bool skip_level = i & 0x1;
                if(skip_level) { //partial trie => just level = 1
                    if(!states[1].ins) { //the previous element exists in the previous level
                        b = m_tries[i].child(states[1].pos, 1, 0);
                        e = b+m_tries[i].children(b)-1;
                        auto p = m_tries[i].next(b, e, triple[spo_orders[i][1]]);
                        if(p.first != triple[spo_orders[i][1]]) {
                            m_tries[i].insert(p.second, triple[spo_orders[i][1]], states[1].ins, (b==p.second));
                        }
                    }else {
                        b = m_tries[i].child(states[1].pos, 1, 0);
                        m_tries[i].insert(b, triple[spo_orders[i][1]], states[1].ins, false);
                    }
                }else { //global tries
                    bool insert = false;
                    for(size_type l = 0; l < 3; ++l) {
                        if(!states[l].ins) {
                            b = (l==0) ? 0 : m_tries[i].child(states[l].pos, 1);
                            e = b+m_tries[i].children(b)-1;
                            auto p = m_tries[i].next(b, e, triple[spo_orders[i][l]]);
                            states[l+1].pos = p.second;
                            states[l+1].first_child = (b==p.second); //first position
                            states[l+1].ins =  p.first != triple[spo_orders[i][l]]; //insert
                            insert = p.first != triple[spo_orders[i][l]];
                        } else {
                            states[l+1].pos = m_tries[i].child(states[l].pos, 1);
                            states[l+1].first_child = false; //it is not the first child of the current range
                            states[l+1].ins = true;
                        }
                    }
                    if(i == 0 && !insert) return false;
                    for(int64_t j = 3; j >= 1; --j) {
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
            }

            //Update root degree because insertion of a new element in the first level
            for(auto i = 0; i < m_tries.size(); i+=2) {
                if(inc_gaps[i/2]) m_tries[i].inc_root_degree();
            }
            ++m_n_triples;
            return true;
        }

        bool insert(const spo_triple &triple) {
            if(!m_n_triples) {
                m_tries[0] = create_full_trie(triple, 0);
                m_tries[1] = create_partial_trie(triple, 1);
                m_tries[2] = create_full_trie(triple, 2);
                m_tries[3] = create_partial_trie(triple, 3);
                m_tries[4] = create_full_trie(triple, 4);
                m_tries[5] = create_partial_trie(triple, 5);
                for(size_type i = 0; i < m_tries.size(); i+=2) {
                    m_tries[i].inc_root_degree();
                }
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
            std::array<bool, 3> ins_part = {false, false, false};
            std::array<state_type, 3> u_part; //info to update partial trie
            states[0].pos = 0; states[0].first_child = false; states[0].ins = false;
            size_type b, e;
            for(size_type i = 0; i < m_tries.size(); i+=2) {
                bool insert = false;
                for(size_type l = 0; l < 3; ++l) {
                    if(!states[l].ins) {
                        b = (l==0) ? 0 : m_tries[i].child(states[l].pos, 1);
                        e = b+m_tries[i].children(b)-1;
                        auto p = m_tries[i].next(b, e, triple[spo_orders[i][l]]);
                        states[l+1].pos = p.second;
                        states[l+1].first_child = (b==p.second); //first position
                        states[l+1].ins =  p.first != triple[spo_orders[i][l]]; //insert
                        insert = p.first != triple[spo_orders[i][l]];
                    } else {
                        states[l+1].pos = m_tries[i].child(states[l].pos, 1);
                        states[l+1].first_child = false; //it is not the first child of the current range
                        states[l+1].ins = true;
                    }
                }
                if(i == 0 && !insert) return false;
                for(int64_t j = 3; j >= 1; --j) {
                    //When the triple is not found in the previous level, it means that we are in the first child (1-bit) of the current level.
                    //Otherwise, we have to add a new child to the current level, thus we add a 0-bit.
                    if(states[j].ins) {
                        //std::cout << "insert at: " << states[j].pos << "[" << states[j-1].ins << ", " << states[j].first_child << "]" << std::endl;
                        m_tries[i].insert(states[j].pos, triple[spo_orders[i][j-1]], states[j-1].ins, states[j].first_child);
                        //m_tries[i].print();
                        if (j == 1) inc_gaps[i/2] = true;
                        if (j == 2) { //insert into partial trie
                            auto pt = ts_part_map[i/2];
                            ins_part[pt/2] = true;
                            u_part[i/2] = states[1]; //to sync with the first level in the partial trie
                        }
                    }
                }
            }

            for (uint64_t i = 1; i < m_tries.size(); i+=2) {
                if (ins_part[i/2]) {
                    if (!u_part[i/2].ins) {
                        //the previous element exists in the previous level
                        b = m_tries[i].child(u_part[i/2].pos, 1, 0);
                        e = b+m_tries[i].children(b)-1;
                        auto p = m_tries[i].next(b, e, triple[spo_orders[i][1]]);
                        m_tries[i].insert(p.second, triple[spo_orders[i][1]], u_part[i/2].ins, (b==p.second));
                    }else {
                        b = m_tries[i].child(u_part[i/2].po, 1, 0);
                        m_tries[i].insert(b, triple[spo_orders[i][1]], states[1].ins, false);
                    }
                }
            }

            //Update root degree because insertion of a new element in the first level
            for(auto i = 0; i < m_tries.size(); i+=2) {
                if(inc_gaps[i/2]) m_tries[i].inc_root_degree();
            }
            ++m_n_triples;
            return true;
        }

        bool remove(const spo_triple &triple) {
            if(!m_n_triples) return false;
            typedef struct {
                size_type pos;
                bool rem;
            } state_type ;
            std::array<bool, 3> dec_gaps = {false, false, false};
            std::array<state_type, 3> u_part; //info to update partial trie
            std::array<state_type, 4> states;
            states[0].pos = 0; states[0].rem = false;
            size_type b, e, gap;
            for(size_type i = 0; i < m_tries.size(); i+=2) { //just full tries
                for(size_type l = 0; l < 3; ++l) {
                    b = (l==0) ? 0 : m_tries[i].child(states[l].pos, 1);
                    e = b+m_tries[i].children(b)-1;
                    auto p = m_tries[i].next(b, e, triple[spo_orders[i][l]]);
                    if(p.first != triple[spo_orders[i][l]]) {
                        //m_tries[i].print();
                        return false;
                    }
                    states[l+1].pos = p.second;
                    //states[l+1].first_child = (b==bs.first);
                    states[l+1].rem = (b==e); //
                }
                bool rem = true; //starts removing in the last level
                for(int64_t j = 3; j >= 1; --j) {
                    if(rem) {
                        //m_tries[i].remove(states[j].pos, states[j].first_child);
                        m_tries[i].remove(states[j].pos, !states[j].rem);
                        if(j == 1) dec_gaps[i/2] = true;
                    }
                    rem &= states[j].rem;
                }
                u_part[i/2].pos = states[1].pos; //to sync with the first level
                auto pt = ts_part_map[i/2];
                u_part[pt/2].rem = states[3].rem; //true => the last level of the subtree is empty
            }

            //Update partial trees
            for(uint i = 0; i < 3; ++i) {
                if(u_part[i].rem) {
                    auto pt = 2*i+1; //partial trie
                    //[b,e] is the range after a down in the first level
                    b = m_tries[pt].child(u_part[i].pos, 1, 0);
                    e = b+m_tries[pt].children(b)-1;
                    //look for the position of the second level
                    auto p = m_tries[pt].next(b, e, triple[spo_orders[pt][1]]); //must exist
                    m_tries[pt].remove(p.second, b!=e); //remove it
                }
            }
            //Updating root degree
            for(auto i = 0; i < m_tries.size(); i+=2) {
                if(dec_gaps[i/2]) m_tries[i].dec_root_degree();
            }
            --m_n_triples;
            return true;
        }

        remove_info_type remove_and_report(const spo_triple &triple) {
            remove_info_type res;
            if(!m_n_triples) return res;
            typedef struct {
                size_type pos;
                bool rem;
            } state_type ;
            std::array<bool, 3> dec_gaps = {false, false, false};
            std::array<state_type, 3> u_part; //info to update partial trie
            std::array<state_type, 4> states;
            states[0].pos = 0; states[0].rem = false;
            size_type b, e, gap;
            for(size_type i = 0; i < m_tries.size(); i+=2) { //just full tries
                for(size_type l = 0; l < 3; ++l) {
                    b = (l==0) ? 0 : m_tries[i].child(states[l].pos, 1);
                    e = b+m_tries[i].children(b)-1;
                    auto p = m_tries[i].next(b, e, triple[spo_orders[i][l]]);
                    if(p.first != triple[spo_orders[i][l]]) {
                        //m_tries[i].print();
                        return res;
                    }
                    states[l+1].pos = p.second;
                    //states[l+1].first_child = (b==bs.first);
                    states[l+1].rem = (b==e); //
                }
                bool rem = true; //starts removing in the last level
                for(int64_t j = 3; j >= 1; --j) {
                    if(rem) {
                        //m_tries[i].remove(states[j].pos, states[j].first_child);
                        m_tries[i].remove(states[j].pos, !states[j].rem);
                        if(j == 1) dec_gaps[i/2] = true;
                    }
                    rem &= states[j].rem;
                }
                u_part[i/2].pos = states[1].pos; //to sync with the first level
                auto pt = ts_part_map[i/2];
                u_part[pt/2].rem = states[3].rem; //true => the last level of the subtree is empty
            }

            //Update partial trees
            for(uint i = 0; i < 3; ++i) {
                if(u_part[i].rem) {
                    auto pt = 2*i+1; //partial trie
                    //[b,e] is the range after a down in the first level
                    b = m_tries[pt].child(u_part[i].pos, 1, 0);
                    e = b+m_tries[pt].children(b)-1;
                    //look for the position of the second level
                    auto p = m_tries[pt].next(b, e, triple[spo_orders[pt][1]]); //must exist
                    m_tries[pt].remove(p.second, b!=e); //remove it
                }
            }
            //Updating root degree
            for(auto i = 0; i < m_tries.size(); i+=2) {
                if(dec_gaps[i/2]) m_tries[i].dec_root_degree();
            }
            //Compute nodes to remove
            if(dec_gaps[0]) {
                //Check if the node exists as object
                auto p = m_tries[4].next(0, m_tries[4].root_degree(), triple[0]);
                res.rem_in_dict[0] = (p.first != triple[0]);
            }
            res.rem_in_dict[1] = dec_gaps[1];
            if(triple[0] != triple[2] && dec_gaps[2]) {
                //Check if the node exists as subject
                auto p = m_tries[0].next(0, m_tries[0].root_degree(), triple[2]);
                res.rem_in_dict[2] = (p.first != triple[2]);
            }
            --m_n_triples;
            res.removed = true;
            return res;
        }

        //Checks if the triple exists, it returns the number of tries where it appears
        //The only possible outputs should be 3 or 0.
        uint64_t test_exists(const spo_triple &triple) {
            if(m_n_triples == 0) return 0;
            size_type r = 0;
            std::array<size_type, 4> states;
            states[0] = 0;
            size_type b, e, l;
            for(size_type i = 0; i < m_tries.size(); i+=2) {
                for(l = 0; l < 3; ++l) {
                    b = (l==0) ? 0 : m_tries[i].child(states[l], 1);
                    e = b+m_tries[i].children(b)-1;
                    auto bs = m_tries[i].next(b, e, triple[spo_orders[i][l]]); //aqui
                    if(bs.first != triple[spo_orders[i][l]]) {
                        break;
                    }
                    states[l+1] = bs.second;
                }
                r += (l == 3);
            }
            return r;
        }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            sdsl::write_member(m_n_triples, out, child, "ntriples");
            for(const auto & trie : m_tries){
                written_bytes += trie.serialize(out, child, "tries");
            }
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            sdsl::read_member(m_n_triples, in);
            for(auto & trie : m_tries){
                trie.load(in);
            }
        }

        bool check() {
            bool ok = true;
            for(uint64_t i = 0; i < 6; ++i) {
                ok &= m_tries[i].check();
                if(!ok) {
                    std::cout << "Error in trie: " << i << std::endl;
                    break;
                }
            }
            return ok;
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

        bool check_leaves() {
            bool ok = true;
            for(uint64_t i = 0; i < 6; ++i) {
                ok &= m_tries[i].check_leaves();
                if(!ok) {
                    std::cout << "Error in trie: " << i << std::endl;
                    break;
                }
            }
            return ok;
        }

    };

    typedef cltj::cltj_index_metatrie_dyn<cltj::compact_metatrie_dyn<>> compact_ltj_metatrie_dyn;
    //typedef cltj::cltj_index_metatrie_dyn<cltj::uncompact_trie_v2> uncompact_ltj;

}

#endif 
