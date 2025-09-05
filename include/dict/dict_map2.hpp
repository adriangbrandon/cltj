/*
 * dict_map.cpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
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

#ifndef DICT_MAP2_HPP
#define DICT_MAP2_HPP

#include <dict/pfc2.hpp>
#include <map>
#include <sdsl/int_vector.hpp>
#include <cltj_config.hpp>

using namespace std;

namespace dict
{

  /**
   * @brief Dictionary that maps the user symbols to IDs used by the ring
   * It uses a binary tree with Plain Front Coding as the leaves.
   * It also has an array mapping every ID with its corresponding PFC
   *
   * @tparam MINSIZE The minimum words a PFC can have before being fused with its sibling
   * @tparam MAXSIZE The maximum words a PFC can have before splitting
   */
  template <uint64_t MINSIZE, uint64_t MAXSIZE>
  class dict_map2
  {
  public:
    typedef uint64_t size_type;
    typedef uint64_t value_type;

  private:
    class node2;
    node2 *root = NULL;
    std::vector<EmptyOrPFC2> id_map;
    //Cached strings
    std::vector<std::string> cache;
    // Values used to represent the Queue of free IDs
    uint64_t first_empty = 0, last_empty = 0, free_ids_size = 0;

    void copy(const dict_map2 &o) {
      std::unordered_map<PFC2*, PFC2*> ht; //mapping between PFCs
      root = (o.root)->clone(ht);
      cache = o.cache;
      first_empty = o.first_empty;
      last_empty = o.last_empty;
      free_ids_size = o.free_ids_size;
      id_map = o.id_map;
      for(uint64_t i = 0; i < id_map.size(); ++i) {
        auto it = ht.find(id_map[i].pfc);
        if(it != ht.end()) {
          id_map[i].pfc = it->second;
        }
      }
    }

  public:
    dict_map2()
    {
      root = new node2();
    }

    explicit dict_map2(std::string &val)
    {
      root = new node2(val, 1);
      id_map.push_back({ .pfc = root->get_pfc()});
    }


    //Bulk load from a map
    explicit dict_map2(std::map<std::string, uint64_t> &dict)
    {
      auto pfc_size = (MAXSIZE + MINSIZE) / 2;
      auto size = (dict.size() + pfc_size - 1) / pfc_size;

      //1. Building the leaves
      std::vector<node2*> nodes(size);
      size_type pfc_i;
      for(pfc_i = 0; pfc_i < size; ++pfc_i) {
        nodes[pfc_i] = new node2();
      }

      //2. Appending the strings in the leaves
      pfc_i = 0;
      size_type k = 0;
      std::string prev = "\0";
      id_map.resize(dict.size());
      for(auto &p : dict) {
        id_map[p.second-1].pfc = nodes[pfc_i]->get_pfc();
        nodes[pfc_i]->get_pfc()->append(p.first, p.second, prev);
        prev = p.first; ++k;
        if(k % pfc_size == 0) {
          ++pfc_i;
          prev = "\0";
        }
      }
      dict.clear(); //delete

      //3. Building the tree
      while(size > 1) {
        for(pfc_i = 0; pfc_i < size - (size % 2); pfc_i += 2) {
          node2* n = new node2(nodes[pfc_i], nodes[pfc_i+1]);
          nodes[pfc_i / 2] = n;
        }
        if(size % 2 == 1) {
          nodes[(size-1) / 2] = nodes[size-1];
        }
        size = (size+1) / 2;
      }
      root = nodes[0];
    }

    //! Copy constructor
    dict_map2(const dict_map2 &o) {
      copy(o);
    }

    //! Move constructor
    dict_map2(dict_map2 &&o) {
      *this = std::move(o);
      o.root = NULL;
    }

    //! Copy Operator=
    dict_map2 &operator=(const dict_map2 &o) {
      if (this != &o) {
        copy(o);
      }
      return *this;
    }

    //! Move Operator=
    dict_map2 &operator=(dict_map2 &&o) {
      if (this != &o) {
        root = o.root;
        o.root = NULL; //prevent remove dynamic info
        cache = std::move(o.cache);
        id_map = std::move(o.id_map);
        first_empty = o.first_empty;
        last_empty = o.last_empty;
        free_ids_size = o.free_ids_size;
      }
      return *this;
    }

    void swap(dict_map2 &o) {
      // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
      std::swap(root, o.root);
      std::swap(cache, o.cache);
      std::swap(id_map, o.id_map);
      std::swap(first_empty, o.first_empty);
      std::swap(last_empty, o.last_empty);
      std::swap(free_ids_size, o.free_ids_size);
    }

    ~dict_map2()
    {
      if(root != NULL) {
        root->free_mem();
        delete root;
      }

    }



    /**
     * @brief Function that serializes the data structure.
     *
     * @param out The out stream where the bytes are being written
     * @return uint64_t The amount of bytes written
     */
    uint64_t serialize(std::ostream &out)
    {
      uint64_t w_bytes = 0;
      size_t map_size = id_map.size();

      out.write((char *)&map_size, sizeof(map_size));
      w_bytes += sizeof(map_size);
      w_bytes += root->serialize(out);
      out.write((char *)&free_ids_size, sizeof(uint64_t));
      w_bytes += sizeof(uint64_t);
      out.write((char *)&first_empty, sizeof(uint64_t));
      w_bytes += sizeof(uint64_t);

      if (free_ids_size > 0)
      {
        // For every empty slot write the next empty
        for (uint64_t i = 0; i < free_ids_size - 1; i++)
        {
          out.write((char *)&id_map[first_empty - 1].next_empty, sizeof(uint64_t));
          first_empty = id_map[first_empty - 1].next_empty;
          w_bytes += sizeof(uint64_t);
        }
      }

      return w_bytes;
    }

    //! Serializes the data structure into the given ostream
    uint64_t serialize(ostream &out, sdsl::structure_tree_node *v = nullptr, string name = "") const
    {
      sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, "dict_map");
      uint64_t written_bytes = 0;
      size_t map_size = id_map.size();

      written_bytes += sdsl::write_member(map_size, out, child, "map_size");
      written_bytes += root->serialize(out);
      written_bytes += sdsl::write_member(free_ids_size, out, child, "free_ids_size");
      written_bytes += sdsl::write_member(first_empty, out, child, "first_empty");
      if (free_ids_size > 0)
      {
        // For every empty slot write the next empty
        auto empty_ptr = first_empty;
        for (uint64_t i = 0; i < free_ids_size - 1; i++)
        {
          out.write((char *)&id_map[empty_ptr - 1].next_empty, sizeof(uint64_t));
          empty_ptr = id_map[empty_ptr - 1].next_empty;
          written_bytes += sizeof(uint64_t);
        }
      }
      sdsl::structure_tree::add_size(child, written_bytes);

      return written_bytes;
    }

    /**
     * @brief Loads a serialized Dict Map from a stream of bytes
     *
     * @param in The in stream where the bytes are coming from
     */
    void load(std::istream &in)
    {
      size_t map_size;

      sdsl::read_member(map_size, in);
      id_map = std::vector<EmptyOrPFC2>(map_size);
      root = new node2();
      root->load(in, id_map);
      sdsl::read_member(free_ids_size, in);
      sdsl::read_member(first_empty, in);
      uint64_t tmp = 0;
      last_empty = first_empty;
      if (free_ids_size > 0)
      {
        for (uint64_t i = 0; i < free_ids_size - 1; i++)
        {
          in.read((char *)&tmp, sizeof(uint64_t));
          id_map[last_empty - 1].next_empty = tmp;
          last_empty = tmp;
        }
      }
    }

    /**
     * @brief Inserts a value into the structure.
     * Assigns an ID and inserts it into the tree.
     *
     * @param val value being inserted
     *
     * @return The ID assigned to the inserted value
     */
    uint64_t insert(const std::string &val)
    {
      uint64_t id;
      if (free_ids_size == 0)
      {
        id = id_map.size() + 1;
        EmptyOrPFC2 data;
        data.pfc = root->insert(val, id, id_map);
        id_map.push_back(data);
        //id_map.push_back({ .pfc = root->insert(val, id, id_map)});
      }
      else
      {
        id = first_empty;
        // Last element on "Symbolic queue"
        if (free_ids_size == 1)
        {
          last_empty = 0;
          first_empty = 0;
        } else {
          first_empty = id_map[id - 1].next_empty;
        }
        id_map[id - 1].pfc = root->insert(val, id, id_map);
        free_ids_size--;
      }

      return id;
    }

    /**
     * @brief Get the ID associated to the value or insert it if its not in the mapping
     *
     * @param val value being searched or inserted
     * @return uint64_t the id assigned to the value
     */
    uint64_t get_or_insert(const std::string &val)
    {
      uint64_t id, found_id;
      std::tuple<uint64_t, PFC2 *> res;
      if (free_ids_size == 0)
      {
        id = id_map.size() + 1;
        res = root->get_or_insert(val, id, id_map);
        found_id = std::get<0>(res);
        if (found_id == id)
        {
          EmptyOrPFC2 data;
          data.pfc = std::get<1>(res);
          id_map.push_back(data);
        }
      }
      else
      {
        id = first_empty;
        res = root->get_or_insert(val, id, id_map);
        found_id = std::get<0>(res);
        if (found_id == id)
        {
          // Last element on "Symbolic queue"
          if (free_ids_size == 1)
          {
            last_empty = 0;
            first_empty = 0;
          } else {
            first_empty = id_map[id - 1].next_empty;
          }
          id_map[id - 1].pfc = std::get<1>(res);
          free_ids_size--;
        }
      }

      return found_id;
    }

    cltj::spo_triple get_or_insert_triple(const std::string &s, const std::string &p, const std::string &o)
    {
      return {this->get_or_insert(s), this->get_or_insert(p), this->get_or_insert(o)};
    }

    /**
     * @brief Deletes a value from the mapping
     *
     * @param val value being eliminated
     *
     * @return The ID of the eliminated value
     */
    uint64_t eliminate(const std::string &val)
    {
      uint64_t elim_id = std::get<0>(root->eliminate(val, id_map));
      // First in "Symbolic queue"
      if (free_ids_size == 0)
      {
        first_empty = elim_id;
        id_map[elim_id - 1].pfc = nullptr;
      }
      else
      {
        id_map[last_empty - 1].next_empty = elim_id;
      }
      last_empty = elim_id;
      free_ids_size++;
      return elim_id;
    }

    /**
     * @brief Deletes a value from the mapping
     *
     * @param id ID of the value being eliminated
     */
    void eliminate(const uint64_t id)
    {
      id_map[id - 1].pfc->elim(id);
      id_map[id - 1].pfc = nullptr;
      // First in "Symbolic queue"
      if (free_ids_size == 0)
      {
        first_empty = id;
      }
      else
      {
        id_map[last_empty - 1].next_empty = id;
      }
      last_empty = id;
      free_ids_size++;
    }

    /**
     * @brief Search a value in the binary tree of the structure and get its ID
     *
     * @param val value being searched
     * @return uint64_t ID associated to the value
     */
    uint64_t locate(const std::string &val)
    {
      return root->search(val);
    }

    /**
     * @brief Search for an ID in the structure and get its corresponding value
     *
     * @param id the ID being searched
     * @return std::string value associated to the ID
     */
    std::string extract(uint64_t id)
    {
      assert(id > 0 && id - 1 < id_map.size());
      std::string res =  id_map[id - 1].pfc->extract(id);
      cache.push_back(res);
      return res;
    }

    size_t size()
    {
      return id_map.size();
    }

    size_t bit_size() const
    {
      size_t id_size = 8 * sizeof(id_map) + 8 * id_map.size() * sizeof(EmptyOrPFC2);
      return 8 * sizeof(root) + id_size + root->bit_size();
    }

    std::string root_value()
    {
      return root->get_value();
    }

    PFC2 *get_root_pfc()
    {
      return root->get_pfc();
    }


  };

  /**
   * @brief Class representing the node of the binary tree
   * in the dictionary mapping.
   *
   * @tparam MINSIZE The minimum words a PFC can have before being fused with its sibling
   * @tparam MAXSIZE The maximum words a PFC can have before splitting
   */
  template <uint64_t MINSIZE, uint64_t MAXSIZE>
  class dict_map2<MINSIZE, MAXSIZE>::node2
  {
  public:
    node2()
    {
      _is_leaf = true;
      pfc = new PFC2();
    }

    node2(std::string& val, uint64_t id)
    {
      _is_leaf = true;
      pfc = new PFC2();
      pfc->insert(val, id);
    }

    node2(PFC2 *p)
    {
      _is_leaf = true;
      pfc = p;
    }

    node2(node2* l, node2* r) {
      _is_leaf = false;
      left = l;
      right = r;
    }

    void free_mem()
    {
      if (_is_leaf)
      {
        delete pfc;
      }
      else
      {
        if(left != NULL) {
          left->free_mem();
          delete left;
        }
        if(right != NULL) {
          right->free_mem();
          delete right;
        }

      }
    }

    node2* clone(std::unordered_map<PFC2*, PFC2*> &ht)
    {
      node2* n;
      if (_is_leaf)
      {
        n = new node2();
        n->_is_leaf = _is_leaf;
        n->pfc = new PFC2(pfc);
        ht.insert({pfc, n->pfc});
      }
      else
      {
        node2* l = left->clone(ht);
        node2* r = right->clone(ht);
        n = new node2(l, r);
      }
      return n;
    }

    bool is_leaf() { return _is_leaf; }

    PFC2 *get_pfc()
    {
      return pfc;
    }

    size_t bit_size() const
    {
      size_t bs = 8 * sizeof(left) * 2;
      bs += 8 * sizeof(pfc);
      bs += 8 * sizeof(bool);
      if (_is_leaf)
      {
        return bs + pfc->bit_size();
      }
      else
      {
        return bs + left->bit_size() + right->bit_size();
      }
    }

    std::string get_value()
    {
      return pfc->first_word();
    }

    /**
     * @brief Function that serializes the node data structure.
     *
     * @param out The out stream where the bytes are being written
     * @return uint64_t The amount of bytes written
     */
    uint64_t serialize(std::ostream &out)
    {
      uint64_t w_bytes = 0;

      out.write((char *)&_is_leaf, sizeof(_is_leaf));
      w_bytes += sizeof(_is_leaf);

      if (_is_leaf)
      {
        w_bytes += pfc->serialize(out);
      }
      else
      {
        w_bytes += left->serialize(out);
        w_bytes += right->serialize(out);
      }

      return w_bytes;
    }

    /**
     * @brief Loads a stream of bytes into a node. It also points
     * every ID being stored in the leaf to that same leaf.
     *
     * @param in The in stream where the bytes are coming from
     * @param id_map Reference to the vector that maps every ID to its corresponding PFC
     * @return PFC* The pointer to the leftmost leaf in the node subtree
     */
    PFC2 *load(std::istream &in, std::vector<EmptyOrPFC2> &id_map)
    {
      size_t string_size;
      in.read((char *)&_is_leaf, sizeof(_is_leaf));

      if (_is_leaf)
      {
        pfc = new PFC2();
        pfc->load(in, id_map);
        return pfc;
      }
      else
      {
        left = new node2();
        PFC2 *left_pfc = left->load(in, id_map);
        right = new node2();
        pfc = right->load(in, id_map);
        return left_pfc;
      }
    }

    /**
     * @brief Inserts a value into the tree on its corresponding Plain Front Coding
     * It assumes that the value being inserted is not part of the dictionary
     *
     * @param val value being inserted
     * @param id ID assigned to that value
     */
    PFC2 *insert(const std::string &val, const uint64_t &id, std::vector<EmptyOrPFC2> &id_map)
    {
      if (is_leaf())
      {
        // Insert in PFC
        pfc->insert(val, id);
        if (pfc->size() > MAXSIZE)
        {
          // Split PFC
          std::tuple<std::string, uint64_t> res = pfc->split();
          PFC2 *new_pfc = new PFC2(std::get<0>(res), std::get<1>(res));
          _is_leaf = false;
          right = new node2(new_pfc);
          left = new node2(pfc);
          pfc = new_pfc;

          // Update ID mapping
          for (uint64_t id : pfc->all_ids())
          {
            id_map[id - 1].pfc = pfc;
          }

          if (val.compare(pfc->first_word()) >= 0)
            return right->get_pfc();
          else
            return left->get_pfc();
        }
        return pfc;
      }
      else
      {
        // Go to correct children
        if (val.compare(pfc->first_word()) > 0)
        {
          return right->insert(val, id, id_map);
        }
        else
        {
          return left->insert(val, id, id_map);
        }
      }
    }

    /**
     * @brief Get the ID associated to the value or insert it if its not in the mapping
     *
     * @param val value being searched or inserted
     * @param id  ID assigned to the value if its inserted
     * @return std::tuple<uint64_t, PFC *> a pair containing the ID of the value and the PFC it was found/inserted
     */
    std::tuple<uint64_t, PFC2 *> get_or_insert(const std::string &val, const uint64_t &id, std::vector<EmptyOrPFC2> &id_map)
    {
      if (is_leaf())
      {
        // Search or insert in PFC
        uint64_t new_id = pfc->get_or_insert(val, id);
        if (pfc->size() > MAXSIZE)
        {
          // Split PFC
          std::tuple<std::string, uint64_t> res = pfc->split();
          PFC2 *new_pfc = new PFC2(std::get<0>(res), std::get<1>(res));
          _is_leaf = false;
          right = new node2(new_pfc);
          left = new node2(pfc);
          pfc = new_pfc;

          // Update ID mapping
          for (uint64_t id : pfc->all_ids())
          {
            id_map[id - 1].pfc = pfc;
          }

          if (val.compare(pfc->first_word()) >= 0)
            return {new_id, right->get_pfc()};
          else
            return {new_id, left->get_pfc()};
        }
        return {new_id, pfc};
      }
      else
      {
        // Go to correct children
        int r = val.compare(pfc->first_word());
        if (r == 0)
        {
          uint64_t new_id = pfc->get_or_insert(val, id);
          return {new_id, pfc};
        }
        else if (r > 0)
        {
          return right->get_or_insert(val, id, id_map);
        }
        else
        {
          return left->get_or_insert(val, id, id_map);
        }
      }
    }

    /**
     * @brief Find and delete the value given
     *
     * @param val value being deleted
     * @return std::tuple<uint64_t, uint64_t> a pair containing
     * the ID of the deleted value and the resulting size of the PFC it was stored in
     */
    std::tuple<uint64_t, uint64_t> eliminate(const std::string &val, std::vector<EmptyOrPFC2> &id_map)
    {
      if (is_leaf())
      {
        // Delete word in PFC
        return {pfc->elim(val), pfc->size()};
      }
      else
      {
        int r = val.compare(pfc->first_word());
        std::tuple<uint64_t, uint64_t> res;
        uint64_t child_size = MAXSIZE;
        // Go to correct children
        if (r == 0)
        {
          // Go to PFC
          res = right->eliminate(val, id_map);
          child_size = std::get<1>(res);
        }
        else if (r < 0)
        {
          res = left->eliminate(val, id_map);
          child_size = std::get<1>(res);
        }
        else
        {
          res = right->eliminate(val, id_map);
          child_size = std::get<1>(res);
        }

        if (child_size < MINSIZE)
        {
          // Update ID mapping
          for (uint64_t id : right->pfc->all_ids())
          {
            id_map[id - 1].pfc = left->pfc;
          }

          std::string right_string = right->pfc->pfc_string();

          left->pfc->fuse(right_string, right->pfc->size());
          right->free_mem();
          delete right;
          _is_leaf = true;
          pfc = left->pfc;
          left->pfc = nullptr;
          delete left;

          if (pfc->size() > MAXSIZE)
          {
            // Split PFC
            std::tuple<std::string, uint64_t> split_res = pfc->split();
            PFC2 *new_pfc = new PFC2(std::get<0>(split_res), std::get<1>(split_res));
            _is_leaf = false;
            right = new node2(new_pfc);
            left = new node2(pfc);
            pfc = new_pfc;

            // Update ID mapping
            for (uint64_t id : pfc->all_ids())
            {
              id_map[id - 1].pfc = pfc;
            }
          }
        }

        return {std::get<0>(res), MAXSIZE};
      }
    }

    /**
     * @brief Search a value in the Binary Tree
     *
     * @param val value being searched
     * @return uint64_t ID associated to the value given
     */
    uint64_t search(const std::string &val)
    {
      if (is_leaf())
      {
        return pfc->locate(val);
      }
      else
      {
        int r = val.compare(pfc->first_word());
        if (r == 0)
        {
          return pfc->locate(val);
        }
        else if (r < 0)
        {
          return left->search(val);
        }
        else
        {
          return right->search(val);
        }
      }
    }

  private:
    bool _is_leaf = false;
    node2 *left = NULL;
    node2 *right = NULL;
    PFC2 *pfc = NULL;
  };

  typedef dict_map2<32, 128> basic_map2;
  typedef dict_map2<2, 4> tiny_map2;

}

#endif