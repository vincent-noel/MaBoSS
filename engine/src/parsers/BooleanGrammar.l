%option prefix="ctbndl"
%option reentrant noyywrap nounput
%option extra-type="BNDParseContext*"
%{
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
     BooleanGrammar.l

   Authors:
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
     Vincent Noël <vincent.noel@curie.fr>
 
   Date:
     January-March 2011
*/

#include <cmath>
#include "../compatibility_header.h"

// This file is included into BooleanGrammar.cc, after the parser.
#define YY_DECL static ctbndl::parser::symbol_type ctbndllex(yyscan_t yyscanner)
#define yyterminate() return ctbndl::parser::make_YYEOF()

static std::string yytokstr(yyscan_t yyscanner);
static void skip_comment(yyscan_t yyscanner);
%}

fracconst	([0-9]+\.[0-9]+)|([0-9]+\.)|(\.[0-9]+)
exppart		[eE](\-|\+)?[0-9]+

%%

[\t\f\v\r ]+ { }
"\n" { yyextra->lineno++; }
"/*" { skip_comment(yyscanner); }
"//".* { }
^"#".*"\n" { yyextra->lineno++; }

[a-zA-Z_][a-zA-Z_0-9]* {
  if (!strcasecmp(yytext, "NOT")) {
     return ctbndl::parser::make_LOGNOT();
  }
  if (!strcasecmp(yytext, "AND")) {
     return ctbndl::parser::make_LOGAND();
  }
  if (!strcasecmp(yytext, "OR")) {
     return ctbndl::parser::make_LOGOR();
  }
  if (!strcasecmp(yytext, "XOR")) {
     return ctbndl::parser::make_LOGXOR();
  }
  if (!strcasecmp(yytext, "NODE")) {
     return ctbndl::parser::make_NODE();
  }

  if (yyextra->pop_network != nullptr)  {
    if (!strcasecmp(yytext, "DIVISION")) {
      return ctbndl::parser::make_DIVISION();
    }
    if (!strcasecmp(yytext, "DEATH")) {
       return ctbndl::parser::make_DEATH();
    }
    if (!strcasecmp(yytext, "RATE")) {
       return ctbndl::parser::make_RATE();
    }
    if (!strcasecmp(yytext, "DAUGHTER1")) {
       return ctbndl::parser::make_DAUGHTER1();
    }
    if (!strcasecmp(yytext, "DAUGHTER2")) {
       return ctbndl::parser::make_DAUGHTER2();
    }
    if (!strcasecmp(yytext, "CELL")) {
       return ctbndl::parser::make_CELL_NUMBER();
    }
  }
  if (!strcasecmp(yytext, "SINGLE_EXPRESSION")) {
    return ctbndl::parser::make_SINGLE_EXPRESSION();
  }
  
  return ctbndl::parser::make_IDENTIFIER(yytext);
}
"$"[a-zA-Z_][a-zA-Z_0-9]* {
  return ctbndl::parser::make_VARIABLE(yytext);
}
\" {
  return ctbndl::parser::make_STRING(yytokstr(yyscanner));
}
"=="			{ return ctbndl::parser::make_EQUAL(); }
"!="			{ return ctbndl::parser::make_NOT_EQUAL(); }
"<="			{ return ctbndl::parser::make_LTEQ(); }
">="			{ return ctbndl::parser::make_GTEQ(); }
"&&"|"&"		{ return ctbndl::parser::make_LOGAND(); }
"||"|"|"		{ return ctbndl::parser::make_LOGOR(); }
"!"			{ return ctbndl::parser::make_LOGNOT(); }
[0-9]+			{ long long l; sscanf(yytext, "%lld", &l); return ctbndl::parser::make_INTEGER(l); }
{fracconst}{exppart}?	{ return ctbndl::parser::make_DOUBLE(atof(yytext)); }
[0-9]+{exppart}		{ return ctbndl::parser::make_DOUBLE(atof(yytext)); }
.			{ return ctbndl::parser::symbol_type(yytext[0]); }

%%

// Drop the backslash of the escape sequences in s.
static std::string yypurgestring(const std::string& s)
{
  std::string r;
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c != '\\' || i + 1 == s.size()) {
      r += c;
      continue;
    }
    c = s[++i];
    switch (c) {
    case 'a': r += '\a'; break;
    case 'b': r += '\b'; break;
    case 'f': r += '\f'; break;
    case 'n': r += '\n'; break;
    case 'r': r += '\r'; break;
    case 't': r += '\t'; break;
    case 'v': r += '\v'; break;
    case '\'': r += '\''; break;
    case '"': r += '"'; break;
    case '\\': r += '\\'; break;
    default:
      r += '\\';
      r += c;
    }
  }
  return r;
}

// Read the rest of a string literal, after its opening quote.
static std::string yytokstr(yyscan_t yyscanner)
{
  struct yyguts_t* yyg = (struct yyguts_t*)yyscanner;
  std::string s;
  unsigned char c;
  int backslash = 0;
  while ((c = (unsigned char) yyinput(yyscanner)) != 0 && c != 255) {
    if (c == '\n') {
      yyextra->lineno++;
    }
    else if (c == '\\') {
      backslash = !backslash;
    }
    else if (c == '"' && !backslash) {
      break;
    }
    else {
      backslash = 0;
    }
    s += (char) c;
  }
  return yypurgestring(s);
}

static void skip_comment(yyscan_t yyscanner)
{
  struct yyguts_t* yyg = (struct yyguts_t*)yyscanner;
  int c1, c2;

  c1 = yyinput(yyscanner);
  c2 = yyinput(yyscanner);

  while (c2 != EOF && !(c1 == '*' && c2 == '/')) {
    if (c1 == '\n') {
      ++yyextra->lineno;
    }
    c1 = c2;
    c2 = yyinput(yyscanner);
  }
}
