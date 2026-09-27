#include "oa_map.hpp"

template<typename K, typename V>
OA_Map<K, V>::OA_Map() {
    OA_Map(32);
}

template<typename K, typename V>
OA_Map<K, V>::OA_Map(size_t capacity) {
    this->capacity = capacity; 
    slot.resize(capacity);
    state.resize(capacity + 1);
    states.fill(state::empty);
    states.back() = state::end;
}

template<typename K, typename V>
void OA_Map<K, V>::insert(const K& key, const V& value) {
    uint64_t hashed = hash(key);
    const size_t index = hashed % capacity;
    for (size_t i = index; ; ++i) {
        if (states[i] == state::filled && slot[i].first == key) {
            slot[i].second = value;
        }

        else if (states[i] == state::empty) {
            slot[i] = {key, value};
            return;
        }

        else if (states[i] == state::end) {
            i = -1;
        }
    }
}

template<typename K, typename V>
void OA_Map<K, V>::erase(const K& key) {
    uint64_t hashed = hash(key);
    const size_t index = hashed % capacity;
    for (size_t i = index; ; ++i) {
        if (states[i] == state::filled && slot[i].first == key) {
            states[i] = state::deleted;
            return;
        }

        else if (states[i] == state::empty) {
            return;
        }

        else if (states[i] == state::end) {
            i = -1;
        }
    }
}

template<typename K, typename V>
void OA_Map<K, V>::set_empty_sentinel(const V& sentinel) {
    sen_empty = sentinel;
}

template<typename K, typename V>
void OA_Map<K, V>::set_deleted_sentinel(const V& sentinel) {
    sen_deleted = sentinel;
}

template<typename K, typename V>
bool OA_Map<K, V>::empty() const {
    return size() == 0;
}

template<typename K, typename V>
size_t OA_Map<K, V>::size() const {
    return m_size;
}


template<typename K, typename V>
V& OA_Map<K, V>::operator[](const K& key) {
    
}

template <typename K, typename V>
OA_Iterator<K, V> OA_Map<K, V>::begin() {

}

template <typename K, typename V>
OA_Iterator<K, V> OA_Map<K, V>::end() {

}

template<typename K, typename V>
OA_Iterator<K, V> OA_Map<K, V>::find(const K& key) {
    uint64_t hashed = hash(key);
    const size_t index = hashed % capacity;
    for (size_t i = index; states[i] != state::empty; ++i) {
        if (states[i] == state::filled && slot[i].first == key) {
            return OA_Iterator()
        }

        else if (states[i] == state::end) {
            i = -1;
        }
    }

    return end();
}

template<typename K, typename V>
uint64_t OA_Map<K, V>::hash(const K& key) {

}

template<typename K, typename V>
void OA_Map<K, V>::rehash() {
    
}