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
    OA_Map();
    explicit OA_Map(size_t capacity);
    void insert(const K& key, const V& value);
    void erase(const K& key);
    void set_empty_sentinel(const V& value);
    void set_deleted_sentinel(const V& value);
    bool empty() const;
    size_t size() const;
    V& operator[](const K& key);
    OA_Iterator<K, V> begin();
    OA_Iterator<K, V> end();
    OA_Iterator<K, V> find(const K& key);

private:
    uint64_t hash(const K& key);
    void rehash();
    V sen_empty;
    V sen_deleted;

private:
    vector<pair<K, V>> slots;
    size_t m_size = 0;
    size_t capacity;
};

#include "oa_map.tpp"