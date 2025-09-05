/*
 * pfc.hpp
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

#ifndef TREE_PF2_H
#define TREE_PFC2_H

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <tuple>

namespace dict
{
  class PFC2;

  /*
  * Union type to represent a pointer to a PFC
  * or the next ID that its free in the array.
  */
  union EmptyOrPFC2
  {
      PFC2* pfc;
      uint64_t next_empty;
  };

  /**
   * @brief Class implementing a Plain Front Coding bucket
   * It accepts IDs of 64 bits except for 0
   * The 0 char is a reserved symbol for this representation
   * The structure is:
   *  ID1 String1 ID2 LCP String2 ID3 LCP String3 ...
   */
  class PFC2
  {

  public:
    PFC2() : text_string(""), current_size(0) {}

    PFC2(std::string &initial_string, uint64_t initial_size) : text_string(initial_string), current_size(initial_size) {}

    explicit PFC2(PFC2* o) {
      text_string = o->text_string;
      current_size = o->current_size;
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
      out.write((char *)&current_size, sizeof(current_size));
      w_bytes += sizeof(current_size);
      size_t string_size = text_string.size();
      out.write((char *)&string_size, sizeof(string_size));
      w_bytes += sizeof(string_size);
      out.write((char *)&(text_string[0]), string_size);
      w_bytes += string_size;
      return w_bytes;
    }

    /**
     * @brief Loads a serialized PFC from a stream of bytes
     *
     * @param in The in stream where the bytes are coming from
     */
    void load(std::istream &in)
    {
      size_t string_size;
      in.read((char *)&current_size, sizeof(current_size));
      in.read((char *)&string_size, sizeof(string_size));
      text_string.resize(string_size);
      in.read((char *)&(text_string[0]), string_size);
    }

    /**
     * @brief Loads a PFC from serialized bytes and points every ID
     * stored in the data structure to itself
     *
     * @param in The in stream where the bytes are coming from
     * @param id_map A vector of PFC pointers to map the IDs
     */
    void load(std::istream &in, std::vector<EmptyOrPFC2> &id_map)
    {
      size_t string_size;
      in.read((char *)&current_size, sizeof(current_size));
      in.read((char *)&string_size, sizeof(string_size));
      text_string.resize(string_size);
      in.read((char *)&(text_string[0]), string_size);

      uint64_t curr_id = 0;
      uint64_t index = 0;

      // Get first word
      curr_id = decode_number(index);
      index = text_string.find_first_of('\0', index) + 1;
      id_map[curr_id - 1].pfc = this;

      // Go through the PFC loading the ID map
      while (index < text_string.size())
      {
        curr_id = decode_number(index);
        decode_number(index);
        index = text_string.find_first_of('\0', index) + 1;
        id_map[curr_id - 1].pfc = this;
      }
    }

    /**
     * @brief Insert a new word with its corresponding ID
     * Preserves the order in the PFC
     *
     * @param s the string being inserted
     * @param id the id corresponding to the string
     */
    void insert(const std::string &s, uint64_t id)
    {
      uint64_t index = 0, curr_index = 0;
      std::string prev = "\0", curr = "\0";
      uint64_t curr_id = 0;

      // Inserting on an empty PFC
      if (text_string.size() == 0)
      {
        text_string += encode_number(id) + s + '\0';
        current_size++;
        return;
      }

      curr_id = decode_number(index);
      curr = read_string(index);

      while (index < text_string.size() && s.compare(curr) > 0)
      {
        prev = curr;
        curr_index = index;
        curr_id = decode_number(index);
        uint64_t lcp = decode_number(index);
        read_string(index, curr, prev, lcp);
      }

      int comp = s.compare(curr);

      // Different cases of insertion
      if (prev == "\0" && comp < 0)
      {
        // Insert at the front of the PFC
        std::string r = encode_number(id);
        r += s + '\0';
        uint64_t lcp = longest_common_prefix(s, curr, std::min(s.size(), curr.size()));
        text_string.erase(0, index);
        std::string p = encode_number(curr_id) + encode_number(lcp) + curr.substr(lcp) + '\0';
        text_string.insert(0, r + p);
      }
      else if (index >= text_string.size() && comp > 0)
      {
        // Insert at the end of the PFC
        uint64_t lcp = longest_common_prefix(s, curr, std::min(s.size(), curr.size()));
        text_string += encode_number(id) + encode_number(lcp) + s.substr(lcp) + '\0';
      }
      else
      {
        // Insert and update the next one
        uint64_t lcp_prev = longest_common_prefix(prev, s, std::min(s.size(), prev.size()));
        uint64_t lcp_after = longest_common_prefix(s, curr, std::min(s.size(), curr.size()));
        text_string.erase(curr_index, index - curr_index);
        std::string r = encode_number(id) + encode_number(lcp_prev) + s.substr(lcp_prev) + '\0';
        std::string p = encode_number(curr_id) + encode_number(lcp_after) + curr.substr(lcp_after) + '\0';
        text_string.insert(curr_index, r + p);
      }

      current_size++;
    }

    /**
     * @brief Appends a new word with its corresponding ID
     * PRE: Inserts in the correct position, preserving the order in the PFC
     *
     * @param s the string being inserted
     * @param id the id corresponding to the string
     *
     */
    void append(const std::string &s, uint64_t id, std::string &prev)
    {
      // Inserting on an empty PFC
      if (text_string.empty())
      {
        text_string += encode_number(id) + s + '\0';
        current_size++;
        return;
      }

      uint64_t lcp = longest_common_prefix(s, prev, std::min(s.size(), prev.size()));
      text_string += encode_number(id) + encode_number(lcp) + s.substr(lcp) + '\0';
      current_size++;
    }

    /**
     * @brief Get the ID of the given string
     * If its not found then insert it with the given ID
     *
     * @param s the string being searched or inserted
     * @param id the ID assigned to the string if its not in the PFC
     * @return uint64_t The ID of the string
     */
    uint64_t get_or_insert(const std::string &s, uint64_t id)
    {
      uint64_t index = 0, curr_index = 0;
      std::string prev = "\0", curr = "\0";
      uint64_t curr_id = 0;

      // Inserting on an empty PFC
      if (text_string.size() == 0)
      {
        text_string += encode_number(id) + s + '\0';
        current_size++;
        return id;
      }

      curr_id = decode_number(index);
      read_string(index, curr);

      while (index < text_string.size() && s.compare(curr) > 0)
      {
        prev = curr;
        curr_index = index;
        curr_id = decode_number(index);
        uint64_t lcp = decode_number(index);
        read_string(index, curr, prev, lcp);
      }

      int comp = s.compare(curr);

      // Found the string
      if (comp == 0)
      {
        return curr_id;
      }

      // If not found then insert
      if (prev == "\0" && comp < 0)
      {
        // Insert at the front of the PFC
        std::string r = encode_number(id);
        r += s + '\0';
        uint64_t lcp = longest_common_prefix(s, curr, std::min(s.size(), curr.size()));
        text_string.erase(0, index);
        std::string p = encode_number(curr_id) + encode_number(lcp) + curr.substr(lcp) + '\0';
        text_string.insert(0, r + p);
      }
      else if (index >= text_string.size() && comp > 0)
      {
        // Insert at the end of the PFC
        uint64_t lcp = longest_common_prefix(s, curr, std::min(s.size(), curr.size()));
        text_string += encode_number(id) + encode_number(lcp) + s.substr(lcp) + '\0';
      }
      else
      {
        // Insert and update the next one
        uint64_t lcp_prev = longest_common_prefix(prev, s, std::min(s.size(), prev.size()));
        uint64_t lcp_after = longest_common_prefix(s, curr, std::min(s.size(), curr.size()));
        text_string.erase(curr_index, index - curr_index);
        std::string r = encode_number(id) + encode_number(lcp_prev) + s.substr(lcp_prev) + '\0';
        std::string p = encode_number(curr_id) + encode_number(lcp_after) + curr.substr(lcp_after) + '\0';
        text_string.insert(curr_index, r + p);
      }

      current_size++;
      return id;
    }

    /**
     * @brief Search the string in the PFC and return its ID
     * If its not found it throws an invalid_argument error
     *
     * @param s The string bieng searched
     * @return bool, uint64_t A flag that indicates if s exists and its ID, in that case
     */
    uint64_t locate(const std::string &s)
    {
      uint64_t index = 0;
      std::string prev, curr;
      uint64_t curr_id;

      // Linear search
      curr_id = decode_number(index);
      read_string(index, curr);
      prev = curr;
      while (index < text_string.size())
      {
        if (curr == s)
        {
          return curr_id;
        }
        curr_id = decode_number(index);
        uint64_t lcp = decode_number(index);
        read_string(index, curr, prev, lcp);
        prev = curr;
      }

      if (curr == s)
      {
        return curr_id;
      }

      return 0;
    }

    /**
     * @brief Search a string with the given ID
     * If its not found it throws an invalid_argument error
     *
     * @param i The ID of the string
     * @return std::string The found string corresponding to the ID
     */
    std::string extract(uint64_t i)
    {
      uint64_t index = 0;
      std::string prev, curr;
      uint64_t curr_id;

      // Linear search
      curr_id = decode_number(index);
      read_string(index, curr);
      prev = curr;
      while (index < text_string.size())
      {
        if (curr_id == i)
        {
          return curr;
        }
        curr_id = decode_number(index);
        uint64_t lcp = decode_number(index);
        read_string(index, curr, prev, lcp);
        prev = curr;
      }

      if (curr_id == i)
      {
        return curr;
      }

      throw std::invalid_argument("ID is not asociated to any string in Plain Front Coding");
    }

    /**
     * @brief Delete a string from the PFC
     *
     * @param s The string being deleted
     * @return uint64_t The ID of the string that was deleted
     */
    uint64_t elim(const std::string &s)
    {
      uint64_t index = 0, curr_index = 0;
      std::string prev = "\0", curr = "\0";
      uint64_t curr_id = 0, curr_lcp = 0;

      curr_id = decode_number(index);
      curr = read_string(index);

      while (index < text_string.size() && s.compare(curr) != 0)
      {
        prev = curr;
        curr_index = index;
        curr_id = decode_number(index);
        curr_lcp = decode_number(index);
        read_string(index, curr, prev, curr_lcp);
      }

      if (index >= text_string.size() && s.compare(curr) != 0)
      {
        throw std::invalid_argument(s + " not in PFC");
      }

      if (index >= text_string.size())
      {
        // Delete at the end
        text_string.erase(curr_index, text_string.size());
      }
      else if (prev == "\0")
      {
        // Delete at the front
        uint64_t next_id = decode_number(index);
        uint64_t lcp = decode_number(index);
        std::string next = encode_number(next_id) + curr.substr(0, lcp) + read_string(index) + '\0';
        text_string.erase(0, index);
        text_string.insert(0, next);
      }
      else
      {
        // Delete and update the next one
        uint64_t next_id = decode_number(index);
        uint64_t lcp_next = decode_number(index);
        std::string next;
        read_string(index, next, curr, lcp_next);
        uint64_t lcp_with_prev = std::min(curr_lcp, lcp_next);
        std::string p = encode_number(next_id) + encode_number(lcp_with_prev) + next.substr(lcp_with_prev) + '\0';
        text_string.erase(curr_index, index - curr_index);
        text_string.insert(curr_index, p);
      }

      current_size--;
      return curr_id;
    }

    /**
     * @brief Delete a string from the PFC. Search it by ID.
     *
     * @param id The ID of the string being deleted
     */
    void elim(const uint64_t &id)
    {
      uint64_t index = 0, curr_index = 0;
      std::string prev = "\0", curr = "\0";
      uint64_t curr_id = 0;

      curr_id = decode_number(index);
      read_string(index, curr);

      while (index < text_string.size() && curr_id != id)
      {
        prev = curr;
        curr_index = index;
        curr_id = decode_number(index);
        uint64_t lcp = decode_number(index);
        read_string(index, curr, prev, lcp);
      }

      if (index >= text_string.size() && curr_id != id)
      {
        throw std::invalid_argument(id + " not in PFC");
      }

      if (index >= text_string.size())
      {
        // Delete at the end
        text_string.erase(curr_index, text_string.size());
      }
      else if (prev == "\0")
      {
        // Delete at the front
        uint64_t next_id = decode_number(index);
        uint64_t lcp = decode_number(index);
        std::string next = encode_number(next_id) + curr.substr(0, lcp) + read_string(index) + '\0';
        text_string.erase(0, index);
        text_string.insert(0, next);
      }
      else
      {
        // Delete and update the next one
        uint64_t next_id = decode_number(index);
        uint64_t lcp = decode_number(index);
        std::string next;
        read_string(index, next, curr, lcp);
        uint64_t lcp_with_prev = longest_common_prefix(prev, next, std::min(prev.size(), next.size()));
        std::string p = encode_number(next_id) + encode_number(lcp_with_prev) + next.substr(lcp_with_prev) + '\0';
        text_string.erase(curr_index, index - curr_index);
        text_string.insert(curr_index, p);
      }

      current_size--;
    }

    /**
     * @brief Splits the PFC into two PFCs. The first half corresponds
     * to itself and the second half to a new PFC.
     *
     * @return std::tuple<std::string, uint64_t, std::string> A triple containing:
     *  (The string corresponding to the second half, The size of the second half, The first word of the second half)
     */
    std::tuple<std::string, uint64_t> split()
    {
      uint64_t index = 0, counter = 1;
      uint64_t middle = current_size / 2;
      std::string prev = "\0", curr = "\0";

      decode_number(index);
      read_string(index, curr);

      // Get to the middle of the PFC
      while (counter < middle)
      {
        prev = curr;
        decode_number(index);
        uint64_t lcp = decode_number(index);
        read_string(index, curr, prev, lcp);
        counter++;
      }

      std::string second_half = text_string.substr(index);
      text_string.erase(index, text_string.size());
      index = 0;
      uint64_t first_id = decode_number(second_half, index);
      uint64_t decode_id = decode_number(second_half, index);
      std::string first_word;
      read_string(second_half, index, first_word, curr, decode_id);
      second_half.erase(0, index);
      second_half.insert(0, encode_number(first_id) + first_word + '\0');
      std::tuple<std::string, uint64_t> response = std::make_tuple(second_half, current_size - middle);

      current_size = middle;
      text_string.shrink_to_fit();

      return response;
    }

    /**
     * @brief Fuses this PFC into another.
     * This function assumes that this instance of the PFC is the smaller one
     * and the given half is the bigger one.
     *
     * @param new_half The string of the othe PFC
     * @param new_words_counter The amount of words stored in the given PFC
     */
    void fuse(std::string &new_half, uint64_t new_words_counter)
    {
      uint64_t index = 0;
      std::string prev = "\0", curr = "\0";

      decode_number(index);
      read_string(index, curr);

      // Find the last string
      while (index < text_string.size())
      {
        prev = curr;
        decode_number(index);
        uint64_t lcp = decode_number(index);
        read_string(index, curr, prev, lcp);
      }

      index = 0;
      uint64_t new_id = decode_number(new_half, index);
      std::string first_word = read_string(new_half, index);
      uint64_t lcp = longest_common_prefix(curr, first_word, std::min(curr.size(), first_word.size()));
      new_half.erase(0, index);
      new_half.insert(0, encode_number(new_id) + encode_number(lcp) + first_word.substr(lcp) + '\0');
      text_string += new_half;
      current_size += new_words_counter;
    }

    /**
     * @brief Gets all the IDs stored in the PFC
     *
     * @return std::vector<uint64_t> Vector with all the IDS stored in the PFC
     */
    std::vector<uint64_t> all_ids()
    {
      uint64_t index = 0;
      std::vector<uint64_t> ids;

      ids.push_back(decode_number(index));
      index = text_string.find_first_of('\0', index) + 1;

      while (index < text_string.size())
      {
        ids.push_back(decode_number(index));
        decode_number(index);
        index = text_string.find_first_of('\0', index) + 1;
      }

      return ids;
    }

    std::string first_word()
    {
      uint64_t index = 0;
      decode_number(index);
      return read_string(index);
    }

    uint64_t size()
    {
      return current_size;
    }

    size_t bit_size()
    {
      return text_string.capacity() * 8 + sizeof(current_size) * 8;
    }

    std::string pfc_string()
    {
      return text_string;
    }

  private:
    uint64_t current_size;   // Amount of words stored in the PFC
    std::string text_string; // The actual bytes of the PFC

    /**
     * @brief Reads the string at position *index in the PFC
     * Moves the index to the end of the encoded string
     *
     * @param index pointer of the index being used
     * @return std::string The read string
     */
    std::string read_string(uint64_t &index)
    {
      uint64_t new_index = text_string.find_first_of('\0', index);
      std::string current = text_string.substr(index, new_index - index);
      index = new_index + 1;
      return current;
    }

    /**
     * @brief Reads the string at position *index in the PFC
     * and stores the result in the container
     * Moves the index to the end of the encoded string
     *
     * @param index pointer of the index being used
     * @param container string where the result is stored
     */
    void read_string(uint64_t &index, std::string &container)
    {
      uint64_t new_index = text_string.find_first_of('\0', index);
      container = text_string.substr(index, new_index - index);
      index = new_index + 1;
    }

    /**
     * @brief Reads the string at position *index in the PFC
     * and constructs the entirety of the string to store it
     * in the container
     * Moves the index to the end of the encoded string
     *
     * @param index pointer of the index being used
     * @param container string where the result is stored
     * @param prev the previous string
     * @param lcp the least common prefix between prev and container
     */
    void read_string(uint64_t &index, std::string &container, const std::string &prev, const uint64_t &lcp)
    {
      uint64_t new_index = text_string.find_first_of('\0', index);
      container = prev.substr(0, lcp) + text_string.substr(index, new_index - index);
      index = new_index + 1;
    }

    /**
     * @brief Reads the string at position *index in the target string of bytes
     * Moves the index to the end of the encoded string
     *
     * @param target the string of bytes being read
     * @param index pointer of the index being used
     * @return std::string the read string
     */
    std::string read_string(const std::string &target, uint64_t &index)
    {
      uint64_t new_index = target.find_first_of('\0', index);
      std::string current = target.substr(index, new_index - index);
      index = new_index + 1;
      return current;
    }

    /**
     * @brief Reads the string at position *index in the target string of bytes
     * and constructs the entirety of the string to store it
     * in the container
     * Moves the index to the end of the encoded string
     *
     * @param target the string of bytes being read
     * @param index pointer of the index being used
     * @param container string where the result is stored
     * @param prev the previous string
     * @param lcp the least common prefix between prev and container
     */
    void read_string(const std::string &target, uint64_t &index, std::string &container, const std::string &prev, const uint64_t &lcp)
    {
      uint64_t new_index = target.find_first_of('\0', index);
      container = prev.substr(0, lcp) + target.substr(index, new_index - index);
      index = new_index + 1;
    }

    /**
     * @brief Encodes a number in the PFC
     *
     * @param n the number being encoded
     * @return std::string the encoded number
     */
    std::string encode_number(uint64_t n)
    {
      std::string s = "";

      // Encode the first 7 bits of the number until its size is 7 bits
      while (n > 127)
      {
        s += (u_char)(n & 127);
        n >>= 7;
      }

      // The last 7 bits encode it with a 1 in front
      s += (u_char)(n | 0x80);

      return s;
    }

    /**
     * @brief Decodes the number at position *index of the PFC
     * Moves the index to the end of the encoded number
     *
     * @param index The pointer to the index being used
     * @return uint64_t the decoded number
     */
    uint64_t decode_number(uint64_t &index)
    {
      uint64_t n = 0;
      uint64_t shift = 0;

      while (!(text_string[index] & 0x80))
      {
        n |= (text_string[index] & 127) << shift;
        index++;
        shift += 7;
      }

      n |= (text_string[index] & 127) << shift;
      index++;

      return n;
    }

    /**
     * @brief Decodes the number at position *index of the s string
     * Moves the index to the end of the encoded number
     *
     * @param s the target string being read
     * @param index pointer to the index being used
     * @return uint64_t the decoded number
     */
    uint64_t decode_number(const std::string &s, uint64_t &index)
    {
      uint64_t n = 0;
      uint64_t shift = 0;

      while (!(s[index] & 0x80))
      {
        n |= (s[index] & 127) << shift;
        index++;
        shift += 7;
      }

      n |= (s[index] & 127) << shift;
      index++;

      return n;
    }

    /**
     * @brief Calculates the length of the longest common prefix between two strings
     *
     * @param s1 string being compared
     * @param s2 string being compared
     * @param max_length the maximum length between the size of both strings
     * @return uint64_t the length of the longest common prefix
     */
    uint64_t longest_common_prefix(const std::string &s1, const std::string &s2, size_t max_length)
    {
      uint64_t index = 0;
      while (index < max_length && (s1[index] == s2[index]))
        index++;
      return index;
    }
  };
}

#endif