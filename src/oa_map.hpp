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
    OA_Map(size_t capacity, const K& sen_empty, const K& sen_deleted) : 
    capacity{capacity}, sen_empty{sen_empty}, sen_deleted{sen_deleted}, slots(capacity, {sen_empty, V{}}) {

    }

    void insert(const K& key, const V& value) {
        uint64_t hashed = hash(key);
        const size_t index = hashed % capacity;
        for (size_t i = index; ; ++i) {
            if (slots[i].first == key) {
                slots[i].second = value;
                return;
            }

            else if (slots[i] == sen_empty) {
                slots[i] = {key, value};
                return;
            }

            else if (i >= capacity - 1) {
                i = -1;
            }
        }
    }

    void erase(const K& key) {
        uint64_t hashed = hash(key);
        const size_t index = hashed % capacity;
        for (size_t i = index; ; ++i) {
            if (slots[i].first == key) {
                slots[i].first = sen_deleted;
                return;
            }

            else if (slots[i].first == sen_empty) {
                return;
            }

            else if (i >= capacity - 1) {
                i = -1;
            }
        }
    }

    bool empty() const {
        return size() == 0;
    }


    size_t size() const {
        return m_size;
    }


    V& operator[](const K& key) {
        auto it = find(key);
        if (it != end()) {
            return it->second;
        }

        uint64_t hashed = hash(key);
        const size_t index = hashed % capacity;
        for (size_t i = 0; ; ++i) {
            if (slots[i].first == sen_empty) {
                return slots[i].second = V{};
            }

            else if (i >= capacity - 1) {
                i = -1;
            }
        }

        return V{};
    }

    OA_Iterator<K, V> begin() {
        for (size_t i = 0; i < capacity; ++i) {
            if (slots[i].first != sen_empty && slots[i].first != sen_deleted) {
                return OA_Iterator(&slots[i]);
            }
        }

        return end();
    }


    OA_Iterator<K, V> end() {
        return OA_Iterator(slots.data() + capacity);
    }

    OA_Iterator<K, V> find(const K& key) {
        uint64_t hashed = hash(key);
        const size_t index = hashed % capacity;
        for (size_t i = index; ; ++i) {
            if (slots[i].first == key) {
                return OA_Iterator(&slots[i]);
            }
            
            else if (slots[i].first == sen_empty) {
                return end();
            }

            else if (i >= capacity - 1) {
                i = -1;
            }
        }

        return end();
    }

private:
    uint64_t hash(const K& key) {

    }

    void rehash() {

    }

private:
    vector<pair<K, V>> slots;
    size_t m_size = 0;
    size_t capacity;
    K sen_empty;
    K sen_deleted;
};