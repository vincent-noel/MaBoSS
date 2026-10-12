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
     BooleanGrammar.h

   Authors:
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
     Vincent Noël <vincent.noel@curie.fr>
 
   Date:
     January-March 2011
*/

#pragma once

#include <cstdio>
#include <vector>
#include <cstring>
#include "../Network.h"
#include "../Node.h"
#include "../PopNetwork.h"
#include "../BNException.h"

class Expression;

// Everything a parse of the boolean network (.bnd) grammar reads or builds.
// Each parse gets its own context and its own scanner, so separate threads
// can parse separate networks at the same time.
struct BNDParseContext {
  Network* network;
  // Set when parsing a PopMaBoSS network: enables the division/death syntax.
  PopNetwork* pop_network;
  // Result of a SINGLE_EXPRESSION parse.
  Expression* expression = nullptr;
  unsigned int lineno = 1;

  BNDParseContext(Network* network, PopNetwork* pop_network) : network(network), pop_network(pop_network) { }
};

// Parse a whole .bnd input, from a file or from a string. Return the bison
// parser's result (0 on success); syntax errors throw BNException.
extern int ctbndl_parse_file(FILE* in, BNDParseContext& ctx);
extern int ctbndl_parse_string(const char* str, BNDParseContext& ctx);

class NodeDeclItem {

  std::string identifier;
  Expression* expr;
  std::string str;

public:
  NodeDeclItem(const std::string& identifier, Expression* expr) : identifier(identifier), expr(expr) { }
  NodeDeclItem(const std::string& identifier, const std::string& str) : identifier(identifier), expr(nullptr), str(str) { }

  const std::string& getIdentifier() const {return identifier;}
  Expression* getExpression() {return expr;}
  const std::string& getString() const {return str;}
};

class NodeDecl {

public:
  NodeDecl(const std::string& identifier, std::vector<NodeDeclItem*>* node_decl_item_list, Network* network) {
    bool reset = false;
    if (network->isNodeDefined(identifier)) {
      if (Node::isOverrideDefinition()) {
	reset = true;
      } else if (!Node::isAugment()) {
	throw BNException("node " + identifier + " already defined");
      }
    }

    network->setNodeAsDefined(identifier);

    Node* node = network->getOrMakeNode(identifier);
    if (reset) {
      node->reset();
    }
    if (nullptr == node_decl_item_list) {
      return;
    }
    size_t size = node_decl_item_list->size();
    for (size_t nn = 0; nn < size; nn++) {
      Expression* expr = (*node_decl_item_list)[nn]->getExpression();
      if (nullptr != expr) {
	node->setAttributeExpression((*node_decl_item_list)[nn]->getIdentifier(), expr);
      } else {
	node->setAttributeString((*node_decl_item_list)[nn]->getIdentifier(), (*node_decl_item_list)[nn]->getString());
      }
    }
  }
};

class DivisionDaughterDecl {
public:
  Node* node;
  int daughter;
  Expression * expr;
  
  DivisionDaughterDecl() {
    node = nullptr;
    daughter = 0;
    expr = nullptr;
  }
  DivisionDaughterDecl(Node* node, int daughter, Expression* expr) : node(node), daughter(daughter), expr(expr) {}
};

class DivisionDecl {
  public:
  
  DivisionDecl(PopNetwork* pop_network, std::vector<DivisionDaughterDecl*>* daughters, Expression* rate) {
    if (pop_network == nullptr)    
      throw BNException("Please use PopMaBoSS to simulate PopMaBoSS models");

    DivisionRule * new_rule = new DivisionRule();
    
    new_rule->setRate(rate);
    
    if (daughters != nullptr){
      for (auto daughter: *daughters) {
        new_rule->addDaughterNode(daughter->daughter, daughter->node, daughter->expr);
      }
    }
    
    pop_network->addDivisionRule(new_rule);
  }
};
