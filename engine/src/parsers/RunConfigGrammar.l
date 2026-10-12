%option prefix="rc"
%option reentrant noyywrap nounput
%top{
// The scanner's extra data is the parse context. Set here rather than with
// %option extra-type, which the development version of flex (git master, used
// by the manylinux wheel builds) does not apply: yyextra stays a void*.
#define YY_EXTRA_TYPE RCParseContext*
}
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
     RunConfigGrammar.l

   Authors:
     Eric Viara <viara@sysra.com>
     Gautier Stoll <gautier.stoll@curie.fr>
     Vincent Noël <vincent.noel@curie.fr>
 
   Date:
     January-March 2011
*/

#include <cmath>

// This file is included into RunConfigGrammar.cc, after the parser.
#define YY_DECL static rc::parser::symbol_type rclex(yyscan_t yyscanner)
#define yyterminate() return rc::parser::make_YYEOF()

static void skip_comment(yyscan_t yyscanner);
%}

fracconst	([0-9]+\.[0-9]+)|([0-9]+\.)|(\.[0-9]+)
exppart		[eE](\-|\+)?[0-9]+

%%

[\t\f\v\r ]+ { }
"\n" { rcget_extra(yyscanner)->lineno++; }
"/*" { skip_comment(yyscanner); }
"//".* { }
[Tt][Rr][Uu][Ee]        { return rc::parser::make_INTEGER(1); }
[Ff][Aa][Ll][Ss][Ee]    { return rc::parser::make_INTEGER(0); }
"#cell" { return rc::parser::make_NUMBER_CELL(); }
"custom_pop_output" { return rc::parser::make_CUSTOM_POP_OUTPUT(); }
" -- " { return rc::parser::make_NODE_SEP(); }
"$"[a-zA-Z_][a-zA-Z_0-9]* {
  return rc::parser::make_VARIABLE(yytext);
}
[a-zA-Z_][a-zA-Z_0-9]* {
  return rc::parser::make_SYMBOL(yytext);
}
[0-9]+			{ long long l; sscanf(yytext, "%lld", &l); return rc::parser::make_INTEGER(l); }
{fracconst}{exppart}?	{ return rc::parser::make_DOUBLE(atof(yytext)); }
[0-9]+{exppart}		{ return rc::parser::make_DOUBLE(atof(yytext)); }
0x([0-9a-fA-F]*\.[0-9a-fA-F]+|[0-9a-fA-F]+\.?)p[-+]?[0-9]+ { return rc::parser::make_DOUBLE(atof(yytext)); }
.			{ return rc::parser::symbol_type(yytext[0]); }

%%

static void skip_comment(yyscan_t yyscanner)
{
  int c1, c2;

  c1 = yyinput(yyscanner);
  c2 = yyinput(yyscanner);

  while (c2 != EOF && !(c1 == '*' && c2 == '/')) {
    if (c1 == '\n') {
      ++rcget_extra(yyscanner)->lineno;
    }
    c1 = c2;
    c2 = yyinput(yyscanner);
  }
}
