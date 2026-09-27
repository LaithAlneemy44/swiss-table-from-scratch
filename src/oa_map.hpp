#pragma once
#include <vector>
#include <cstddef>
#include <utility>
#include <cstdint>
#include "oa_iterator.hpp"

using namespace std;

template<typename K, typename V>
class OA_Map {
public:
    OA_Map(size_t capacity, const K& sen_empty, const K& sen_deleted);
    void insert(const K& key, const V& value);
    void erase(const K& key);
    bool empty() const;
    size_t size() const;
    V& operator[](const K& key);
    OA_Iterator<K, V> begin();
    OA_Iterator<K, V> end();
    OA_Iterator<K, V> find(const K& key);

private:
    uint64_t hash(const K& key);
    void rehash();
    K sen_empty;
    K sen_deleted;

private:
    vector<pair<K, V>> slots;
    size_t m_size = 0;
    size_t capacity;
};

#include "oa_map.tpp"