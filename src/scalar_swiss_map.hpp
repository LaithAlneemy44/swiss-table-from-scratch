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

class Scalar_Swiss_Map {
public:
    class Iterator {
    public:
        Iterator(pair<K, V>* slot_ptr, OA_Map* map_ptr) : m_slot_ptr(slot_ptr), m_map_ptr(map_ptr){
     
        }

        Iterator& operator++() {
            ++m_slot_ptr;
            while (*this != m_map_ptr->end() && (m_slot_ptr->first == m_map_ptr->sen_empty || m_slot_ptr->first == m_map_ptr->sen_deleted)) {
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
    Scalar_Swiss_Map(size_t capacity, double max_load_factor) : 
    m_capacity{bit_ceil(capacity)}, m_max_load_factor{max_load_factor}, slots(this->m_capacity), control(this->m_capacity, sen_empty) {

    }

    Iterator insert(const K& key, const V& value) {
        if (load_factor() >= m_max_load_factor) {
            rehash();
        }

        const uint64_t hashed = hash(key);
        const uint64_t h1 = hashed & ((static_cast<uint64_t>(1) << 57) - 1);
        const uint8_t h2 = static_cast<uint8_t>(hashed >> 57);
        const size_t mask = m_capacity - 1;
        const size_t index = h1 & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (control[i] == sen_deleted) {
                continue;
            }

            if (control[i] != sen_empty && control[i] != h2) {
                continue;
            }

            if (control[i] == sen_empty) {
                slots[i] = {key, value};
                control[i] = h2;
                ++m_size;
                return Iterator(&slots[i], this);
            }

            else if (slots[i].first == key) {
                return Iterator(&slots[i], this);
            }
        }

        unreachable();
    }

    void erase(const K& key) {
        const uint64_t hashed = hash(key);
        const uint64_t h1 = hashed & ((static_cast<uint64_t>(1) << 57) - 1);
        const uint8_t h2 = static_cast<uint8_t>(hashed >> 57);
        const size_t mask = m_capacity - 1;
        const size_t index = h1 & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (control[i] == sen_empty) {
                return;
            }

            if (control[i] == h2 && slots[i].first == key) {
                control[i] = sen_deleted;
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
            if (control[i] != sen_empty && control[i] != sen_deleted) {
                return Iterator(&slots[i], this);
            }
        }

        return end();
    }

    Iterator end() {
        return Iterator(slots.data() + m_capacity, this);
    }

    Iterator find(const K& key) {
        const uint64_t hashed = hash(key);
        const uint64_t h1 = hashed & ((static_cast<uint64_t>(1) << 57) - 1);
        const uint8_t h2 = static_cast<uint8_t>(hashed >> 57);
        const size_t mask = m_capacity - 1;
        const size_t index = h1 & mask;
        for (size_t i = index; ; i = (i + 1) & mask) {
            if (control[i] == h2 && slots[i].first == key) {
                return Iterator(&slots[i], this);
            }
            
            else if (control[i] == sen_empty) {
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
        vector<pair<K, V>> temp_slots = move(slots);
        vector<uint8_t> temp_control = move(control);
        m_capacity <<= 1;
        slots.resize(m_capacity);
        control.assign(m_capacity, sen_empty);
        m_size = 0;
        for (size_t i = 0; i < temp_control.size(); ++i) {
            if (temp_control[i] != sen_empty && temp_control[i] != sen_deleted) {
                insert(temp_slots[i].first, temp_slots[i].second);
            }
        }
    }

private:
    size_t m_size = 0;
    size_t m_capacity;
    const double m_max_load_factor;
    static constexpr uint8_t sen_empty = 0b10000000;
    static constexpr uint8_t sen_deleted = 0b11111111;
    vector<pair<K, V>> slots;
    vector<uint8_t> control;
};