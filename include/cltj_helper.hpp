//
// Created by adrian on 9/10/24.
//

#ifndef CLTJ_HELPER_HPP
#define CLTJ_HELPER_HPP

#include <cstdint>
#include <vector>
#include <cltj_config.hpp>

namespace cltj {

    namespace helper {
         
        static bool equal(const spo_triple &a, const spo_triple &b, const spo_order_type &order, const uint64_t l){
            for(uint64_t i = 0; i <= l; ++i){
                if(a[order[i]] != b[order[i]]) return false;
            }
            return true;
        }

        static bool same_parent(const spo_triple &a, const spo_triple &b, const spo_order_type &order, const uint64_t l) {
            for(uint64_t i = 0; i < l; ++i){
                if(a[order[i]] != b[order[i]]) return false;
            }
            return true;
        }

        static void sym_root(const vector<spo_triple> &D, const spo_order_type &order, std::vector<uint32_t> &syms){
            spo_triple prev, curr;
            for(uint64_t i = 1; i < D.size(); ++i){
                prev = D[i-1]; curr = D[i];
                if(!helper::equal(prev, curr, order, 0)){
                    syms.emplace_back(prev[order[0]]);
                }
            }
            syms.emplace_back(curr[order[0]]);
        }

        static void sym_level(const vector<spo_triple> &D, const spo_order_type &order, uint64_t beg_level, uint64_t end_level,
                      std::vector<uint32_t> &syms, std::vector<uint64_t> &lengths){
            spo_triple prev, curr;
            uint64_t children;
            for(uint32_t l = beg_level; l < end_level; ++l){
                children = 1;
                for(uint64_t i = 1; i < D.size(); ++i){
                    prev = D[i-1]; curr = D[i];
                    if(!helper::equal(prev, curr, order, l)){
                        syms.emplace_back(prev[order[l]]);
                        if(!same_parent(prev, curr, order, l)) {// false when parent is the root
                            lengths.emplace_back(children);
                            children = 1;
                        }else {
                            ++children;
                        }
                    }
                }
                syms.emplace_back(curr[order[l]]);
                lengths.emplace_back(children);
            }
        }

        static void sym_level(vector<spo_triple>::iterator beg, vector<spo_triple>::iterator end, const spo_order_type &order, uint64_t level, uint64_t end_level,
                      std::vector<uint32_t> &syms, std::vector<uint64_t> &lengths){
            vector<spo_triple>::iterator prev, curr;
            uint64_t children;

            for(uint32_t l = level; l < end_level; ++l){
                children = 1;
                prev = beg; curr = beg+1;
                while(curr != end) {
                    if(!helper::equal(*prev, *curr, order, l)){
                        syms.emplace_back(prev->at(order[l]));
                        if(!same_parent(*prev, *curr, order, l)) {// false when parent is the root
                            lengths.emplace_back(children);
                            children = 1;
                        }else {
                            ++children;
                        }
                    }
                    ++curr; ++prev;
                }
                syms.emplace_back(prev->at(order[l]));
                lengths.emplace_back(children);
            }
        }
    }
}
#endif //CLTJ_HELPER_HPP
