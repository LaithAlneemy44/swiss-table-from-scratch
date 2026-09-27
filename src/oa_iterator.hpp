#pragma once
#include <utility>

using namespace std;

template<typename K, typename V>
class OA_Iterator {
public:
    OA_Iterator();
    explicit OA_Iterator(pair<K, V>* slot);
    OA_Iterator& operator++();
    OA_Iterator operator++(int);
    OA_Iterator& operator--();
    OA_Iterator operator--(int);
    pair<K, V>& operator*() const;
    pair<K, V>* operator->() const;
    bool operator==(const OA_Iterator& other) const;
    bool operator!=(const OA_Iterator& other) const;

private:
    pair<K, V>* ptr;
};

#include "oa_iterator.tpp"