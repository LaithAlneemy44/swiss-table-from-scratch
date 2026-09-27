#pragma once
#include <vector>
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
    void find(const K& key) const;
    bool empty() const;
    size_t size() const;
    V& operator[](const K& key);
    OA_Iterator<K, V> begin() const;
    OA_Iterator<K, V> end() const ;

private:
    vector<pair<K, V>> slots; 
    size_t m_size = 0;
    size_t capacity;
    void rehash();
};

#include "oa_map.tpp"