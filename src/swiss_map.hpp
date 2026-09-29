#pragma once
#include <algorithm>
#include <vector>
#include <cstddef>
#include <utility>
#include <cstdint>
#include <bit>
#include <stdexcept>
#include <emmintrin.h>
#include "hash.hpp"

using namespace std;

template<typename K, typename V>

class Swiss_Map {
public:
    class Iterator {
    public:
        Iterator(pair<K, V>* slot_ptr, uint8_t* control_ptr, Swiss_Map* map_ptr) : m_slot_ptr(slot_ptr), m_control_ptr(control_ptr), m_map_ptr(map_ptr) {
            
        }

        Iterator& operator++() {
            ++m_slot_ptr;
            ++m_control_ptr;
            const uint8_t* const base_ptr = m_map_ptr->control.data();
            const size_t capacity = m_map_ptr->m_capacity;
            size_t pos = static_cast<size_t>(m_control_ptr - base_ptr);
            while (pos < capacity) {
                const __m128i group = _mm_loadu_si128(reinterpret_cast<const __m128i*>(m_control_ptr));
                const uint16_t mask = static_cast<uint16_t>(~_mm_movemask_epi8(group));
                if (mask) {
                    const uint16_t i = static_cast<uint16_t>(countr_zero(mask));
                    if (pos + i < capacity) {
                        m_slot_ptr += i;
                        m_control_ptr += i;
                        return *this;
                    }

                    else {
                        return (*this) = m_map_ptr->end();
                    }
                }

                if ((pos += 16) >= capacity) {
                    return (*this) = m_map_ptr->end();
                }

                m_slot_ptr += 16;
                m_control_ptr += 16;
            }

            return (*this) = m_map_ptr->end();
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
            return m_control_ptr == other.m_control_ptr;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        pair<K, V>* m_slot_ptr;
        uint8_t* m_control_ptr;
        Swiss_Map* m_map_ptr;
    };

public:
    Swiss_Map(size_t capacity, double max_load_factor) : 
    m_capacity{max(bit_ceil(capacity), size_t{16})}, m_max_load_factor{max_load_factor}, slots(this->m_capacity), control(this->m_capacity + 16, sen_empty) {

    }

    Iterator insert(const K& key, const V& value) {
        if (load_factor() >= m_max_load_factor) {
            rehash();
        }

        const uint64_t hashed = hash(key);
        const uint64_t h1 = hashed & ((static_cast<uint64_t>(1) << 57) - 1);
        const uint8_t h2 = static_cast<uint8_t>(hashed >> 57);
        const __m128i target = _mm_set1_epi8(static_cast<char>(h2));
        const __m128i empty = _mm_set1_epi8(static_cast<char>(sen_empty));
        const size_t mask = m_capacity - 1;
        const size_t index = h1 & mask;
        for (size_t i = index; ; i = (i + 16) & mask) {
            const __m128i group = _mm_loadu_si128(reinterpret_cast<const __m128i*>(control.data() + i));
            uint16_t control_mask = static_cast<uint16_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(group, target)));
            while (control_mask) {
                const size_t j = (i + static_cast<size_t>(countr_zero(control_mask))) & mask;
                control_mask &= control_mask - 1;
                if (slots[j].first == key) {
                    return Iterator(&slots[j], &control[j], this);
                }
            }
            
            const uint16_t empty_mask = static_cast<uint16_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(group, empty)));
            if (empty_mask) { 
                const size_t j = (i + static_cast<size_t>(countr_zero(empty_mask))) & mask;
                ++m_size;
                slots[j] = {key, value};
                set_control(j, h2);
                return Iterator(&slots[j], &control[j], this);
            }
        }

        unreachable();
    }

    void erase(const K& key) {
        const uint64_t hashed = hash(key);
        const uint64_t h1 = hashed & ((static_cast<uint64_t>(1) << 57) - 1);
        const uint8_t h2 = static_cast<uint8_t>(hashed >> 57);
        const __m128i target = _mm_set1_epi8(static_cast<char>(h2));
        const __m128i empty = _mm_set1_epi8(static_cast<char>(sen_empty));
        const size_t mask = m_capacity - 1;
        const size_t index = h1 & mask;
        for (size_t i = index; ; i = (i + 16) & mask) {
            const __m128i group = _mm_loadu_si128(reinterpret_cast<const __m128i*>(control.data() + i));
            uint16_t control_mask = static_cast<uint16_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(group, target)));
            while (control_mask) {
                const size_t j = (i + static_cast<size_t>(countr_zero(control_mask))) & mask;
                control_mask &= control_mask - 1;
                if (slots[j].first == key) {
                    set_control(j, sen_deleted);
                    return;
                }
            }
        
            if (_mm_movemask_epi8(_mm_cmpeq_epi8(group, empty))) { 
                return;
            }
        }

        unreachable();
    }

    size_t size() const {
        return m_size;
    }

    size_t capacity() const {
        return m_capacity;
    }

    V& operator[](const K& key) {
        return insert(key, V{})->second;
    }

    Iterator begin() {
        const size_t mask = m_capacity - 1;
        for (size_t i = 0; i < m_capacity; i += 16) {
            const __m128i group = _mm_loadu_si128(reinterpret_cast<const __m128i*>(control.data() + i));
            const uint16_t control_mask = static_cast<uint16_t>(~_mm_movemask_epi8(group));
            if (control_mask) {
                const size_t j = (i + static_cast<size_t>(countr_zero(control_mask))) & mask;
                return Iterator(&slots[j], &control[j], this);
            }
        }

        return end();
    }

    Iterator end() {
        return Iterator(slots.data() + m_capacity, control.data() + m_capacity, this);
    }

    Iterator find(const K& key) {
        const uint64_t hashed = hash(key);
        const uint64_t h1 = hashed & ((static_cast<uint64_t>(1) << 57) - 1);
        const uint8_t h2 = static_cast<uint8_t>(hashed >> 57);
        const __m128i target = _mm_set1_epi8(static_cast<char>(h2));
        const __m128i empty = _mm_set1_epi8(static_cast<char>(sen_empty));
        const size_t mask = m_capacity - 1;
        const size_t index = h1 & mask;
        for (size_t i = index; ; i = (i + 16) & mask) {
            const __m128i group = _mm_loadu_si128(reinterpret_cast<const __m128i*>(control.data() + i));
            uint16_t control_mask = static_cast<uint16_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(group, target)));
            while (control_mask) {
                const size_t j = (i + static_cast<size_t>(countr_zero(control_mask))) & mask;
                control_mask &= control_mask - 1;
                if (slots[j].first == key) {
                    return Iterator(&slots[j], &control[j], this);
                }
            }
            
            if (_mm_movemask_epi8(_mm_cmpeq_epi8(group, empty)) != 0) { 
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
        const size_t temp_capacity = m_capacity;
        m_capacity <<= 1;
        slots.resize(m_capacity);
        control.assign(m_capacity + 16, sen_empty);
        m_size = 0;

        for (size_t i = 0; i < temp_capacity; i += 16) {
            const __m128i group = _mm_loadu_si128(reinterpret_cast<const __m128i*>(temp_control.data() + i));
            uint16_t mask = static_cast<uint16_t>(~_mm_movemask_epi8(group));
            while (mask) {
                const size_t j = i + static_cast<size_t>(countr_zero(mask));
                mask &= mask - 1;
                insert(temp_slots[j].first, temp_slots[j].second);
            }
        }
    }

    void set_control(size_t i, uint8_t x) {
        control[i] = x;
        if (i < 16) {
            control[i + m_capacity] = x;
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