#pragma once
#include <cstddef>
#include <utility>
#include "oa_iterator.hpp"

using namespace std;

template<typename K, typename V>
class OA_Map {
public:
    OA_Map();
    explicit OA_Map(size_t capacity);
    void insert(const K& key, const V& value);
    void erase(const K& key);
    void find(const K& key);
    bool empty();
    size_t size();
    V& operator[](const K& key);
    OA_Iterator<K, V> begin();
    OA_Iterator<K, V> end();

private:
    size_t m_size = 0;
    size_t capacity;
    void rehash();
};

#include "oa_map.tpp"