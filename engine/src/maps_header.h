
#pragma once

#include <map>
#include <unordered_map>

#ifndef NETWORKSTATE_STD_MAP
#define USE_UNORDERED_MAP
#endif

namespace MaBoSS {

#ifndef NETWORKSTATE_STD_MAP
template <typename K, typename V>
using StateMap = std::unordered_map<K, V>;
#else
template <typename K, typename V>
using StateMap = std::map<K, V>;
#endif

} // namespace MaBoSS
