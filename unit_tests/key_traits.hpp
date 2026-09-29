#pragma once
#include <string>

using namespace std;

template<typename K>
struct KeyTraits;

template<>
struct KeyTraits<int> {
    static int empty() { return -1; }
    static int deleted() { return -2; }
    static int key(int i) { return i; }
    static string name() { return "Int"; }
};

template<>
struct KeyTraits<long long> {
    static long long empty() { return -1; }
    static long long deleted() { return -2; }
    static long long key(int i) { return static_cast<long long>(i) * 1'000'003LL; }
    static string name() { return "LongLong"; }
};

template<>
struct KeyTraits<string> {
    static string empty() { return "\x01" "empty"; }
    static string deleted() { return "\x01" "deleted"; }
    static string key(int i) { return "key" + to_string(i); }
    static string name() { return "String"; }
};
