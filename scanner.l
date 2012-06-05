/* scanner, Daniel Achleitner (0926807) */
%option yylineno
%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>

#include "y.tab.h"
#include "common.h"

static void  error(void);
static unsigned long to_int(const char* s, int base);
static char* lexeme(const char *str);
%}

ID	[a-zA-Z_][a-zA-Z0-9_]*
HEX	[0-9][0-9a-fA-F]*
DEC	&[0-9]+

%%

"(*"(.*|[ \n\t])*"*)"	; /* eat comments */

"end"		return END;
"return"	return RETURN;
"goto"		return GOTO;
"if"		return IF;	@{ @IF.lineno@ = yylineno; @}
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

{DEC}	return NUM; 	@{ @NUM.value@ = to_int(yytext + 1, 10); @}

{HEX}	return NUM; 	@{ @NUM.value@ = to_int(yytext, 16); @}

{ID}	return IDENT; 	@{ 
				@IDENT.name@ = lexeme(yytext); 
				@IDENT.lineno@ = yylineno;
			@}

[ \t\n]	; /* whitespace be-gone */
.	error(); /* everything else */

%%

static unsigned long to_int(const char* s, int base)
{
	return strtoul(s, NULL, base);
}

static char* lexeme(const char *str)
{
	char *lexstr = malloc(strlen(str) + 1);
	MALLOC_CHECK(lexstr)

	strcpy(lexstr, str);
	return lexstr;
}

static void error(void)
{
	fprintf(stderr, "scanner error: line %d: %s\n", yylineno, yytext);
	ERROR_LEX;
}
