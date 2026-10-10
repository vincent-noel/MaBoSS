
#pragma once

#include <map>

#ifndef NETWORKSTATE_STD_MAP

#define USE_UNORDERED_MAP
#include <unordered_map>
template <typename K, typename V>
using StateMap = std::unordered_map<K, V>;

#else
template <typename K, typename V>
using StateMap = std::map<K, V>;
#endif
