#include "oa_map.hpp"

template<typename K, typename V>
OA_Map<K, V>::OA_Map() {
    OA_Map(32);
}

template<typename K, typename V>
OA_Map<K, V>::OA_Map(size_t capacity) {
    this->capacity = capacity; 
    slots.resize(capacity);
}

template<typename K, typename V>
void OA_Map<K, V>::insert(const K& key, const V& value) {
    uint64_t hashed = hash(key);
    const size_t index = hashed % capacity;
    for (size_t i = index; ; ++i) {
        if (slots[i].first == key) {
            slots[i].second = value;
        }

        else if (slots[i] == sen_empty || slots[i] == sen_deleted) {
            slots[i] = {key, value};
            return;
        }

        else if (i >= capacity - 1) {
            i = -1;
        }
    }
}

template<typename K, typename V>
void OA_Map<K, V>::erase(const K& key) {
    uint64_t hashed = hash(key);
    const size_t index = hashed % capacity;
    for (size_t i = index; ; ++i) {
        if (slots[i].first == key) {
            slots[i].first = sen_deleted;
            return;
        }

        else if (slots[i] == sen_empty) {
            return;
        }

        else if (i >= capacity - 1) {
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
    for (size_t i = 0; i < capacity; ++i) {
        if (slots[i].first != sen_empty || slots[i].first != sen_deleted) {
            return OA_Iterator(&slots[i]);
        }
    }

    return end();
}

template <typename K, typename V>
OA_Iterator<K, V> OA_Map<K, V>::end() {

}

template<typename K, typename V>
OA_Iterator<K, V> OA_Map<K, V>::find(const K& key) {
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

template<typename K, typename V>
uint64_t OA_Map<K, V>::hash(const K& key) {

}

template<typename K, typename V>
void OA_Map<K, V>::rehash() {
    
}