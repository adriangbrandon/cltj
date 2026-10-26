/*
 * cltj_compact_trie_ef.hpp
 *
 * CompactLTJ trie where the children labels of long nodes (degree >= t_long)
 * are stored with Elias-Fano, and the rest in a plain array with one width
 * per level (as compact_trie_lw). Short ranges are searched with galloping
 * from the cursor (as compact_trie_lwg).
 *
 * Topology T (bv, select0, succ0) is the same as in compact_trie, so child()
 * and children() are unchanged. Positions p are positions of L (edges), as in
 * compact_trie: node v has its children at positions [v, v + degree(v) - 1].
 *
 * Alignment ("scheme (b)"): L positions are cut in blocks of s = t_long/2.
 * Since a long range has more than s positions, a block contains at most the
 * tail of one long range and the head of another, and at most one long range
 * starts inside it. Per block we store one word with K[j] = number of long
 * nodes starting before position j*s and D[j] = their total length. Then:
 *   - p belongs to long node K[j] if start[K[j]] <= p, or to node K[j]-1 if
 *     p < start[K[j]-1] + len[K[j]-1]; otherwise p is short.
 *   - a short p is at position p - D[j] of the short array.
 * The iterators pass the degree of the node, so a short node is recognized
 * without reading anything and costs one extra word read per access.
 *
 * Long lists are grouped in density classes (level, w = floor(log2(u/d)),
 * u = last - first + 1) and each class is one sdsl::sd_vector where list k is
 * stored as base_k + (x - first_k). A leap on a long node decodes sequentially a
 * few elements from the cursor (H is scanned with bits::next) and otherwise
 * finds the successor with one select0 on the class.
 *
 * Cursor: the iterators keep, together with the position of the last answer in
 * a node, its position in H ("cur", or no_cursor when unknown) and pass it to
 * access() and binary_search_seek(), which update it. The trie has no state
 * that changes during queries, so one index can be queried from several threads.
 */

#ifndef CLTJ_COMPACT_TRIE_EF_HPP
#define CLTJ_COMPACT_TRIE_EF_HPP

#include <map>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/sd_vector.hpp>
#include <sdsl/select_support_mcl.hpp>
#include <cds/succ_support_v.hpp>
#include <cltj_helper.hpp>

namespace cltj {

    template<uint64_t t_long = 1024>
    class compact_trie_ef_t {

        static_assert(t_long >= 4 && (t_long & (t_long - 1)) == 0, "t_long must be a power of 2 >= 4");

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef sdsl::sd_vector<> ef_type;

        static constexpr uint64_t log2c(uint64_t x) { return x <= 1 ? 0 : 1 + log2c(x >> 1); }
        static const uint64_t lg_s = log2c(t_long) - 1;  // blocks of s = t_long/2 positions
        static const uint64_t scan_elems = 8;                      // sequential decodes before select0
        static const uint64_t no_cursor = ~0ULL;

    private:
        sdsl::bit_vector m_bv;
        cds::succ_support_v<0> m_succ0;
        sdsl::select_support_mcl<0> m_select0;

        // short positions
        sdsl::int_vector<> m_seq2s, m_seq3s;  // labels of short positions of level 2 and 3
        size_type m_n2s = 0;                   // number of short positions of level 2

        // alignment: one word per block j, (K << 32) | D, with K = long nodes starting before
        // position j*s and D = total length of those K nodes = long positions before the short
        // segment of the block. A short position p is at p - D in the short arrays.
        sdsl::int_vector<64> m_KD;

        // long nodes, k = 0..m_nlong-1 in position order. One 64-byte record per node (a cache
        // line), 8 consecutive words of m_hdr:
        //   H_SC   start position in L (low 32 bits) | class << 32
        //   H_LEN  number of children
        //   H_BASE offset of the list in its class sequence (element x is stored as base + x - first)
        //   H_FIRST, H_LAST  first and last label
        //   H_HP0  position in H of the first element (so the first access needs no select);
        //          the elements of the class before this list are H_HP0 - (H_BASE >> wl)
        //   H_R1, H_R2  reserved (zero), to keep one record per cache line
        enum { H_SC = 0, H_LEN, H_BASE, H_FIRST, H_LAST, H_HP0, H_R1, H_R2, H_WORDS };
        size_type m_nlong = 0;
        sdsl::int_vector<64> m_hdr;            // 8 words per long node
        std::vector<ef_type> m_classes;

        inline const uint64_t *hdr(size_type k) const { return m_hdr.data() + k * H_WORDS; }
        static inline uint64_t h_start(const uint64_t *h) { return h[H_SC] & 0xffffffffULL; }
        static inline uint64_t h_cls(const uint64_t *h) { return h[H_SC] >> 32; }
        static inline uint64_t h_ones(const uint64_t *h, const ef_type &sd) { return h[H_HP0] - (h[H_BASE] >> sd.wl); }

        //! position in H of local element a (global rank r) of the list with header h,
        //! given the cursor of that element if known
        static inline uint64_t h_pos(const uint64_t *h, const ef_type &sd, uint64_t a, uint64_t r, uint64_t cur) {
            if (cur != no_cursor) return cur;
            if (a == 0) return h[H_HP0];
            return sd.high_1_select(r);
        }

        void copy(const compact_trie_ef_t &o) {
            m_bv = o.m_bv;
            m_succ0 = o.m_succ0;
            m_succ0.set_vector(&m_bv);
            m_select0 = o.m_select0;
            m_select0.set_vector(&m_bv);
            m_seq2s = o.m_seq2s; m_seq3s = o.m_seq3s; m_n2s = o.m_n2s;
            m_KD = o.m_KD; m_nlong = o.m_nlong;
            m_hdr = o.m_hdr;
            m_classes = o.m_classes;
        }

        //! long node containing position p, or m_nlong if p is short
        inline size_type find_long(size_type p, uint64_t &D) const {
            const uint64_t w = m_KD[p >> lg_s];
            const uint64_t K = w >> 32;
            D = w & 0xffffffffULL;
            if (K < m_nlong && h_start(hdr(K)) <= p) return K;
            if (K > 0) {
                const uint64_t *h = hdr(K - 1);
                if (h_start(h) + h[H_LEN] > p) return K - 1;
            }
            return m_nlong;
        }

        //! value of the r-th one (1-based) of an sd_vector, given its position hp in H
        static inline uint64_t ef_value(const ef_type &sd, uint64_t r, uint64_t hp) {
            return ((hp - r + 1) << sd.wl) | sd.low[r - 1];
        }

        //! first element of sd that is >= target: returns its rank r (1-based) and position hp in H.
        //! Requires that such element exists. One select0 plus a forward scan of H.
        static inline void ef_successor(const ef_type &sd, uint64_t target, uint64_t &r, uint64_t &hp) {
            const uint64_t high_val = target >> sd.wl;
            const uint64_t val_low = target & sdsl::bits::lo_set[sd.wl];
            // H position where bucket high_val starts (after the high_val-th zero)
            hp = high_val ? sd.high_0_select(high_val) + 1 : 0;
            r = hp - high_val + 1;            // rank of the next one (1-based)
            const uint64_t *data = sd.high.data();
            while (true) {
                if ((data[hp >> 6] >> (hp & 63)) & 1ULL) {     // a one in bucket high_val
                    if (sd.low[r - 1] >= val_low) return;
                    ++hp; ++r;
                } else {                                         // bucket exhausted: next one is the successor
                    hp = sdsl::bits::next(data, hp + 1);
                    return;
                }
            }
        }

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
            if (seq[hi - off] < val) return std::make_pair(0, f + 1);
            ++lo;
            while (lo < hi) {
                uint64_t mid = (lo + hi) / 2;
                if (seq[mid - off] < val) lo = mid + 1; else hi = mid;
            }
            return std::make_pair((uint32_t) seq[lo - off], lo);
        }

        //! leap inside long node k from global position i (cursor cur), range ends at f
        std::pair<uint32_t, uint64_t> seek_long(size_type k, uint32_t val, uint64_t i, uint64_t f, uint64_t &cur) const {
            const uint64_t *h = hdr(k);
            if (val > h[H_LAST]) return std::make_pair(0, f + 1);
            const ef_type &sd = m_classes[h_cls(h)];
            const uint64_t st = h_start(h), ob = h_ones(h, sd), base = h[H_BASE], fst = h[H_FIRST];
            uint64_t a = i - st, b = f - st;
            uint64_t r = ob + a + 1;
            uint64_t hp = h_pos(h, sd, a, r, cur);
            uint64_t x = ef_value(sd, r, hp) - base + fst;
            if (x < val) {
                bool found = false;
                for (uint64_t g = 0; g < scan_elems && a < b; ++g) {
                    ++a; ++r;
                    hp = sdsl::bits::next(sd.high.data(), hp + 1);
                    x = ef_value(sd, r, hp) - base + fst;
                    if (x >= val) { found = true; break; }
                }
                if (!found) {
                    // val <= last, so the successor exists
                    ef_successor(sd, base + (val - fst), r, hp);
                    a = r - 1 - ob;
                    x = ef_value(sd, r, hp) - base + fst;
                }
            }
            cur = hp;
            return std::make_pair((uint32_t) x, st + a);
        }

    public:

        const sdsl::bit_vector &bv = m_bv;

        compact_trie_ef_t() = default;

        compact_trie_ef_t(const std::vector<uint32_t> &syms, const std::vector<size_type> &lengths, size_type n1) {
            const size_type n = syms.size();
            m_bv = sdsl::bit_vector(n + 1, 1);
            m_bv[0] = 0;
            size_type pos_bv = 0, n2 = 0;
            for (size_type k = 0; k < lengths.size(); ++k) {
                pos_bv += lengths[k];
                m_bv[pos_bv] = 0;
                if (k < n1) n2 += lengths[k];
            }
            sdsl::util::init_support(m_succ0, &m_bv);
            sdsl::util::init_support(m_select0, &m_bv);

            // long nodes
            std::vector<uint64_t> start, len, first, last, level;
            size_type p = 0, nshort2 = 0, nshort3 = 0;
            for (size_type k = 0; k < lengths.size(); ++k) {
                uint64_t d = lengths[k];
                bool lvl2 = k < n1;
                if (d >= t_long) {
                    start.push_back(p); len.push_back(d);
                    first.push_back(syms[p]); last.push_back(syms[p + d - 1]);
                    level.push_back(lvl2 ? 2 : 3);
                } else {
                    (lvl2 ? nshort2 : nshort3) += d;
                }
                p += d;
            }
            m_nlong = start.size();

            // short arrays
            m_n2s = nshort2;
            m_seq2s = sdsl::int_vector<>(nshort2);
            m_seq3s = sdsl::int_vector<>(nshort3);
            {
                size_type q2 = 0, q3 = 0, kl = 0;
                p = 0;
                for (size_type k = 0; k < lengths.size(); ++k) {
                    uint64_t d = lengths[k];
                    if (kl < m_nlong && start[kl] == p) {
                        ++kl;
                    } else if (k < n1) {
                        for (uint64_t j = 0; j < d; ++j) m_seq2s[q2++] = syms[p + j];
                    } else {
                        for (uint64_t j = 0; j < d; ++j) m_seq3s[q3++] = syms[p + j];
                    }
                    p += d;
                }
            }
            sdsl::util::bit_compress(m_seq2s);
            sdsl::util::bit_compress(m_seq3s);

            // blocks
            if (n >= (1ULL << 32)) throw std::runtime_error("compact_trie_ef: more than 2^32 positions in a trie");
            size_type nblocks = (n >> lg_s) + 1;
            m_KD = sdsl::int_vector<64>(nblocks, 0);
            {
                size_type kl = 0, D = 0;
                for (size_type j = 0; j < nblocks; ++j) {
                    while (kl < m_nlong && start[kl] < (j << lg_s)) { D += len[kl]; ++kl; }
                    m_KD[j] = (kl << 32) | D;
                }
            }

            // classes and headers
            std::map<uint64_t, uint64_t> cls_id;
            std::vector<uint64_t> cls_u, cls_d;
            std::vector<uint64_t> cls(m_nlong), ones(m_nlong), base(m_nlong);
            for (size_type k = 0; k < m_nlong; ++k) {
                uint64_t u = last[k] - first[k] + 1, d = len[k];
                uint64_t w = (u > d) ? sdsl::bits::hi(u / d) : 0;
                uint64_t key = level[k] * 64 + w;
                auto it = cls_id.find(key);
                if (it == cls_id.end()) {
                    it = cls_id.emplace(key, cls_u.size()).first;
                    cls_u.push_back(0); cls_d.push_back(0);
                }
                uint64_t c = it->second;
                cls[k] = c; base[k] = cls_u[c]; ones[k] = cls_d[c];
                cls_u[c] += u; cls_d[c] += d;
            }
            std::vector<sdsl::sd_vector_builder> builders;
            builders.reserve(cls_u.size());
            for (size_type c = 0; c < cls_u.size(); ++c) builders.emplace_back(cls_u[c], cls_d[c]);
            for (size_type k = 0; k < m_nlong; ++k) {
                for (uint64_t j = 0; j < len[k]; ++j) {
                    builders[cls[k]].set(base[k] + syms[start[k] + j] - first[k]);
                }
            }
            m_classes.clear();
            m_classes.reserve(cls_u.size());
            for (auto &b: builders) m_classes.emplace_back(b);

            m_hdr = sdsl::int_vector<64>(m_nlong * H_WORDS, 0);
            for (size_type k = 0; k < m_nlong; ++k) {
                uint64_t *h = m_hdr.data() + k * H_WORDS;
                const ef_type &sd = m_classes[cls[k]];
                h[H_SC] = start[k] | (cls[k] << 32); h[H_LEN] = len[k];
                h[H_BASE] = base[k]; h[H_FIRST] = first[k]; h[H_LAST] = last[k];
                h[H_HP0] = sd.high_1_select(ones[k] + 1);
                if (h_ones(h, sd) != ones[k]) throw std::runtime_error("compact_trie_ef: inconsistent header");
            }
        }

        compact_trie_ef_t(const compact_trie_ef_t &o) { copy(o); }

        compact_trie_ef_t(compact_trie_ef_t &&o) { *this = std::move(o); }

        compact_trie_ef_t &operator=(const compact_trie_ef_t &o) {
            if (this != &o) copy(o);
            return *this;
        }

        compact_trie_ef_t &operator=(compact_trie_ef_t &&o) {
            if (this != &o) {
                m_bv = std::move(o.m_bv);
                m_succ0 = std::move(o.m_succ0);
                m_succ0.set_vector(&m_bv);
                m_select0 = std::move(o.m_select0);
                m_select0.set_vector(&m_bv);
                m_seq2s = std::move(o.m_seq2s); m_seq3s = std::move(o.m_seq3s); m_n2s = o.m_n2s;
                m_KD = std::move(o.m_KD); m_nlong = o.m_nlong;
                m_hdr = std::move(o.m_hdr);
                m_classes = std::move(o.m_classes);
            }
            return *this;
        }

        void swap(compact_trie_ef_t &o) {
            compact_trie_ef_t tmp(std::move(o));
            o = std::move(*this);
            *this = std::move(tmp);
        }

        inline size_type child(uint32_t it, uint32_t n, uint32_t gap = 1) const {
            return m_select0(it + gap + n);
        }

        inline size_type nodeselect(uint32_t it, uint32_t gap = 1) const {
            return child(it, 1, gap);
        }

        size_type children(size_type i) const {
            return m_succ0(i + 1) - i;
        }

        size_type first_child(size_type i) const {
            return i;
        }

        //! Label of the edge at position p of L
        inline value_type access(size_type p) const {
            uint64_t D, cur = no_cursor;
            size_type k = find_long(p, D);
            return access_k(p, k, D, cur);
        }

        //! Same, given the degree of the node and the cursor of position p (or no_cursor),
        //! which is updated. A short node needs only the block word.
        inline value_type access(size_type p, size_type deg, uint64_t &cur) const {
            if (deg < t_long) {
                uint64_t q = p - (m_KD[p >> lg_s] & 0xffffffffULL);
                return (q < m_n2s) ? m_seq2s[q] : m_seq3s[q - m_n2s];
            }
            uint64_t D;
            size_type k = find_long(p, D);
            return access_k(p, k, D, cur);
        }

        //! Appends to out the labels of positions [beg, beg + deg), the children of one node
        void append_children(size_type beg, size_type deg, std::vector<uint64_t> &out) const {
            if (deg < t_long) {
                uint64_t q = beg - (m_KD[beg >> lg_s] & 0xffffffffULL);
                for (uint64_t j = 0; j < deg; ++j, ++q) out.emplace_back((q < m_n2s) ? m_seq2s[q] : m_seq3s[q - m_n2s]);
                return;
            }
            uint64_t D;
            const uint64_t *h = hdr(find_long(beg, D));
            const ef_type &sd = m_classes[h_cls(h)];
            uint64_t r = h_ones(h, sd) + 1, hp = h[H_HP0];
            for (uint64_t j = 0; j < deg; ++j, ++r) {
                if (j) hp = sdsl::bits::next(sd.high.data(), hp + 1);
                out.emplace_back(ef_value(sd, r, hp) - h[H_BASE] + h[H_FIRST]);
            }
        }

    private:
        inline value_type access_k(size_type p, size_type k, uint64_t D, uint64_t &cur) const {
            if (k < m_nlong) {
                const uint64_t *h = hdr(k);
                const ef_type &sd = m_classes[h_cls(h)];
                uint64_t a = p - h_start(h), r = h_ones(h, sd) + a + 1;
                uint64_t hp = h_pos(h, sd, a, r, cur);
                cur = hp;                              // a leap usually starts here next
                return ef_value(sd, r, hp) - h[H_BASE] + h[H_FIRST];
            }
            uint64_t q = p - D;
            return (q < m_n2s) ? m_seq2s[q] : m_seq3s[q - m_n2s];
        }

        inline std::pair<uint32_t, uint64_t> seek_short(uint32_t val, uint64_t i, uint64_t f, uint64_t D) const {
            std::pair<uint32_t, uint64_t> r;
            if (f - D < m_n2s) r = gallop(m_seq2s, 0, val, i - D, f - D);
            else r = gallop(m_seq3s, m_n2s, val, i - D, f - D);
            r.second += D;
            return r;
        }

    public:

        //! Smallest position k in [i, f] with L[k] >= val, and L[k]; (0, f+1) if none.
        //! [i, f] is (a suffix of) the child range of one node.
        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint64_t i, uint64_t f) const {
            uint64_t D, cur = no_cursor;
            size_type k = find_long(i, D);
            if (k < m_nlong) return seek_long(k, val, i, f, cur);
            return seek_short(val, i, f, D);
        }

        //! Same, given the degree of the node and the cursor of position i (or no_cursor).
        //! On success, cur becomes the cursor of the answer.
        std::pair<uint32_t, uint64_t> binary_search_seek(uint32_t val, uint64_t i, uint64_t f, uint64_t deg,
                                                         uint64_t &cur) const {
            if (deg < t_long) return seek_short(val, i, f, m_KD[i >> lg_s] & 0xffffffffULL);
            uint64_t D;
            size_type k = find_long(i, D);
            return seek_long(k, val, i, f, cur);
        }

        size_type long_nodes() const { return m_nlong; }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_bv.serialize(out, child, "bv");
            written_bytes += m_succ0.serialize(out, child, "succ0");
            written_bytes += m_select0.serialize(out, child, "select0");
            written_bytes += m_seq2s.serialize(out, child, "seq2_short");
            written_bytes += m_seq3s.serialize(out, child, "seq3_short");
            written_bytes += sdsl::write_member(m_n2s, out, child, "n2_short");
            written_bytes += m_KD.serialize(out, child, "KD");
            written_bytes += sdsl::write_member(m_nlong, out, child, "nlong");
            written_bytes += m_hdr.serialize(out, child, "long_headers");
            uint64_t ncls = m_classes.size();
            written_bytes += sdsl::write_member(ncls, out, child, "nclasses");
            for (const auto &c: m_classes) written_bytes += c.serialize(out, child, "ef_class");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_bv.load(in);
            m_succ0.load(in, &m_bv);
            m_select0.load(in, &m_bv);
            m_seq2s.load(in);
            m_seq3s.load(in);
            sdsl::read_member(m_n2s, in);
            m_KD.load(in);
            sdsl::read_member(m_nlong, in);
            m_hdr.load(in);
            uint64_t ncls = 0;
            sdsl::read_member(ncls, in);
            m_classes.clear();
            m_classes.resize(ncls);
            for (auto &c: m_classes) c.load(in);
        }
    };

    typedef compact_trie_ef_t<1024> compact_trie_ef;   // lists of >= 1024 children in Elias-Fano

    template<uint64_t t_long>
    struct trie_builder<compact_trie_ef_t<t_long>> {
        static compact_trie_ef_t<t_long> build(const std::vector<uint32_t> &syms,
                                               const std::vector<uint64_t> &lengths, uint64_t n1) {
            return compact_trie_ef_t<t_long>(syms, lengths, n1);
        }
    };
}

#endif //CLTJ_COMPACT_TRIE_EF_HPP
