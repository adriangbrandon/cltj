/***
BSD 2-Clause License

Copyright (c) 2018, Adrián
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/


//
// Created by Adrián on 21/7/22.
//

#ifndef RING_GAO_HPP
#define RING_GAO_HPP

#include <ring.hpp>
#include <unordered_map>
#include <vector>
#include <utils.hpp>
#include <unordered_set>

namespace ring {

    namespace gao {

        template<class var_t = uint8_t >
        class gao_size {

        public:
            typedef var_t var_type;
            typedef uint64_t size_type;
            typedef struct {
                var_type name;
                size_type n_triples;
                size_type weight;
                std::unordered_set<size_type> related;
            } info_var_type;

        private:
            const std::vector<triple_pattern>* m_ptr_triple_patterns;
            ring* m_ptr_ring;
            std::vector<std::pair<var_type, info_var_type>> m_gao;


            void var_to_vector(const var_type var, const size_type size,
                               std::unordered_map<var_type, size_type> &hash_table,
                               std::vector<info_var_type> &vec){

                auto it = hash_table.find(var);
                if(it == hash_table.end()){
                    info_var_type info;
                    info.name = var;
                    info.n_triples = 1;
                    info.weight = size;
                    vec.emplace_back(info);
                    hash_table.insert({var, vec.size()-1});
                }else{
                    info_var_type& info = vec[it->second];
                    ++info.n_triples;
                    if(info.weight > size){
                        info.weight = size;
                    }
                }
            }

            void var_to_related(const var_type var, const var_type rel,
                                std::unordered_map<var_type, size_type> &hash_table,
                                std::vector<info_var_type> &vec){

                auto pos_var = hash_table[var];
                auto pos_rel = hash_table[rel];
                vec[pos_var].related.insert(pos_rel);
            }

            bool compare(const info_var_type& linfo,
                         const info_var_type& rinfo)
            {
                if(linfo.n_triples > 1 && rinfo.n_triples == 1){
                    return true;
                }
                if(linfo.n_triples == 1 && rinfo.n_triples > 1){
                    return false;
                }
                return linfo.weight < rinfo.weight;
            }

        public:

            gao_size(const std::vector<triple_pattern>* triple_patterns, ring* r){
                m_ptr_triple_patterns = triple_patterns;
                m_ptr_ring = r;

                //std::unordered_map<var_type, std::set<size_type>> related_vars;
                std::vector<info_var_type> var_info;
                std::unordered_map<var_type, size_type> hash_table_position;
                for (const triple_pattern& triple_pattern : *m_ptr_triple_patterns) {
                    size_type size = util::get_size_interval(triple_pattern, m_ptr_ring);
                    var_type var_s = -1, var_p = -1, var_o = -1;
                    std::vector<var_type> rel;
                    if(triple_pattern.s_is_variable()){
                        var_s = (var_type) triple_pattern.term_s.value;
                        var_to_vector(var_s, size,hash_table_position, var_info);
                        rel.push_back(var_s);
                    }
                    if(triple_pattern.p_is_variable()){
                        var_p = (var_type) triple_pattern.term_p.value;
                        var_to_vector(var_p, size,hash_table_position, var_info);
                        rel.push_back(var_p);
                    }
                    if(triple_pattern.o_is_variable()){
                        var_o = triple_pattern.term_o.value;
                        var_to_vector(var_o, size,hash_table_position, var_info);
                        rel.push_back(var_o);
                    }

                    for(size_type i = 0; i < rel.size(); ++i){
                        for(size_type j = 0; j < rel.size(); ++j){
                            if(i == j) continue;
                            var_to_related(rel[i], rel[j], hash_table_position, var_info);
                        }
                    }
                }

                std::vector<var_type> lonely;
                size_type pos_min, weight_min = UINT64_MAX, i = 0;
                for(const auto &var : var_info){
                    if(var.n_triples > 1 && var.weight < weight_min){
                        pos_min = i;
                        weight_min = var.weight;
                    }else if (var.n_triples == 1){
                        lonely.push_back(var);
                    }
                }

                typedef std::pair<size_type, var_type> pair_type;
                typedef std::priority_queue<pair_type, std::vector<pair_type>, greater<pair_type>> min_heap_type;

                std::vector<var_type> gao;
                gao.push_back(var_info[pos_min].var_name);
                min_heap_type heap;
                /*      (*) for()




                m_gao.reserve(hash_table.size());
                for(auto& d : hash_table){
                    m_gao.push_back(d);
                }*/




                std::unordered_map<var_type, info_var_type> hash_table;
                std::unordered_map<var_type, std::set<var_type>> related_vars;
                for (const triple_pattern& triple_pattern : *m_ptr_triple_patterns) { //TODO: esto está ben (solo refactoring)
                    size_type size = util::get_size_interval(triple_pattern, m_ptr_ring);
                    var_type var_s = -1, var_p = -1, var_o = -1;

                    std::vector<var_type> rel;
                    if(triple_pattern.s_is_variable()){
                        var_s = (var_type) triple_pattern.term_s.value;
                        var_to_map(var_s, size,hash_table);
                        rel.push_back(var_s);
                    }
                    if(triple_pattern.p_is_variable()){
                        var_p = (var_type) triple_pattern.term_p.value;
                        var_to_map(var_p, size, hash_table);
                        rel.push_back(var_p);
                    }
                    if(triple_pattern.o_is_variable()){
                        var_o = triple_pattern.term_o.value;
                        var_to_map(var_o, size, hash_table);
                        rel.push_back(var_o);
                    }

                    for(size_type i = 0; i < rel.size(); i++){
                        for(size_type j = 0; j < rel.size(); j++){
                            if(i == j) continue;
                            var_to_related(rel[i], rel[j]);
                        }
                    }
                }





            }

        };

    }
}

#endif //RING_GAO_HPP
