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
        Iterator(pair<K, V>* slot_ptr, OA_Map* map_ptr) : m_slot_ptr(slot_ptr), m_map_ptr(map_ptr) {
     
        }

        Iterator& operator++() {
            ++m_slot_ptr;
            while (*this != m_map_ptr->end() && (m_slot_ptr->first == m_map_ptr->m_sen_empty || m_slot_ptr->first == m_map_ptr->m_sen_deleted)) {
                ++m_slot_ptr;
            }

            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++*this;
            return temp;
        }

        pair<K, V>& operator*() const {
            return *m_slot_ptr;
        }

        pair<K, V>* operator->() const {
            return m_slot_ptr;
        }

        bool operator==(const Iterator& other) const {
            return m_slot_ptr == other.m_slot_ptr;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        pair<K, V>* m_slot_ptr;
        OA_Map* m_map_ptr;
    };

public:
    OA_Map(size_t capacity, double max_load_factor, const K& sen_empty, const K& sen_deleted) : 
    m_capacity{bit_ceil(capacity)}, m_max_load_factor{max_load_factor}, m_sen_empty{sen_empty}, m_sen_deleted{sen_deleted}, slots(this->m_capacity, {sen_empty, V{}}) {

    }

    Iterator insert(const K& key, const V& value) {
        if (key == m_sen_empty || key == m_sen_deleted) {
            throw invalid_argument("Cannot insert sentinel values.");
        }

        if (load_factor() >= m_max_load_factor) {
            rehash();
        }

        const uint64_t hashed = hash(key);
        const size_t mask = m_capacity - 1;
        const size_t index = hashed & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (slots[i].first == key) {
                return Iterator(&slots[i], this);
            }

            else if (slots[i].first == m_sen_empty) {
                slots[i] = {key, value};
                ++m_size;
                return Iterator(&slots[i], this);
            }
        }

        unreachable();
    }

    void erase(const K& key) {
        if (key == m_sen_empty || key == m_sen_deleted) {
            return;
        }

        const uint64_t hashed = hash(key);
        const size_t mask = m_capacity - 1;
        const size_t index = hashed & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (slots[i].first == key) {
                slots[i].first = m_sen_deleted;
                return;
            }

            else if (slots[i].first == m_sen_empty) {
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
        for (size_t i = 0; i < m_capacity; ++i) {
            if (slots[i].first != m_sen_empty && slots[i].first != m_sen_deleted) {
                return Iterator(&slots[i], this);
            }
        }

        return end();
    }

    Iterator end() {
        return Iterator(slots.data() + m_capacity, this);
    }

    Iterator find(const K& key) {
        if (key == m_sen_empty || key == m_sen_deleted) {
            return end();
        }

        const uint64_t hashed = hash(key);
        const size_t mask = m_capacity - 1;
        const size_t index = hashed & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (slots[i].first == key) {
                return Iterator(&slots[i], this);
            }
            
            else if (slots[i].first == m_sen_empty) {
                return end();
            }
        }

        unreachable();
    }

private:
    double load_factor() const {
        return static_cast<double>(m_size + 1) / static_cast<double>(m_capacity);
    }

    uint64_t hash(const K& key) const {
        return SplitMix64<K>{}(key);
    }

    void rehash() {
        vector<pair<K, V>> temp = move(slots);
        m_capacity <<= 1;
        slots.assign(m_capacity, {m_sen_empty, V{}});
        m_size = 0;
        for (const auto&[key, value] : temp) {
            if (key != m_sen_empty && key != m_sen_deleted) {
                insert(key, value);
            }
        }
    }

private:
    size_t m_size = 0;
    size_t m_capacity;
    const double m_max_load_factor;
    const K m_sen_empty;
    const K m_sen_deleted;
    vector<pair<K, V>> slots;
};