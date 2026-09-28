/*
#############################################################################
#                                                                           #
# BSD 3-Clause License (see https://opensource.org/licenses/BSD-3-Clause)   #
#                                                                           #
# Copyright (c) 2011-2020 Institut Curie, 26 rue d'Ulm, Paris, France       #
# All rights reserved.                                                      #
#                                                                           #
# Redistribution and use in source and binary forms, with or without        #
# modification, are permitted provided that the following conditions are    #
# met:                                                                      #
#                                                                           #
# 1. Redistributions of source code must retain the above copyright notice, #
# this list of conditions and the following disclaimer.                     #
#                                                                           #
# 2. Redistributions in binary form must reproduce the above copyright      #
# notice, this list of conditions and the following disclaimer in the       #
# documentation and/or other materials provided with the distribution.      #
#                                                                           #
# 3. Neither the name of the copyright holder nor the names of its          #
# contributors may be used to endorse or promote products derived from this #
# software without specific prior written permission.                       #
#                                                                           #
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       #
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED #
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           #
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER #
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  #
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       #
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        #
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    #
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      #
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        #
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              #
#                                                                           #
#############################################################################

   Module:
     BooleanNetwork.h

   Authors:
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
     Vincent Noël <vincent.noel@curie.fr>
 
   Date:
     January-March 2011
*/

#ifndef _NETWORKSTATE_IMPL_H
#define _NETWORKSTATE_IMPL_H

#ifdef USE_DYNAMIC_BITSET

  #undef MAXNODES
  #define MAXNODES 0xFFFFFFF

#elif MAXNODES>64

  #define USE_STATIC_BITSET

#endif

#ifdef USE_STATIC_BITSET
#include <bitset>
#include <cstddef>
#include <functional>

// The network state for 65..MAXNODES nodes.
//
// Ordered containers keyed by the state (std::map<NetworkState_Impl, ...>,
// std::set<NetworkState_Impl>) need an ordering, and std::bitset has no
// operator<. It used to come from a specialization of
// std::less<std::bitset<MAXNODES>>, but recent libc++ (LLVM 23) no longer calls
// std::less<T> specializations inside std::map/std::set: it rewrites std::less<T>
// into std::less<> and compares with `a < b` directly. operator< cannot be added
// to std::bitset itself, so the state is a thin subclass that has one.
template <std::size_t N>
class MaBoSSBitset : public std::bitset<N> {
public:
  using std::bitset<N>::bitset;
  MaBoSSBitset() = default;
  MaBoSSBitset(const std::bitset<N>& bits) : std::bitset<N>(bits) { }

  // The most significant differing bit decides. This is exactly the ordering
  // the former std::less<std::bitset<MAXNODES>> specialization used, so the
  // iteration order of these containers -- and hence of MaBoSS output -- does
  // not change.
  bool operator<(const MaBoSSBitset& other) const {
    for (int i = static_cast<int>(N) - 1; i >= 0; i--) {
      if ((*this)[i] ^ other[i]) {
        return other[i];
      }
    }
    return false;
  }
};

typedef MaBoSSBitset<MAXNODES> NetworkState_Impl;

namespace std {
  template <std::size_t N> struct hash<MaBoSSBitset<N> > {
    size_t operator()(const MaBoSSBitset<N>& val) const {
      return std::hash<std::bitset<N> >{}(val);
    }
  };
}

#elif defined(USE_DYNAMIC_BITSET)
#include "MBDynBitset.h"
typedef MBDynBitset NetworkState_Impl;

#else
typedef unsigned long long NetworkState_Impl;
#endif

#endif
