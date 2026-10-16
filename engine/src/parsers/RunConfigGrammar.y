%require "3.2"
%language "c++"
%define api.namespace {MaBoSS::rc}
%define api.value.type variant
%define api.token.constructor
%define api.token.prefix {TOK_}
%param {yyscan_t scanner}
%parse-param {RCParseContext& ctx}

%code requires {
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
     RunConfigGrammar.l

   Authors:
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
     Vincent Noël <vincent.noel@curie.fr>
 
   Date:
     January-March 2011
     updated to add extended initial conditions: October 2014
*/

#include "../Node.h"
#include "../Expressions.h"
#include "../RunConfig.h"
#include "../compatibility_header.h"
#include "RunConfigGrammar.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif
}

%code {
// The scanner, from lex.RC.cc which is included at the end of this file.
#define yylex rclex
namespace rc = MaBoSS::rc;
static rc::parser::symbol_type yylex(yyscan_t yyscanner);
}

%type<Expression*> primary_expression 
%type<Expression*> postfix_expression 
%type<Expression*> unary_expression 
%type<Expression*> multiplicative_expression 
%type<Expression*> additive_expression 
%type<Expression*> relational_expression 
%type<Expression*> equality_expression
%type<Expression*> logical_and_expression
%type<Expression*> logical_or_expression
%type<Expression*> logical_xor_expression
%type<Expression*> conditional_expression
%type<Expression*> expression
%type<std::vector<const Node*>*> symbol_list
%type<std::vector<const Node*>*> symbol_istate_list
%type<std::vector<const Node*>*> state
%type<std::vector<Expression*>*> expression_list
%type<IStateGroup::ProbaIState*> istate_expression
%type<std::vector<IStateGroup::ProbaIState*>*> istate_expression_list
%type<PopIStateGroup::PopProbaIState*> pop_istate_expression
%type<std::vector<PopIStateGroup::PopProbaIState*>*> pop_istate_expression_list
%type<std::vector<PopIStateGroup::PopProbaIState::PopIStateGroupIndividual*>*> pop_istate_population_list
%type<PopIStateGroup::PopProbaIState::PopIStateGroupIndividual*> pop_istate_individual
%type<ArgumentList*> argument_expression_list
%type<std::map<double, Expression*>*> schedule_list

%token<std::string> VARIABLE
%token<std::string> SYMBOL
%token<double> DOUBLE
%token<long long> INTEGER

%token LOGAND LOGOR LOGXOR LOGNOT EQUAL NOT_EQUAL GTEQ LTEQ CUSTOM_POP_OUTPUT NUMBER_CELL NODE_SEP

%%

translation_unit: decl
{
}
| translation_unit decl
{
}
;

decl: var_decl
| runconfig_decl
| node_attr_decl
| custom_pop_output_decl
| ';'
;

runconfig_decl: SYMBOL '=' expression ';'
{
  NetworkState network_state;
  double value = $3->eval(nullptr, network_state);
  ctx.config->setParameter($1, value);
  delete $3;
}
;

node_attr_decl: SYMBOL '.' SYMBOL '=' expression ';'
{
  Node* node = ctx.network->getNode($1);
  NetworkState network_state;
  double value = $5->eval(nullptr, network_state);
  if (!strcasecmp($3.c_str(), "istate")) {
    if (backward_istate) {
      node->setIState((bool)value);
    } else {
      if (value < 0) {
	new IStateGroup(ctx.network, node);
      } else {
	new IStateGroup(ctx.network, node, $5);
      }
    }
  } else if (!strcasecmp($3.c_str(), "is_internal")) {
    node->isInternal((bool)value);
  } else if (!strcasecmp($3.c_str(), "refstate")) {
    if (value < 0) {
      node->setReference(false);
    } else {
      node->setReferenceState((NodeState)value);
    }
  } else if (!strcasecmp($3.c_str(), "in_graph")) {
    node->inGraph((bool) value);
  } else {
    throw BNException(std::string(ctx.errorHead() + "invalid node attribute: ") + $3 + ", valid attributes are: istate or is_internal");
  }

  delete $5;
}
| SYMBOL '.' SYMBOL '=' schedule_list ';'
{
  Node* node = ctx.network->getNode($1);

  if (!strcasecmp($3.c_str(), "schedule"))
  {
    node->setScheduledFlips($5);
  } 
}
| symbol_istate_list '.' SYMBOL '=' istate_expression_list ';'
{
  if (strcasecmp($3.c_str(), "istate")) {
    throw BNException(std::string(ctx.errorHead() + "invalid node group attribute: ") + $3 + ", valid attribute is istate");
  }
  new IStateGroup(ctx.network, $1, $5);  
}
| symbol_istate_list '.' SYMBOL '=' pop_istate_expression_list ';'
{
  if (strcasecmp($3.c_str(), "pop_istate")) {
    throw BNException(std::string(ctx.errorHead() + "invalid node group attribute: ") + $3 + ", valid attribute is pop_istate");
  }
  PopNetwork* pop_network = static_cast<PopNetwork*>(ctx.network);
  new PopIStateGroup(pop_network, $1, $5);
}
;

symbol_istate_list: '[' symbol_list ']'
{
  $$ = $2;
}
;

symbol_list: SYMBOL
{
  $$ = new std::vector<const Node*>();
  $$->push_back(ctx.network->getNode($1));
}
| symbol_list ',' SYMBOL
{
  $$ = $1;
  $$->push_back(ctx.network->getNode($3));
}
;

istate_expression_list: istate_expression
{
  $$ = new std::vector<IStateGroup::ProbaIState*>();
  $$->push_back($1);
}
| istate_expression_list ',' istate_expression
{
  $$ = $1;
  $$->push_back($3);
}
;

istate_expression: primary_expression '[' expression_list ']'
{
  $$ = new IStateGroup::ProbaIState($1, $3);
  delete $1;
  for (std::vector<Expression*>::iterator it = $3->begin(); it != $3->end(); ++it) {
    delete *it;
  }
  delete $3;
}
;

pop_istate_expression_list: pop_istate_expression
{
  $$ = new std::vector<PopIStateGroup::PopProbaIState*>();
  $$->push_back($1);
}
| pop_istate_expression_list ',' pop_istate_expression
{
  $$ = $1;
  $$->push_back($3);
}
;

pop_istate_expression: primary_expression '[' pop_istate_population_list ']'
{
  $$ = new PopIStateGroup::PopProbaIState($1, $3);
}
;
 
pop_istate_population_list: pop_istate_individual
{
  $$ = new std::vector<PopIStateGroup::PopProbaIState::PopIStateGroupIndividual*>();
  $$->push_back($1);
  
} 
| pop_istate_population_list ',' pop_istate_individual
{
  $$ = $1;
  $$->push_back($3); 
}
;

pop_istate_individual: '{' '[' expression_list ']' ':' primary_expression '}'
{
  $$ = new PopIStateGroup::PopProbaIState::PopIStateGroupIndividual($3, $6);
  for (auto expr: *$3) {
    delete expr;
  }
  delete $3;
  delete $6;
}
;

schedule_list: DOUBLE ':' expression
{
  $$ = new std::map<double, Expression*>();
  (*$$)[$1] = $3;
}
| INTEGER ':' expression
{
  $$ = new std::map<double, Expression*>();
  (*$$)[(double)$1] = $3;
}
| schedule_list ',' DOUBLE ':' expression
{
  $$ = $1;
  (*$$)[$3] = $5;
}
| schedule_list ',' INTEGER ':' expression
{
  $$ = $1;
  (*$$)[(double)$3] = $5;
}


expression_list: primary_expression
{
  $$ = new std::vector<Expression*>();
  $$->push_back($1);
}
| expression_list ',' primary_expression
{
  $$ = $1;
  $$->push_back($3);
}
;

var_decl: VARIABLE '=' expression ';'
{
  const Symbol* symbol = ctx.network->getSymbolTable()->getOrMakeSymbol($1);
  NetworkState dummy_state;
  ctx.network->getSymbolTable()->setSymbolValue(symbol, $3->eval(nullptr, dummy_state));
  delete $3;
}
;

custom_pop_output_decl: CUSTOM_POP_OUTPUT '=' expression ';'
{
  ctx.config->setCustomPopOutputExpression($3);
}
;

primary_expression: INTEGER
{
  $$ = new ConstantExpression((double) $1);
}
| DOUBLE
{
  $$ = new ConstantExpression($1);
}
| VARIABLE
{
  $$ = new SymbolExpression(ctx.network->getSymbolTable(), ctx.network->getSymbolTable()->getOrMakeSymbol($1));
}
| '(' expression ')'
{
  $$ = new ParenthesisExpression($2);
}
| NUMBER_CELL '(' state ')'
{
  NetworkState network_state;
  for (auto* node : *$3) {
    network_state.setNodeState(node, 1);
  }
  StateExpression* state_expr = new StateExpression(network_state, ctx.network);
  
  $$ = new PopExpression(state_expr);
  delete $3;
}
;

state: SYMBOL
{
  $$ = new std::vector<const Node*>();
  $$->push_back(ctx.network->getNode($1));
}
| state NODE_SEP SYMBOL
{
  $$ = $1;
  $$->push_back(ctx.network->getNode($3));
}
;
 
argument_expression_list: expression
{
  $$ = new ArgumentList();
  $$->push_back($1);
}
| argument_expression_list ',' expression
{
  $$ = $1;
  $$->push_back($3);
}
;


postfix_expression: primary_expression
{
  $$ = $1;
}
| SYMBOL '(' argument_expression_list ')'
{
  $$ = new FuncCallExpression($1, $3);
}
| SYMBOL '(' ')'
{
  $$ = new FuncCallExpression($1, nullptr);
}
;


unary_expression: postfix_expression
{
  $$ = $1;
}
| '+' unary_expression
{
  $$ = $2;
}
| '-' unary_expression
{
  $$ = new SubExpression(new ConstantExpression(0.0), $2);
}
| '!' unary_expression
{
  $$ = new NotLogicalExpression($2);
}
| LOGNOT unary_expression
{
  $$ = new NotLogicalExpression($2);
}
;

multiplicative_expression: unary_expression
{
  $$ = $1;
}
| multiplicative_expression '*' unary_expression
{
  $$ = new MulExpression($1, $3);
}
| multiplicative_expression '/' unary_expression
{
  $$ = new DivExpression($1, $3);
}
;

additive_expression: multiplicative_expression
{
  $$ = $1;
}
| additive_expression '+' multiplicative_expression
{
  $$ = new AddExpression($1, $3);
}
| additive_expression '-' multiplicative_expression
{
  $$ = new SubExpression($1, $3);
}
;

relational_expression: additive_expression
{
  $$ = $1;
}
| relational_expression '<' additive_expression
{
  $$ = new LetterExpression($1, $3);
}
| relational_expression '>' additive_expression
{
  $$ = new GreaterExpression($1, $3);
}
| relational_expression LTEQ additive_expression
{
  $$ = new LetterOrEqualExpression($1, $3);
}
| relational_expression GTEQ additive_expression
{
  $$ = new GreaterOrEqualExpression($1, $3);
}
;

equality_expression: relational_expression
{
  $$ = $1;
}
| equality_expression EQUAL relational_expression
{
  $$ = new EqualExpression($1, $3);
}
| equality_expression NOT_EQUAL relational_expression
{
  $$ = new NotEqualExpression($1, $3);
}
;

logical_and_expression: equality_expression
{
  $$ = $1;
}
| logical_and_expression LOGAND equality_expression
{
  $$ = new AndLogicalExpression($1, $3);
}
;

logical_or_expression: logical_and_expression
{
  $$ = $1;
}
| logical_or_expression LOGOR logical_and_expression
{
  $$ = new OrLogicalExpression($1, $3);
}
;

logical_xor_expression: logical_or_expression
{
  $$ = $1;
}
| logical_xor_expression LOGXOR logical_or_expression
{
  $$ = new XorLogicalExpression($1, $3);
}
| logical_xor_expression '^' logical_or_expression
{
  $$ = new XorLogicalExpression($1, $3);
}
;

conditional_expression: logical_xor_expression
{
  $$ = $1;
}
| logical_xor_expression '?' expression ':' conditional_expression
{
  $$ = new CondExpression($1, $3, $5);
}
;

expression: conditional_expression
{
  $$ = $1;
}
;

%%

void rc::parser::error(const std::string&)
{
  if (!ctx.file.empty()) {
    throw BNException("configuration syntax error at line #" + std::to_string(ctx.lineno) + " in file \"" + ctx.file + "\"");
  } else if (ctx.expr) {
    throw BNException("configuration syntax error in expression '" + std::string(ctx.expr) + "'");
  } else {
    throw BNException("configuration syntax error at line #" + std::to_string(ctx.lineno));
  }
}

#include "lex.RC.cc"

namespace MaBoSS {

namespace {
// Owns a scanner for the duration of one parse.
class Scanner {
  yyscan_t scanner = nullptr;
public:
  explicit Scanner(RCParseContext& ctx) {
    if (rclex_init_extra(&ctx, &scanner) != 0) {
      throw BNException("configuration parsing: cannot create the scanner");
    }
  }
  ~Scanner() { rclex_destroy(scanner); }
  Scanner(const Scanner&) = delete;
  Scanner& operator=(const Scanner&) = delete;
  yyscan_t get() const { return scanner; }
};
}

int rc_parse_file(FILE* in, RCParseContext& ctx)
{
  Scanner scanner(ctx);
  rcset_in(in, scanner.get());
  rc::parser parser(scanner.get(), ctx);
  return parser.parse();
}

int rc_parse_string(const char* str, RCParseContext& ctx)
{
  Scanner scanner(ctx);
  rc_scan_string(str, scanner.get());
  rc::parser parser(scanner.get(), ctx);
  return parser.parse();
}

} // namespace MaBoSS
