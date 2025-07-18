/*
 * ltj_iterator.hpp
 * Copyright (C) 2023 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LTJ_ITERATOR_BASIC_HPP
#define LTJ_ITERATOR_BASIC_HPP

#include <triple_pattern.hpp>
#include <cltj_config.hpp>
#include <cltj_utils.hpp>
#include <vector>
#include <string>

#define VERBOSE 0

namespace ltj {

    template<class index_scheme_t, class var_t, class cons_t>
    class ltj_iterator_basic {

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef index_scheme_t index_scheme_type;
        typedef uint64_t size_type;

        typedef struct {
            std::array<size_type, 2> it;
            size_type cnt;
            size_type beg;
            size_type end;
        } level_data_type;
        typedef std::array<level_data_type, 4> status_type;
        typedef std::array<bool, 4> redo_array_type;
        //std::vector<value_type> leap_result_type;


    private:
        const triple_pattern *m_ptr_triple_pattern;
        index_scheme_type *m_ptr_index; //TODO: should be const
        //std::vector<std::string> m_orders = {"0 1 2", "0 2 1", "1 2 0", "1 0 2", "2 0 1", "2 1 0"};
        //                                     SPO      SOP      POS      PSO     OSP      OPS
        //Penso que con esto debería ser suficiente (mais parte do de Diego)
        bool m_is_empty = false;
        //std::array<cltj::compact_trie*, 6> m_tries;
        size_type m_nfixed = 0;
        std::array<state_type, 3> m_fixed;

        size_type m_trie_i = 0;
        size_type m_status_i = 0;
        status_type m_status;
        redo_array_type m_redo;


        //std::array<std::vector<size_type>, 2> m_it_v = {std::vector<size_type>(4, 0),
        //                                                std::vector<size_type>(4, 0)};
        //std::array<std::vector<size_type>, 2> m_pos_v = {std::vector<size_type>(4, 1),
        //                                                std::vector<size_type>(4, 1)}; //TODO: remove it
        //std::array<std::vector<size_type>, 2> m_degree_v = {std::vector<size_type>(4, 1),
        //                                                    std::vector<size_type>(4, 1)};
        //TODO: add another vector for the last child
        //std::array<size_type, 2> m_parent_it_v = {0, 0}; // NEW DIEGO
        //std::array<size_type, 2> m_pos_in_parent_v = {1,1}; // NEW DIEGO

        void copy(const ltj_iterator_basic &o) {
            m_ptr_triple_pattern = o.m_ptr_triple_pattern;
            m_ptr_index = o.m_ptr_index;
            m_nfixed = o.m_nfixed;
            m_fixed = o.m_fixed;
            m_is_empty = o.m_is_empty;
            m_trie_i = o.m_trie_i;
            m_status_i = o.m_status_i;
            m_status = o.m_status;
            m_redo = o.m_redo;
        }

        void print_status(){
            std::cout << "fixed: " << m_nfixed << std::endl;
            for(int i = 0; i < m_status.size(); ++i){
                std::cout << "it0=" << m_status[i].it[0] << " it1=" << m_status[i].it[1] << " cnt=" << m_status[i].cnt
                          << " beg=" << m_status[i].beg << " end=" << m_status[i].end << std::endl;
            }
        }

        void print_redo(){
            for(int i = 0; i < m_redo.size(); ++i){
                std::cout << m_redo[i] << " ";
            }
            std::cout << std::endl;
        }

        void process_constants(){

            if(!m_ptr_triple_pattern->s_is_variable()){
                if(!exists(s, m_ptr_triple_pattern->term_s.value)){
                    m_is_empty = true;
                    return;
                }
                down(s);
            }

            if(!m_ptr_triple_pattern->p_is_variable()){
                if(!exists(p, m_ptr_triple_pattern->term_p.value)){
                    m_is_empty = true;
                    return;
                }
                down(p);
            }

            if(!m_ptr_triple_pattern->o_is_variable()){
                if(!exists(o, m_ptr_triple_pattern->term_o.value)){
                    m_is_empty = true;
                    return;
                }
                down(o);
            }

            /*for(auto i = 0; i < 6; ++i){
                std::cout << "Order: ";
                for(auto j = 0; j < 3; ++j){
                    std::cout << (uint) cltj::spo_orders[i][j];
                }
                std::cout << std::endl;
                m_ptr_index->tries[i].print();
            }*/
            //std::cout << "Constants" << std::endl;
            //print_status();
        }

        void choose_trie(state_type state){
            if(m_nfixed == 0) {
                m_trie_i = 2*state;
                m_status_i = 0;
            }else if (m_nfixed == 1){
                if (state == s) { //Fix variables
                    m_trie_i = (m_fixed[m_nfixed-1] == o) ? 4 : 3 ;
                    m_status_i = (m_fixed[m_nfixed-1] == o) ? 0 : 1;
                } else if (state == p) {
                    m_trie_i = (m_fixed[m_nfixed-1] == s) ? 0 : 5 ;
                    m_status_i = (m_fixed[m_nfixed-1] == s) ? 0 : 1;
                } else {
                    m_trie_i = (m_fixed[m_nfixed-1] == p) ? 2 : 1 ;
                    m_status_i = (m_fixed[m_nfixed-1] == p) ? 0 : 1;
                }
            }
        }

    public:
        /*
            Returns the key of the current position of the iterator
        */
        const size_type &nfixed = m_nfixed;


        inline bool is_variable_subject(var_type var) {
            return m_ptr_triple_pattern->term_s.is_variable && var == m_ptr_triple_pattern->term_s.value;
        }

        inline bool is_variable_predicate(var_type var) {
            return m_ptr_triple_pattern->term_p.is_variable && var == m_ptr_triple_pattern->term_p.value;
        }

        inline bool is_variable_object(var_type var) {
            return m_ptr_triple_pattern->term_o.is_variable && var == m_ptr_triple_pattern->term_o.value;
        }
        inline const bool is_empty(){
            return m_is_empty;
        }

        inline size_type parent() const;


        ltj_iterator_basic() = default;
        ltj_iterator_basic(const triple_pattern *triple, index_scheme_type *index) {
            m_ptr_triple_pattern = triple;
            m_ptr_index = index;

            m_status[0].it[0] = 0;
            m_status[0].it[1] = 0;
            m_status[0].beg = 1;
            m_status[0].end = 0;
            m_status[0].cnt = 1;
            //m_status[1][0].it = 2;
            //m_status[1][0].last = 1;
            m_redo[0] = true;
            m_redo[1] = true;
            m_redo[2] = true;
            m_redo[3] = true;

            process_constants();
        }
        /*
        ~ltj_iterator(){
            m_ptr_triple_pattern = nullptr;
            m_ptr_index = nullptr;
        }
        */



        const triple_pattern* get_triple_pattern() const{
            return m_ptr_triple_pattern;
        }
        //! Copy constructor
        ltj_iterator_basic(const ltj_iterator_basic &o) {
            copy(o);
        }

        //! Move constructor
        ltj_iterator_basic(ltj_iterator_basic &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_iterator_basic &operator=(const ltj_iterator_basic &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_iterator_basic &operator=(ltj_iterator_basic &&o) {
            if (this != &o) {
                m_ptr_triple_pattern = std::move(o.m_ptr_triple_pattern);
                m_ptr_index = std::move(o.m_ptr_index);
                m_nfixed = std::move(o.m_nfixed);
                m_fixed = std::move(o.m_fixed);
                m_is_empty = std::move(o.m_is_empty);
                m_trie_i = std::move(o.m_trie_i);
                m_status_i = std::move(o.m_status_i);
                m_status = std::move(o.m_status);
                m_redo = std::move(o.m_redo);
            }
            return *this;
        }

        void swap(ltj_iterator_basic &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_ptr_triple_pattern, o.m_ptr_triple_pattern);
            std::swap(m_ptr_index, o.m_ptr_index);
            std::swap(m_nfixed, o.m_nfixed);
            std::swap(m_fixed, o.m_fixed);
            std::swap(m_is_empty, o.m_is_empty);
            std::swap(m_trie_i, o.m_trie_i);
            std::swap(m_status_i, o.m_status_i);
            std::swap(m_status, o.m_status);
            std::swap(m_redo, o.m_redo);
        }


        void down(state_type state){
            ++m_nfixed;
            m_fixed[m_nfixed-1] = state;

            if(m_nfixed == 1){
                const auto* trie = m_ptr_index->get_trie(m_trie_i);
                auto pos = m_status[m_nfixed].beg;
                m_status[m_nfixed].it[0] = trie->nodeselect(pos);
                trie = m_ptr_index->get_trie(m_trie_i+1);
                m_status[m_nfixed].it[1] = trie->nodeselect(pos);
            }else if (m_nfixed == 2){
                const auto* trie = m_ptr_index->get_trie(m_trie_i);
                auto pos = m_status[m_nfixed].beg;
                m_status[m_nfixed].it[m_status_i] = trie->nodeselect(pos);
            }
            //print_status();
            //m_redo[m_nfixed] = true;
            //print_redo();
        }

        void leap_done(){
            m_redo[m_nfixed] = true;
        }

        void down(var_type var, value_type c) { //Go down in the trie
            state_type state;
            if (is_variable_subject(var)) {
                state = s;
            } else if (is_variable_predicate(var)) {
                state = p;
            } else {
                state = o;
            }
            down(state);
        };

        //Reverses the intervals and variable weights. Also resets the current value.
        void up(var_type var){ //Go up in the trie
            --m_nfixed;
        };



        bool exists(state_type state, size_type c) { //Return the minimum in the range

            choose_trie(state);
            const auto* trie = m_ptr_index->get_trie(m_trie_i);

            size_type beg, end;
            auto cnt = trie->children(parent());
            //auto child = trie->child(parent(), 1)
            beg = trie->first_child(parent());
            end = beg + cnt -1;
            auto p = trie->binary_search_seek(c, beg, end);
            if(p.second > end or p.first != c) return false;
            m_status[m_nfixed+1].beg = p.second;
            m_status[m_nfixed+1].end = end;
            m_status[m_nfixed+1].cnt = cnt;
            m_redo[m_nfixed] = false;
            //print_status();
            return true;
        }

        value_type leap(var_type var, size_type c = -1ULL) { //Return the minimum in the range
            //If c=-1 we need to get the minimum value for the current level.


            state_type state = o;
            if (is_variable_subject(var)) {
                state = s;
            } else if (is_variable_predicate(var)) {
                state = p;
            }
            choose_trie(state);
            const auto* trie = m_ptr_index->get_trie(m_trie_i);
            size_type beg, end, it;
            //std::cout << "Leap redo n_fixed:" << m_nfixed << std::endl;
            //print_redo();
            if(m_redo[m_nfixed]){ //First time of leap
                auto cnt = trie->children(parent());
                beg = trie->first_child(parent());
                end = beg + cnt -1;
                m_status[m_nfixed+1].beg = beg;
                m_status[m_nfixed+1].end = end;
                //m_status[m_nfixed+1].it[0] = m_status[m_nfixed+1].it[1] = it;
                m_status[m_nfixed+1].cnt  = cnt;
                m_redo[m_nfixed] = false;
            }else{
               // std::cout << "Current: " << current() << std::endl;
                beg = m_status[m_nfixed+1].beg;
                end = m_status[m_nfixed+1].end;
            }
            size_type value;
            if(c == -1ULL){
                value = trie->seq[beg];
                m_status[m_nfixed+1].beg = beg; //First position in the sequence
            }else{
                const auto p  = trie->binary_search_seek(c, beg, end);
                if(p.second > end) return 0;
                value = p.first;
                m_status[m_nfixed+1].beg = p.second; //Position of the first value gt c
            }

            //print_status();
            return value;
        }


        bool in_last_level(){
            return m_nfixed == 2;
        }

        inline size_type children(state_type state) const{
            size_type t_i, s_i=0;
            if(m_nfixed == 0) {
                t_i = 2*state;
            }else if (m_nfixed == 1){
                if (state == s) { //Fix variables
                    t_i = (m_fixed[m_nfixed-1] == o) ? 4 : 3 ;
                    s_i = (m_fixed[m_nfixed-1] == o) ? 0 : 1;
                } else if (state == p) {
                    t_i = (m_fixed[m_nfixed-1] == s) ? 0 : 5 ;
                    s_i = (m_fixed[m_nfixed-1] == s) ? 0 : 1;
                } else {
                    t_i = (m_fixed[m_nfixed - 1] == p) ? 2 : 1;
                    s_i = (m_fixed[m_nfixed - 1] == p) ? 0 : 1;
                }
            }else{
                t_i = m_trie_i; //Previously decided
                s_i = m_status_i; //Previously decided
            }
            auto trie = m_ptr_index->get_trie(t_i);
            auto it = m_status[m_nfixed].it[s_i];
            return trie->children(it);
        }

        inline size_type interval_length(var_type var) const {
            state_type state = o;
            if (is_variable_subject(var)) {
                state = s;
            } else if (is_variable_predicate(var)) {
                state = p;
            }
            return children(state);
        }

        inline size_type subtree_size_fixed1(state_type state) const {
            size_type t_i, s_i;
            if (state == s) { //Fix variables
                t_i = (m_fixed[m_nfixed-1] == o) ? 4 : 3 ;
                s_i = (m_fixed[m_nfixed-1] == o) ? 0 : 1;
            } else if (state == p) {
                t_i = (m_fixed[m_nfixed-1] == s) ? 0 : 5 ;
                s_i = (m_fixed[m_nfixed-1] == s) ? 0 : 1;
            } else {
                t_i = (m_fixed[m_nfixed - 1] == p) ? 2 : 1;
                s_i = (m_fixed[m_nfixed - 1] == p) ? 0 : 1;
            }

            const auto* trie =  m_ptr_index->get_trie(t_i);
            auto it = m_status[m_nfixed].it[s_i];

            size_type leftmost_leaf, rightmost_leaf;

            //Count children
            auto cnt = trie->children(it);
            //Leftmost
            auto first = trie->child(it, 1);
            leftmost_leaf = trie->first_child(first);
            //Rightmost
            it = trie->child(it, cnt);
            cnt = trie->children(it);
            rightmost_leaf = trie->first_child(it) + cnt - 1;
            return rightmost_leaf - leftmost_leaf + 1;
        }

        inline size_type subtree_size_fixed2() const {
            const auto* trie =  m_ptr_index->get_trie(m_trie_i);
            auto it = m_status[m_nfixed].it[m_status_i];
            return trie->children(it);
        }



        std::vector<uint64_t> seek_all(var_type x_j){
            std::vector<uint64_t> results;
            const auto* trie = m_ptr_index->get_trie(m_trie_i);
            uint32_t cnt = trie->children(parent());
            size_type beg = trie->first_child(parent());
            for(auto i = beg; i < beg + cnt; ++i){
                results.emplace_back(trie->seq[i]);
            }
            return results;
        }
    };

    template<class index_scheme_t, class var_t, class cons_t>
    uint64_t ltj_iterator_basic<index_scheme_t, var_t, cons_t>::parent() const {
        return m_status[m_nfixed].it[m_status_i];
    }

}

#endif //LTJ_ITERATOR_HPP
