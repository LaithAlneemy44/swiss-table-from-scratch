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
    
}

template<typename K, typename V>
void OA_Map<K, V>::erase(const K& key) {
    
}

template<typename K, typename V>
void OA_Map<K, V>::find(const K& key) const {

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
OA_Iterator<K, V> OA_Map<K, V>::begin() const {

}

template <typename K, typename V>
OA_Iterator<K, V> OA_Map<K, V>::end() const {

}

template<typename K, typename V>
void OA_Map<K, V>::rehash() {
    
}