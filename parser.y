/* parser, Daniel Achleitner (0926807) */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define YYERROR_VERBOSE 1

int yylex(void);
void yyerror(const char *str);

%}

%start program

%token END	RETURN	GOTO	IF
%token THEN	VAR	NOT	AND
%token LTOREQ	NUM	IDENT

%%

program	:	/* empty */
	|	program funcdef ';'
	;

funcdef	:	IDENT '(' pars lastpar ')' stats END	/* Funktionsdefinition */
	;	

pars	:	/* empty */				/* Parameterdefinition */
	|	pars IDENT ','
	;

lastpar	:	/* empty */
	|	IDENT
	;

stats	:	/* empty */
	|	stats labels stat ';'
	;

labels	:	/* empty */
	|	labels IDENT ':'
	;

stat	:	RETURN expr
	|	GOTO IDENT
	|	IF expr THEN stats END
	|	VAR IDENT '=' expr			/* Variablendefinition */
	|	lexpr '=' expr				/* Zuweisung */
	|	term
	;

lexpr	:	IDENT					/* schreibender Variablenzugriff */
	|	'*' unary				/* schreibender Speicherzugriff */
	;

expr	:	unary
	|	term
	|	term termops
	|	term LTOREQ term
	|	term '#' term
	;

termops	:	/* empty */
	|	termops '+' term
	|	termops '*' term
	|	termops AND term
	;

unary	:	unaryops unary
	|	'*' unary				/* lesender Speicherzugriff */
	|	term
	;

unaryops:	/* empty */
	|	unaryops NOT
	|	unaryops '-'
	;

term	:	'(' expr ')'
	|	NUM
	|	IDENT					/* Variablenverwendung */
	|	IDENT '(' exprs lastexpr ')'		/* Funktionsaufruf */
	;

exprs	:	/* empty */
	|	exprs expr ','
	;

lastexpr:	/* empty */
	|	expr
	;

%%

extern int yylineno;

void yyerror(const char *str)
{
	fprintf(stderr, "parser error: line %d: %s\n", yylineno, str);
	exit(2);
}

int yywrap()
{
	return 1;
}

int main()
{
	yyparse();

	return EXIT_SUCCESS;
}
