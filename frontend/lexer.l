%{
#include "parser.tab.h"
#include <string.h>
#include <stdlib.h>

static int lineno = 1;
%}

%%

[ \t\r]+                ;            /* skip horizontal whitespace */
\n                      { lineno++; } /* track lines */

"//".*                  ;            /* C++-style comments to EOL */

"sources" { return SOURCE_BUFFERS; }
"destinations" { return DESTINATION_BUFFERS; }

"process"               { return PROCESS; }
"size"                  { return SIZE; }
"MB"                    { return MB; }

"="                     { return '='; }
":"                     { return ':'; }
","                     { return ','; }
"["                     { return '['; }
"]"                     { return ']'; }

[A-Za-z_][A-Za-z0-9_]*  { yylval.sval = strdup(yytext); return IDENT; }

[0-9]+                  { yylval.ival = atoi(yytext); return NUMBER; }

.                       { /* unknown char */ return *yytext; }

%%

int yywrap(void) { return 1; }
int yylex_destroy(void);

