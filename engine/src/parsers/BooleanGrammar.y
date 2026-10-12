%require "3.2"
%language "c++"
%define api.namespace {ctbndl}
%define api.value.type variant
%define api.token.constructor
%define api.token.prefix {TOK_}
%param {yyscan_t scanner}
%parse-param {BNDParseContext& ctx}

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
     BooleanGrammar.y

   Authors:
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
     Vincent Noël <vincent.noel@curie.fr> 
   Date:
     January-March 2011
*/

#include "BooleanGrammar.h"

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif
}

%code {
// The scanner, from lex.CTBNDL.cc which is included at the end of this file.
#define yylex ctbndllex
static ctbndl::parser::symbol_type yylex(yyscan_t yyscanner);
}

%type<NodeDecl*> node_decl
%type<std::vector<NodeDeclItem*>*> node_decl_item_list
%type<NodeDeclItem*> node_decl_item
%type<DivisionDecl*> division_decl
%type<Expression*> division_decl_rate
%type<std::vector<DivisionDaughterDecl*>*> division_list_daughter
%type<DivisionDaughterDecl*> division_decl_daughter
%type<Expression*> death_decl
%type<Expression*> death_decl_rate
%type<Expression*> cell_number
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
%type<ArgumentList*> argument_expression_list

%token<std::string> IDENTIFIER VARIABLE STRING
%token<double> DOUBLE
%token<long long> INTEGER

%token LOGAND LOGOR LOGXOR LOGNOT EQUAL NOT_EQUAL NODE GTEQ LTEQ DIVISION DEATH RATE DAUGHTER1 DAUGHTER2 CELL_NUMBER SINGLE_EXPRESSION

%%

translation_unit: decl
{
}
| translation_unit decl
{
}
;

decl: node_decl
{
}
| division_decl
{
}
| death_decl
{
}
| SINGLE_EXPRESSION expression ';'
{
  ctx.expression = $2;
}
;

node_decl: NODE IDENTIFIER '{' node_decl_item_list '}'
{
  NodeDecl* node_decl = new NodeDecl($2, $4, ctx.network);
  for (auto * node_decl_item : *($4)) {
    delete node_decl_item;
  }
  delete $4;
  delete node_decl;
}
| NODE IDENTIFIER '{' '}'
{
  NodeDecl* node_decl = new NodeDecl($2, nullptr, ctx.network);
  delete node_decl;
}
| IDENTIFIER ':' expression term_opt
{
  NodeDeclItem* decl_item = new NodeDeclItem("logic", $3);
  std::vector<NodeDeclItem*>* decl_item_v = new std::vector<NodeDeclItem*>();
  decl_item_v->push_back(decl_item);

  NodeDecl* node_decl = new NodeDecl($1, decl_item_v, ctx.network);
  for (auto * d_decl_item : *decl_item_v) {
    delete d_decl_item;
  }
  delete decl_item_v;
  delete node_decl;
}
| IDENTIFIER ',' expression term_opt
{
  if ($1 == "targets" && $3->toString() == "factors") {
    ctx.network->removeLastNode($3->toString());
    delete $3;
  } else {
    NodeDeclItem* decl_item = new NodeDeclItem("logic", $3);
    std::vector<NodeDeclItem*>* decl_item_v = new std::vector<NodeDeclItem*>();
    decl_item_v->push_back(decl_item);

    NodeDecl* node_decl = new NodeDecl($1, decl_item_v, ctx.network);
    for (auto * node_decl_item : *decl_item_v) {
      delete node_decl_item;
    }
    delete decl_item_v;
    delete node_decl;
  }
}
;

node_decl_item_list: node_decl_item
{
  $$ = new std::vector<NodeDeclItem*>();
  $$->push_back($1);
} 
| node_decl_item_list node_decl_item
{
  $1->push_back($2);
  $$ = $1;
}
;

node_decl_item: IDENTIFIER '=' expression ';'
{
  $$ = new NodeDeclItem($1, $3);
}
| IDENTIFIER '=' STRING ';'
{
  $$ = new NodeDeclItem($1, $3);
}
;

division_decl: DIVISION '{' division_decl_rate division_list_daughter '}'
{
  DivisionDecl* div_decl = new DivisionDecl(ctx.pop_network, $4, $3);
  for (auto * div_daughter_decl : *($4)) {
    delete div_daughter_decl;
  }
  delete $4;
  delete div_decl;
}
| DIVISION '{' division_decl_rate '}'
{
  std::vector<DivisionDaughterDecl*>* empty_daughters = nullptr;
  DivisionDecl* div_decl = new DivisionDecl(ctx.pop_network, empty_daughters, $3);
  delete div_decl;
}
| DIVISION '{' division_list_daughter '}'
{
  Expression* empty_rate = nullptr;
  DivisionDecl* div_decl = new DivisionDecl(ctx.pop_network, $3, empty_rate);
  for (auto * div_daughter_decl : *($3)) {
    delete div_daughter_decl;
  }
  delete $3;
  delete div_decl;
}
;

division_decl_rate: RATE '=' expression ';'
{
  $$ = $3;
}
;

division_list_daughter: division_decl_daughter
{
  $$ = new std::vector<DivisionDaughterDecl*>();
  $$->push_back($1);
}
| division_list_daughter division_decl_daughter
{ 
  $1->push_back($2);
  $$ = $1; 
}
;

division_decl_daughter: IDENTIFIER '.' DAUGHTER1 '=' expression ';'
{  
  $$ = new DivisionDaughterDecl(ctx.network->getOrMakeNode($1), DivisionRule::DAUGHTER_1, $5);
}
| IDENTIFIER '.' DAUGHTER2 '=' expression ';'
{
  $$ = new DivisionDaughterDecl(ctx.network->getOrMakeNode($1), DivisionRule::DAUGHTER_2, $5);  
}
;

death_decl: DEATH '{' death_decl_rate '}'
{
}
;

death_decl_rate: RATE '=' expression ';'
{
  if (ctx.pop_network == nullptr) 
      throw BNException("Please use PopMaBoSS to simulate PopMaBoSS models");

  ctx.pop_network->setDeathRate($3);
}
;

primary_expression: IDENTIFIER
{
  Node* node = ctx.network->getOrMakeNode($1);
  $$ = new NodeExpression(node);
}
| '@' IDENTIFIER
{
  $$ = new AliasExpression($2);
}
| VARIABLE
{
  $$ = new SymbolExpression(ctx.network->getSymbolTable(), ctx.network->getSymbolTable()->getOrMakeSymbol($1));
}
| INTEGER
{
  $$ = new ConstantExpression((double) $1);
  
}
| DOUBLE
{
  $$ = new ConstantExpression($1);
}
| '(' expression ')'
{
  $$ = new ParenthesisExpression($2);
}
| '#' CELL_NUMBER '(' cell_number ')'
{
  $$ = new PopExpression($4);
}
;

cell_number:
{
  $$ = nullptr;
}
| expression
{
  $$ = $1;
}

postfix_expression: primary_expression
{
  $$ = $1;
}
| IDENTIFIER '(' argument_expression_list ')'
{
  $$ = new FuncCallExpression($1, $3);
}
| IDENTIFIER '(' ')'
{
  $$ = new FuncCallExpression($1, nullptr);
}
;

argument_expression_list: conditional_expression
{
  $$ = new ArgumentList();
  $$->push_back($1);
}
| argument_expression_list ',' conditional_expression
{
  $$ = $1;
  $$->push_back($3);
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
| equality_expression LOGAND logical_and_expression
{
  $$ = new AndLogicalExpression($1, $3);
}
;

logical_or_expression: logical_and_expression
{
  $$ = $1;
}
| logical_and_expression LOGOR logical_or_expression
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

term_opt :
| ';'
;

%%

void ctbndl::parser::error(const std::string&)
{
  throw BNException("BND syntax error at line #" + std::to_string(ctx.lineno));
}

#include "lex.CTBNDL.cc"

namespace {
// Owns a scanner for the duration of one parse.
class Scanner {
  yyscan_t scanner = nullptr;
public:
  explicit Scanner(BNDParseContext& ctx) {
    if (ctbndllex_init_extra(&ctx, &scanner) != 0) {
      throw BNException("network parsing: cannot create the scanner");
    }
  }
  ~Scanner() { ctbndllex_destroy(scanner); }
  Scanner(const Scanner&) = delete;
  Scanner& operator=(const Scanner&) = delete;
  yyscan_t get() const { return scanner; }
};
}

int ctbndl_parse_file(FILE* in, BNDParseContext& ctx)
{
  Scanner scanner(ctx);
  ctbndlset_in(in, scanner.get());
  ctbndl::parser parser(scanner.get(), ctx);
  return parser.parse();
}

int ctbndl_parse_string(const char* str, BNDParseContext& ctx)
{
  Scanner scanner(ctx);
  ctbndl_scan_string(str, scanner.get());
  ctbndl::parser parser(scanner.get(), ctx);
  return parser.parse();
}
