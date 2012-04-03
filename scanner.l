/* scanner, Daniel Achleitner (0926807) */
%option yylineno
%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>
#include "y.tab.h"

static void  error(void);
static unsigned long to_int(const char* s, int base);
%}

ID	[a-zA-Z_][a-zA-Z0-9_]*
HEX	[0-9][0-9a-fA-F]*
DEC	&[0-9]+

%%

"(*"([^\*]|\*[^\)])*"*)"	; /* eat comments */

"end"		return END;
"return"	return RETURN;
"goto"		return GOTO;
"if"		return IF;
"then"		return THEN;
"var"		return VAR;
"not"		return NOT;
"and"		return AND;

"=<"		return LTOREQ;

"-"		return '-';
";"		return ';';
"("		return '(';
")"		return ')';
","		return ',';
":"		return ':';
"="		return '=';
"*"		return '*';
"+"		return '+';
"#"		return '#';

{DEC}	printf("num %lu\n", to_int(yytext + 1, 10)); return NUM;
{HEX}	printf("num %lu\n", to_int(yytext, 16)); return NUM;
{ID}	printf("ident %s\n", yytext); return IDENT;

[ \t\n]	; /* whitespace be-gone */
.	error(); /* everything else */

%%

static unsigned long to_int(const char* s, int base)
{
	return strtoul(s, NULL, base);
}

static void error(void)
{
	fprintf(stderr, "scanner error: line %d: %s\n", yylineno, yytext);
	exit(1);
}
