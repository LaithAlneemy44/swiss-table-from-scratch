#include "oa_iterator.hpp"

template <typename K, typename V>
OA_Iterator<K, V>::OA_Iterator(pair<K, V>* slot) {
    ptr = slot;
}

template <typename K, typename V>
OA_Iterator<K, V>& OA_Iterator<K, V>::operator++() {
    
}

template <typename K, typename V>
OA_Iterator<K, V> OA_Iterator<K, V>::operator++(int) {

}

template <typename K, typename V>
OA_Iterator<K, V>& OA_Iterator<K, V>::operator--() {

}

template <typename K, typename V>
OA_Iterator<K, V> OA_Iterator<K, V>::operator--(int) {

}

template <typename K, typename V>
pair<K, V>& OA_Iterator<K, V>::operator*() const {
    return *ptr;
}

template <typename K, typename V>
pair<K, V>* OA_Iterator<K, V>::operator->() const {
    return ptr;
}

template <typename K, typename V>
bool OA_Iterator<K, V>::operator==(const OA_Iterator& other) const {
    return this->first == other->first && this->second == other->second;
}

template <typename K, typename V>
bool OA_Iterator<K, V>::operator!=(const OA_Iterator& other) const {
    return !(*this == other);
}