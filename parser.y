/* parser, Daniel Achleitner (0926807) */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "symtable.h"

int yylex(void);
void yyerror(const char *str);

%}

%start program

%token END	RETURN	GOTO	IF
%token THEN	VAR	NOT	AND
%token LTOREQ	NUM	IDENT

/* lexer synthesized attributes */
@attributes { char *name; int lineno; } IDENT
@attributes { unsigned long value; }    NUM

/* auto inherit labels table for whole function */
@autoinh i_flabs
@attributes { struct sym *i_lab; struct sym *s_lab; struct sym *i_flabs; } stats stat
@attributes { struct sym *i_lab; struct sym *s_lab; }                      labels

/* checks whether referenced labels/parameters/variables exist */
@traversal @preorder check

/* prints symbol table debug information */
@traversal @preorder print

%%

program	:	/* empty */
	|	program funcdef ';'
	;

funcdef	:	IDENT '(' pars lastpar ')' stats END	/* Funktionsdefinition */
		@{
			@i @stats.i_lab@ = NULL;
			@i @stats.i_flabs@ = @stats.s_lab@;
			@print sym_print(@stats.s_lab@, "labels", @IDENT.name@);
		@}
	;	

pars	:	/* empty */				/* Parameterdefinition */
	|	pars IDENT ','
	;

lastpar	:	/* empty */
	|	IDENT
	;

stats	:	/* empty */
		@{ @i @stats.s_lab@ = @stats.i_lab@; @}
	|	stats labels stat ';'
		@{
			@i @stats.1.i_lab@ = @stats.0.i_lab@;
			@i @labels.i_lab@ = @stats.1.s_lab@;
			@i @stat.i_lab@ = @labels.s_lab@;
			@i @stats.0.s_lab@ = @stat.s_lab@;
		@}
	;

labels	:	/* empty */
		@{ @i @labels.s_lab@ = @labels.i_lab@; @}
	|	labels IDENT ':'
		@{
			@i @labels.1.i_lab@ = @labels.0.i_lab@;
			@i @labels.0.s_lab@ = sym_add(@labels.1.s_lab@, @IDENT.name@, @IDENT.lineno@);
		@}
	;

stat	:	RETURN expr
		@{ @i @stat.s_lab@ = @stat.i_lab@; @}
	|	GOTO IDENT
		@{ 
			@i @stat.s_lab@ = @stat.i_lab@; 
			@check if (!sym_find(@stat.i_flabs@, @IDENT.name@)) {
				fprintf(stderr, "error: line %d: label '%s' not defined\n",
					@IDENT.lineno@,
					@IDENT.name@);
				ERROR_SEM;
			}
		@}
	|	IF expr THEN stats END
		@{ 
			@i @stats.i_lab@ = @stat.i_lab@; 
			@i @stat.s_lab@ = @stats.s_lab@;
		@}
	|	VAR IDENT '=' expr			/* Variablendefinition */
		@{ @i @stat.s_lab@ = @stat.i_lab@; @}
	|	lexpr '=' expr				/* Zuweisung */
		@{ @i @stat.s_lab@ = @stat.i_lab@; @}
	|	term
		@{ @i @stat.s_lab@ = @stat.i_lab@; @}
	;

lexpr	:	IDENT					/* schreibender Variablenzugriff */
	|	'*' unary				/* schreibender Speicherzugriff */
	;

expr	:	unary	
	|	plusterm
	|	multterm
	|	andterm
	|	term LTOREQ term
	|	term '#' term
	;

plusterm:	plusterm '+' term
	|	term '+' term
	;
	
multterm:	multterm '*' term
	|	term '*' term
	;
	
andterm	:	andterm AND term
	|	term AND term
	;

unary	:	NOT unary
	|	'-' unary
	|	'*' unary				/* lesender Speicherzugriff */
	|	term
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
	ERROR_SYN;
}

int yywrap()
{
	return 1;
}