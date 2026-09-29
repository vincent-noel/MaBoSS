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
     Expressions.h

   Authors:
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
     Vincent Noël <vincent.noel@curie.fr>
 
   Date:
     January-March 2011
*/

#ifndef _EXPRESSIONS_H_
#define _EXPRESSIONS_H_

#include <cstdlib>
#include <ostream>
#include <sstream>
#include <vector>
#include <algorithm>

#ifdef SBML_COMPAT
#include <sbml/SBMLTypes.h>
#include "sbml/packages/qual/common/QualExtensionTypes.h" 
LIBSBML_CPP_NAMESPACE_USE
#endif

#include "Node.h"
#include "Symbols.h"
#include "Function.h"
#include "Network.h"
#include "NetworkState.h"

class NotLogicalExpression;

class LogicalExprGenContext {

  const Network* network;
  const Node* node;
  std::ostream& os;
  unsigned int level;

 public:
  LogicalExprGenContext(const Network* network, const Node* node, std::ostream& os) : network(network), node(node), os(os), level(0) { }

  const Network* getNetwork() const {return network;}
  std::ostream& getOStream() const {return os;}

  const Node* getNode() {return node;}

  unsigned int getLevel() const {return level;}
  void incrLevel() {level++;}
  void decrLevel() {level--;}

  class LevelManager {

    LogicalExprGenContext& genctx;
    unsigned int level;

  public:
    LevelManager(LogicalExprGenContext& genctx) : genctx(genctx) {
      level = genctx.getLevel();
      genctx.incrLevel();
    }

    unsigned int getLevel() const {return level;}

    ~LevelManager() {
      genctx.decrLevel();
    }
  };
};


// abstract base class used for expression evaluation
class Expression {

public:
  virtual double eval(const Node* this_node, const NetworkState& network_state) const = 0;
  virtual double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop_state) const = 0;
  virtual double eval(const NetworkState&, double){
    return 0;
  }
  virtual bool hasCycle(Node* node) const = 0;

  std::string toString() const {
    std::ostringstream ostr;
    display(ostr);
    return ostr.str();
  }

  virtual Expression* clone() const = 0; 

  virtual Expression* cloneAndShrink(bool&) const {
    return clone();
  }

  virtual void display(std::ostream& os) const = 0;
  virtual bool isConstantExpression() const {return false;}
  virtual bool isLogicalExpression() const {return false;}
  virtual std::vector<Node*> getNodes() const{return std::vector<Node*>(); }
#ifdef SBML_COMPAT
  virtual ASTNode* writeSBML(LogicalExprGenContext&) const { return new ASTNode(AST_CONSTANT_TRUE); }
#endif
  virtual void generateLogicalExpression(LogicalExprGenContext& genctx) const = 0;
  virtual bool generationWillAddParenthesis() const {return false;}

  bool evalIfConstant(double& value) const;
  bool evalIfConstant(bool& value) const;

  static Expression* cloneAndShrinkRecursive(Expression* expr);

  virtual const NotLogicalExpression* asNotLogicalExpression() const {return nullptr;}

  virtual ~Expression() = default;
};


class TimeExpression : public Expression {

public:
  TimeExpression() = default;

  Expression* clone() const override {return new TimeExpression();}

  double eval(const Node*, const NetworkState&) const override {
    return 0;
  }

  double eval(const Node*, const NetworkState&, const PopNetworkState&) const override {
    return 0;
  }
  
  double eval(const NetworkState&, double time) override
  {
    return time;
  }

  bool hasCycle(Node*) const override {
    return false;
  }

  void display(std::ostream& os) const override {
    os << "#time";
  }

  bool isLogicalExpression() const override {return false;}
  
  std::vector<Node*> getNodes() const override{
    std::vector<Node*> vec;
    return vec;
  }

#ifdef SBML_COMPAT
  ASTNode* writeSBML(LogicalExprGenContext&) const override {
    ASTNode* time_node = new ASTNode(AST_NAME_TIME);
    return time_node;
  }
#endif
  void generateLogicalExpression(LogicalExprGenContext&) const override {}

  ~TimeExpression() override = default;
};

class NodeExpression : public Expression {
  Node* node;

public:
  NodeExpression(Node* node) : node(node) { }

  Expression* clone() const override {return new NodeExpression(node);}

  double eval(const Node*, const NetworkState& network_state) const override {
    return (double)node->getNodeState(network_state);
  }

  double eval(const Node*, const NetworkState& network_state, const PopNetworkState&) const override {
    return (double)node->getNodeState(network_state);
  }
  
  double eval(const NetworkState& network_state, double) override
  {
    return (double)node->getNodeState(network_state);
  }

  bool hasCycle(Node* _node) const override {
    return this->node == _node;
  }

  void display(std::ostream& os) const override {
    os << node->getLabel();
  }

  bool isLogicalExpression() const override {return true;}
  
  std::vector<Node*> getNodes() const override{
    std::vector<Node*> vec;
    vec.push_back(node);
    return vec;
  }

#ifdef SBML_COMPAT
  ASTNode* writeSBML(LogicalExprGenContext&) const override {
    ASTNode* equ = new ASTNode(AST_RELATIONAL_EQ);
    ASTNode* a_node = new ASTNode(AST_NAME);
    a_node->setId(node->getLabel());
    a_node->setName(node->getLabel().c_str());
    ASTNode* one = new ASTNode(AST_INTEGER);
    one->setValue(1);
    
    equ->addChild(a_node);
    equ->addChild(one);
    return equ;
  }
#endif
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;

  ~NodeExpression() override = default;
};


class StateExpression: public Expression {
  NetworkState state;
  Network* network;
public:
#ifdef USE_DYNAMIC_BITSET
  StateExpression(const NetworkState state, Network* network) : state(NetworkState(state, 1)), network(network) { }
  Expression* clone() const override {return new StateExpression(NetworkState(state, 1), network);}
#else
  StateExpression(const NetworkState state, Network* network) : state(state), network(network) { }
  Expression* clone() const override {return new StateExpression(NetworkState(state), network);}
#endif
  

  double eval(const Node*, const NetworkState& network_state) const override {
    return state.getState() == network_state.getState() ? 1.0 : 0.0;
  }

  double eval(const Node*, const NetworkState& network_state, const PopNetworkState&) const override {
    return state.getState() == network_state.getState() ? 1.0 : 0.0;
  }
  
  double eval(const NetworkState& network_state, double) override
  {
    return state.getState() == network_state.getState() ? 1.0 : 0.0;
  }
  
  bool hasCycle(Node*) const override {
    return false;
  }

  void display(std::ostream& os) const override {
    state.displayOneLine(os, network);
  }

  bool isLogicalExpression() const override {return true;}
  
  std::vector<Node*> getNodes() const override{
    std::vector<Node*> vec;
    for (auto* node: network->getNodes())
      if (state.getNodeState(node))
        vec.push_back(node);
    return vec;
  }
  
  void generateLogicalExpression(LogicalExprGenContext&) const override {}

  ~StateExpression() override = default;
};

class PopExpression : public Expression {
  Expression* expr;

public:
  PopExpression(Expression* expr) : expr(expr) { }

  Expression* clone() const override {return new PopExpression(expr->clone());}

  double eval(const Node*, const NetworkState&) const override {
    return 0.;
  }

  double eval(const Node*, const NetworkState&, const PopNetworkState& pop_state) const override {
    return (double) pop_state.count(expr);
  }
  
  bool hasCycle(Node*) const override {
    return false;
  }

  void display(std::ostream& os) const override {
    os << "#cell(";
    if (expr != nullptr) {
      expr->display(os);
    } else {
      os << "1";
    }
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}
  std::vector<Node*> getNodes() const override{
    return expr->getNodes();
  }
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;

  ~PopExpression() override {
    delete expr;
  }
};

// concrete classes used for expression evaluation
class BinaryExpression : public Expression {

protected:
  Expression* left;
  Expression* right;

public:
  BinaryExpression(Expression* left, Expression* right) : left(left), right(right) { }

  bool hasCycle(Node* node) const override {
    return left->hasCycle(node) || right->hasCycle(node);
  }

  bool isConstantExpression() const override {
    return left->isConstantExpression() && right->isConstantExpression();
  }
 
  std::vector<Node*> getNodes() const override{
    std::vector<Node*> vec1 = left->getNodes();
    std::vector<Node*> vec2 = right->getNodes();
    std::vector<Node*> vec(vec1.begin(), vec1.end());
    for (auto* node : vec2) {
      if (std::find(vec.begin(), vec.end(), node) == vec.end()) {
        vec.push_back(node);
      }
    }
    return vec;
  }
  
  ~BinaryExpression() override {
    delete left;
    delete right;
  }
};

class MulExpression : public BinaryExpression {

public:
  MulExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new MulExpression(left->clone(), right->clone());}
  Expression* cloneAndShrink(bool& shrinked) const override;
  
  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) * right->eval(this_node, network_state);
  }
  
  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) * right->eval(this_node, network_state, pop);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " * ";
    right->display(os);
    os <<  ")";
  }

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class DivExpression : public BinaryExpression {

public:
  DivExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new DivExpression(left->clone(), right->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) / right->eval(this_node, network_state);
  }
  
  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) / right->eval(this_node, network_state, pop);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " / ";
    right->display(os);
    os << ")";
  }

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class AddExpression : public BinaryExpression {

public:
  AddExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new AddExpression(left->clone(), right->clone());}
  Expression* cloneAndShrink(bool& shrinked) const override;

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) + right->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) + right->eval(this_node, network_state, pop);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " + ";
    right->display(os);
    os << ")";
  }

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class SubExpression : public BinaryExpression {

public:
  SubExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new SubExpression(left->clone(), right->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) - right->eval(this_node, network_state);
  }
  
  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) - right->eval(this_node, network_state, pop);
  }
  
  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " - ";
    right->display(os);
    os << ")";
  }

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class EqualExpression : public BinaryExpression {

public:
  EqualExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new EqualExpression(left->clone(), right->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) == right->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) == right->eval(this_node, network_state, pop);
  }
  
  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " == ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class NotEqualExpression : public BinaryExpression {

public:
  NotEqualExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new NotEqualExpression(left->clone(), right->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) != right->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) != right->eval(this_node, network_state, pop);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " != ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class LetterExpression : public BinaryExpression {

public:
  LetterExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new LetterExpression(left->clone(), right->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) < right->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) < right->eval(this_node, network_state, pop);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " < ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class LetterOrEqualExpression : public BinaryExpression {

public:
  LetterOrEqualExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new LetterOrEqualExpression(left->clone(), right->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) <= right->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) <= right->eval(this_node, network_state, pop);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " <= ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class GreaterExpression : public BinaryExpression {

public:
  GreaterExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new GreaterExpression(left->clone(), right->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) > right->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) > right->eval(this_node, network_state, pop);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " > ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class GreaterOrEqualExpression : public BinaryExpression {

public:
  GreaterOrEqualExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new GreaterOrEqualExpression(left->clone(), right->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return left->eval(this_node, network_state) >= right->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return left->eval(this_node, network_state, pop) >= right->eval(this_node, network_state, pop);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " >= ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class CondExpression : public Expression {

  Expression* cond_expr;
  Expression* true_expr;
  Expression* false_expr;

public:
  CondExpression(Expression* cond_expr, Expression* true_expr, Expression* false_expr) : cond_expr(cond_expr), true_expr(true_expr), false_expr(false_expr) { }

  Expression* clone() const override {return new CondExpression(cond_expr->clone(), true_expr->clone(), false_expr->clone());}

  Expression* cloneAndShrink(bool& shrinked) const override;
  //  Expression* cloneAndShrink(bool& shrinked) const {return new CondExpression(cond_expr->cloneAndShrink(shrinked), true_expr->cloneAndShrink(shrinked), false_expr->cloneAndShrink(shrinked));}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    if (0. != cond_expr->eval(this_node, network_state)) {
      return true_expr->eval(this_node, network_state);
    }
    return false_expr->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    if (0. != cond_expr->eval(this_node, network_state, pop)) {
      return true_expr->eval(this_node, network_state, pop);
    }
    return false_expr->eval(this_node, network_state, pop);
  }

  bool hasCycle(Node* node) const override {
    return cond_expr->hasCycle(node) || true_expr->hasCycle(node) || false_expr->hasCycle(node);
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    cond_expr->display(os);
    os <<  " ? ";
    true_expr->display(os);
    os <<  " : ";
    false_expr->display(os);
    os << ")";
  }

  bool isConstantExpression() const override {
    return cond_expr->isConstantExpression() && true_expr->isConstantExpression() && false_expr->isConstantExpression();
  }

  bool isLogicalExpression() const override {
    return true_expr->isLogicalExpression() && false_expr->isLogicalExpression();
  }

  std::vector<Node*> getNodes() const override{
    std::vector<Node*> vec1 = cond_expr->getNodes();
    std::vector<Node*> vec2 = true_expr->getNodes();
    std::vector<Node*> vec3 = false_expr->getNodes();
    std::vector<Node*> vec(vec1.begin(), vec1.end());
    for (auto* node : vec2) {
      if (std::find(vec.begin(), vec.end(), node) == vec.end()) {
        vec.push_back(node);
      }
    }
    for (auto* node : vec3) {
      if (std::find(vec.begin(), vec.end(), node) == vec.end()) {
        vec.push_back(node);
      }
    }
    return vec;
  }
  
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;

  ~CondExpression() override {
    delete cond_expr;
    delete true_expr;
    delete false_expr;
  }
};

class ConstantExpression : public Expression {

  double value;

public:
  ConstantExpression(double value) : value(value) { }

  Expression* clone() const override {return new ConstantExpression(value);}

  double eval(const Node*, const NetworkState&) const override {
    return value;
  }
  
  double eval(const Node*, const NetworkState&, const PopNetworkState&) const override {
    return value;
  }

  bool hasCycle(Node*) const override {
    return false;
  }

  void display(std::ostream& os) const override {
    os << value;
  }

  bool isConstantExpression() const override {return true;}

  bool isLogicalExpression() const override {return value == 0 || value == 1;}
  
  std::vector<Node*> getNodes() const override{
    return std::vector<Node*>();
  }
  
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;

};

class SymbolExpression : public Expression {

  SymbolTable* symbol_table;
  const Symbol* symbol;
  mutable bool value_set;
  mutable double value;

public:
  SymbolExpression(SymbolTable* symbol_table, const Symbol* symbol) : symbol_table(symbol_table), symbol(symbol), value_set(false) { 
    symbol_table->addSymbolExpression(this);
  }

  Expression* clone() const override {return new SymbolExpression(symbol_table, symbol);}

  double eval(const Node*, const NetworkState&) const override {
    if (!value_set) {
      value = symbol_table->getSymbolValue(symbol);
      value_set = true;
    }
    return value;
  }
  
  double eval(const Node*, const NetworkState&, const PopNetworkState&) const override {
    if (!value_set) {
      value = symbol_table->getSymbolValue(symbol);
      value_set = true;
    }
    return value;
  }

  bool hasCycle(Node*) const override {
    return false;
  }

  void display(std::ostream& os) const override {
    os << symbol->getName();
  }

  bool isConstantExpression() const override {return true;}
  
  std::vector<Node*> getNodes() const override{
    return std::vector<Node*>();
  }
  
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;

  void unset() { value_set = false; }
};

class AliasExpression : public Expression {
  std::string identifier;

  const Expression* getAliasExpression(const Node* this_node) const {
    if (nullptr != this_node) {
      return this_node->getAttributeExpression(identifier);
    }
    return nullptr;
  }

  mutable const Expression* alias_expr;

public:
  AliasExpression(const std::string& identifier) : identifier(identifier), alias_expr(nullptr) { }

  Expression* clone() const override {return new AliasExpression(identifier);}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    if (nullptr == alias_expr) {
      alias_expr = getAliasExpression(this_node);
    }
    if (nullptr != alias_expr) {
      return alias_expr->eval(this_node, network_state);
    }
    if (nullptr != this_node) {
      throw BNException("invalid use of alias attribute @" + identifier + " in node " + this_node->getLabel());
    }
    throw BNException("invalid use of alias attribute @" + identifier + " in unknown node");
  }
  
  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    if (nullptr == alias_expr) {
      alias_expr = getAliasExpression(this_node);
    }
    if (nullptr != alias_expr) {
      return alias_expr->eval(this_node, network_state, pop);
    }
    
    if (nullptr != this_node) {
      throw BNException("invalid use of alias attribute @" + identifier + " in node " + this_node->getLabel());
    }
    throw BNException("invalid use of alias attribute @" + identifier + " in unknown node");
  }

  bool hasCycle(Node*) const override {
    return false;
  }

  void display(std::ostream& os) const override {
    os << '@' << identifier;
  }
#ifdef SBML_COMPAT
  ASTNode* writeSBML(LogicalExprGenContext& genctx) const override {
    alias_expr = getAliasExpression(genctx.getNode());
    if (nullptr != alias_expr) {
      return alias_expr->writeSBML(genctx);
    }
    else return new ASTNode(AST_CONSTANT_FALSE);
  }
#endif
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class OrLogicalExpression : public BinaryExpression {

public:
  OrLogicalExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }
  Expression* cloneAndShrink(bool& shrinked) const override;

  Expression* clone() const override {return new OrLogicalExpression(left->clone(), right->clone());}

  bool generationWillAddParenthesis() const override {return true;}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return (double)((bool)left->eval(this_node, network_state) || (bool)right->eval(this_node, network_state));
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return (double)((bool)left->eval(this_node, network_state, pop) || (bool)right->eval(this_node, network_state, pop));
  }
  
  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " OR ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}

#ifdef SBML_COMPAT
  ASTNode* writeSBML(LogicalExprGenContext& genctx) const override {
    ASTNode* op = new ASTNode(AST_LOGICAL_OR);
    
    op->addChild(left->writeSBML(genctx));
    op->addChild(right->writeSBML(genctx));
    return op;
  }
#endif
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class AndLogicalExpression : public BinaryExpression {

public:
  AndLogicalExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new AndLogicalExpression(left->clone(), right->clone());}
  Expression* cloneAndShrink(bool& shrinked) const override;

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return (double)((bool)left->eval(this_node, network_state) && (bool)right->eval(this_node, network_state));
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return (double)((bool)left->eval(this_node, network_state, pop) && (bool)right->eval(this_node, network_state, pop));
  }

  double eval(const NetworkState& network_state, double time) override
  {
    return (double)((bool)left->eval(network_state, time ) && (bool)right->eval(network_state, time));
  }
  
  bool generationWillAddParenthesis() const override {return true;}

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " AND ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}
#ifdef SBML_COMPAT
  ASTNode* writeSBML(LogicalExprGenContext& genctx) const override {
    ASTNode* op = new ASTNode(AST_LOGICAL_AND);
    
    op->addChild(left->writeSBML(genctx));
    op->addChild(right->writeSBML(genctx));
    return op;
  }
#endif
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class XorLogicalExpression : public BinaryExpression {

public:
  XorLogicalExpression(Expression* left, Expression* right) : BinaryExpression(left, right) { }

  Expression* clone() const override {return new XorLogicalExpression(left->clone(), right->clone());}

  Expression* cloneAndShrink(bool& shrinked) const override;

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return (double)((bool)left->eval(this_node, network_state) ^ (bool)right->eval(this_node, network_state));
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return (double)((bool)left->eval(this_node, network_state, pop) ^ (bool)right->eval(this_node, network_state, pop));
  }

  void display(std::ostream& os) const override {
    os <<  "(";
    left->display(os);
    os <<  " XOR ";
    right->display(os);
    os << ")";
  }

  bool isLogicalExpression() const override {return true;}
#ifdef SBML_COMPAT
  ASTNode* writeSBML(LogicalExprGenContext& genctx) const override {
    ASTNode* op = new ASTNode(AST_LOGICAL_XOR);
    
    op->addChild(left->writeSBML(genctx));
    op->addChild(right->writeSBML(genctx));
    return op;
  }
#endif
  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
};

class NotLogicalExpression : public Expression {
  Expression* expr;

public:
  NotLogicalExpression(Expression* expr) : expr(expr) { }

  Expression* clone() const override {return new NotLogicalExpression(expr->clone());}
  Expression* cloneAndShrink(bool& shrinked) const override;

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return (double)(!((bool)expr->eval(this_node, network_state)));
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return (double)(!((bool)expr->eval(this_node, network_state, pop)));
  }
  
  double eval(const NetworkState& network_state, double time) override {
    return (double)(!((bool)expr->eval(network_state, time)));
  }
  
  bool hasCycle(Node* node) const override {
    return expr->hasCycle(node);
  }

  const NotLogicalExpression* asNotLogicalExpression() const override {return this;}

  void display(std::ostream& os) const override {
    os <<  "NOT ";
    expr->display(os);
  }

  bool isLogicalExpression() const override {return true;}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;
#ifdef SBML_COMPAT
  ASTNode* writeSBML(LogicalExprGenContext& genctx) const override {
    ASTNode* op = new ASTNode(AST_LOGICAL_NOT);
    
    op->addChild(expr->writeSBML(genctx));
    return op;
  }
#endif
  ~NotLogicalExpression() override {
    delete expr;
  }
};

class ParenthesisExpression : public Expression {
  Expression* expr;

public:
  ParenthesisExpression(Expression* expr) : expr(expr) { }

  Expression* clone() const override {return new ParenthesisExpression(expr->clone());}

  Expression* cloneAndShrink(bool& shrinked) const override {
    return new ParenthesisExpression(expr->cloneAndShrink(shrinked));
  }

  bool generationWillAddParenthesis() const override {return true;}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    return expr->eval(this_node, network_state);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    return expr->eval(this_node, network_state, pop);
  }

  const NotLogicalExpression* asNotLogicalExpression() const override {return expr->asNotLogicalExpression();}

  bool hasCycle(Node* node) const override {
    return expr->hasCycle(node);
  }

  void display(std::ostream& os) const override {
    os <<  '(';
    expr->display(os);
    os <<  ')';
  }

  std::vector<Node*> getNodes() const override{
    return expr->getNodes();
  }
  bool isConstantExpression() const override {return expr->isConstantExpression();}
  bool isLogicalExpression() const override {return expr->isLogicalExpression();}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;

#ifdef SBML_COMPAT
  ASTNode* writeSBML(LogicalExprGenContext& genctx) const override {

    return expr->writeSBML(genctx);
  }
#endif

  ~ParenthesisExpression() override {
    delete expr;
  }
};

class FuncCallExpression : public Expression {
  std::string funname;
  ArgumentList* arg_list;
  Function* function;
  bool is_const = false;
  double value;

public:
  FuncCallExpression(const std::string& funname, ArgumentList* arg_list) : funname(funname), arg_list(arg_list), function(nullptr), value(0.) {
    function = Function::getFunction(funname);

    if (function == nullptr) {
      throw BNException("unknown function " + funname);
    }
    function->check(arg_list);
    
    // This part is evaluating the formula, but if there is a parameter its value
    // has not been parsed yet. So this will fail. 
    // is_const = function->isDeterministic() && isConstantExpression();
    // if (is_const) {
    //   NetworkState network_state;
    //   value = function->eval(NULL, network_state, arg_list);
    // }
 }

  Expression* clone() const override {return new FuncCallExpression(funname, arg_list->clone());}

  double eval(const Node* this_node, const NetworkState& network_state) const override {
    if (is_const) {
      return value;
    }
    return function->eval(this_node, network_state, arg_list);
  }

  double eval(const Node* this_node, const NetworkState& network_state, const PopNetworkState& pop) const override {
    if (is_const) {
      return value;
    }
    return function->eval(this_node, network_state, pop, arg_list);
  }
  void display(std::ostream& os) const override {
    os <<  funname << '(';
    arg_list->display(os);
    os <<  ')';
  }

  bool hasCycle(Node* node) const override {
    return arg_list->hasCycle(node);
  }

  bool isConstantExpression() const override {return arg_list->isConstantExpression();}

  void generateLogicalExpression(LogicalExprGenContext& genctx) const override;

  ~FuncCallExpression() override {
    delete arg_list;
  }
};

extern bool dont_shrink_logical_expressions;

#endif
