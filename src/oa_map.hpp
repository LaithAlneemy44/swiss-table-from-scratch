#pragma once
#include <vector>
#include <cstddef>
#include <utility>
#include <cstdint>
#include <bit>
#include <stdexcept>
#include "hash.hpp"

using namespace std;

template<typename K, typename V>

class OA_Map {
public:
    class Iterator {
    public:
        Iterator(pair<K, V>* slot_ptr, OA_Map* map_ptr) : slot_ptr(slot_ptr), map_ptr(map_ptr){
     
        }

        Iterator& operator++() {
            ++slot_ptr;
            while (*this != map_ptr->end() && (slot_ptr->first == map_ptr->sen_empty || slot_ptr->first == map_ptr->sen_deleted)) {
                ++slot_ptr;
            }

            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++*this;
            return temp;
        }

        pair<K, V>& operator*() const {
            return *slot_ptr;
        }

        pair<K, V>* operator->() const {
            return slot_ptr;
        }

        bool operator==(const Iterator& other) const {
            return slot_ptr == other.slot_ptr;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        pair<K, V>* slot_ptr;
        OA_Map* map_ptr;
    };

public:
    OA_Map(size_t capacity, double max_load_factor, const K& sen_empty, const K& sen_deleted) : 
    capacity{bit_ceil(capacity)}, max_load_factor{max_load_factor}, sen_empty{sen_empty}, sen_deleted{sen_deleted}, slots(this->capacity, {sen_empty, V{}}) {

    }

    Iterator insert(const K& key, const V& value) {
        if (key == sen_empty || key == sen_deleted) {
            throw invalid_argument("Cannot insert sentinel values.");
        }

        if (load_factor() >= max_load_factor) {
            rehash();
        }

        uint64_t hashed = hash(key);
        const size_t mask = capacity - 1;
        const size_t index = hashed & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (slots[i].first == key) {
                return Iterator(&slots[i], this);
            }

            else if (slots[i].first == sen_empty) {
                slots[i] = {key, value};
                ++m_size;
                return Iterator(&slots[i], this);
            }
        }

        unreachable();
    }

    void erase(const K& key) {
        if (key == sen_empty || key == sen_deleted) {
            return;
        }

        uint64_t hashed = hash(key);
        const size_t mask = capacity - 1;
        const size_t index = hashed & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (slots[i].first == key) {
                slots[i].first = sen_deleted;
                return;
            }

            else if (slots[i].first == sen_empty) {
                return;
            }
        }

        unreachable();
    }

    size_t size() const {
        return m_size;
    }

    V& operator[](const K& key) {
        return insert(key, V{})->second;
    }

    Iterator begin() {
        for (size_t i = 0; i < capacity; ++i) {
            if (slots[i].first != sen_empty && slots[i].first != sen_deleted) {
                return Iterator(&slots[i], this);
            }
        }

        return end();
    }

    Iterator end() {
        return Iterator(slots.data() + capacity, this);
    }

    Iterator find(const K& key) {
        if (key == sen_empty || key == sen_deleted) {
            return end();
        }

        uint64_t hashed = hash(key);
        const size_t mask = capacity - 1;
        const size_t index = hashed & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (slots[i].first == key) {
                return Iterator(&slots[i], this);
            }
            
            else if (slots[i].first == sen_empty) {
                return end();
            }
        }

        return end();
    }

private:
    double load_factor() const {
        return static_cast<double>(m_size + 1) / capacity;
    }

    uint64_t hash(const K& key) const {
        SplitMix64(key);
    }

    void rehash() {
        vector<pair<K, V>> temp = move(slots);
        capacity <<= 1;
        slots.assign(capacity, {sen_empty, V{}});
        m_size = 0;
        for (const auto&[key, value] : temp) {
            if (key != sen_empty && key != sen_deleted) {
                insert(key, value);
            }
        }
    }

private:
    size_t m_size = 0;
    size_t capacity;
    const double max_load_factor;
    const K sen_empty;
    const K sen_deleted;
    vector<pair<K, V>> slots;
};