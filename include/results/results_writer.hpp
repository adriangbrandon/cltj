//
// Created by Adrián on 24/10/23.
//

#ifndef CLTJ_RESULTS_WRITER_HPP
#define CLTJ_RESULTS_WRITER_HPP


#include <array>
#include <fstream>
#include <vector>
#include <cstdint>

namespace util {

    template<class Var, class Cons>
    class results_writer {

    public:
        typedef std::vector<std::pair<Var, Cons>> value_type;
        typedef uint64_t size_type;

    private:

        std::ofstream m_stream;
        size_type m_elements = 0;
        size_type m_cnt = 0;

        void copy(const results_writer &o) {
            m_stream = o.m_stream;
            m_cnt = o.m_cnt;
            m_elements = o.m_elements;
        }

    public:

        results_writer(std::string &file, size_type elements) {
            m_stream.open(file, std::ios::binary);
            m_elements = elements;
        }

        inline void add(const value_type &val){
            std::vector<Cons> values(m_elements);
            for(const auto &pair : val){
                values[pair.first] = pair.second;
            }
            m_stream.write((char*)values.data(), sizeof(Cons)*m_elements);
            ++m_cnt;
        }


        inline size_type size(){
            return m_cnt;
        }

        inline void close() {
            m_stream.close();
        }

        //! Copy constructor
        results_writer(const results_writer &o) {
            copy(o);
        }

        //! Move constructor
        results_writer(results_writer &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        results_writer &operator=(const results_writer &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        results_writer &operator=(results_writer &&o) {
            if (this != &o) {
                m_stream = std::move(o.m_stream);
                m_elements = std::move(o.m_elements);
                m_cnt = std::move(o.m_cnt);
            }
            return *this;
        }

        void swap(results_writer &o) {
            std::swap(m_stream, o.m_stream);
            std::swap(m_elements, o.m_elements);
            std::swap(m_cnt, o.m_cnt);
        }

    };

}
#endif //CLTJ_RESULTS_WRITER_HPP