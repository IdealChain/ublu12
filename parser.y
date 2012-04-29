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

/* grammar attribute definition
   'i_labs', 'i_vars', 's_labs' and 's_vars' are the inherited and synthesized incomplete
   symbol tables that are used to collect all label and variable declarations in the 
   respective scope.
   'i_flabs' and 'i_fvars' are the auto-inherited, complete symbol tables containing all
   labels and variables visible in the respective scope. */
@autoinh i_flabs i_fvars
@attributes { struct sym *i_labs; struct sym *s_labs; struct sym *i_flabs; 
              struct sym *i_vars; struct sym *s_vars; struct sym *i_fvars; } stats stat
@attributes { struct sym *i_labs; struct sym *s_labs;                      } labels
@attributes { struct sym *i_vars; struct sym *s_vars;                      } pars lastpar 
@attributes {                                         struct sym *i_fvars; } lexpr expr plusterm multterm andterm unary term exprs lastexpr

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
			/* labels */
			@i @stats.i_labs@ = NULL;
			@i @stats.i_flabs@ = @stats.s_labs@;
			@print sym_print(@stats.s_labs@, "labels", @IDENT.name@);
			
			/* parameters / variables */
			@i @pars.i_vars@ = NULL;
			@i @lastpar.i_vars@ = @pars.s_vars@;
			@i @stats.i_vars@ = @lastpar.s_vars@;
			@i @stats.i_fvars@ = @stats.s_vars@;
			@print sym_print(@stats.s_vars@, "variables", @IDENT.name@);
		@}
	;	

pars	:	/* empty */				/* Parameterdefinition */
		@{ @i @pars.s_vars@ = @pars.i_vars@; @}
	|	pars IDENT ','
		@{ 
			@i @pars.1.i_vars@ = @pars.0.i_vars@; 
			@i @pars.0.s_vars@ = sym_add(@pars.1.s_vars@, @IDENT.name@, @IDENT.lineno@);
		@}
	;

lastpar	:	/* empty */
		@{ @i @lastpar.s_vars@ = @lastpar.i_vars@; @}
	|	IDENT
		@{ @i @lastpar.s_vars@ = sym_add(@lastpar.i_vars@, @IDENT.name@, @IDENT.lineno@); @}
	;

stats	:	/* empty */
		@{ 
			@i @stats.s_labs@ = @stats.i_labs@; 
			@i @stats.s_vars@ = @stats.i_vars@;
		@}
	|	stats labels stat ';'
		@{
			/* labels */
			@i @stats.1.i_labs@ = @stats.0.i_labs@;
			@i @labels.i_labs@ = @stats.1.s_labs@;
			@i @stat.i_labs@ = @labels.s_labs@;
			@i @stats.0.s_labs@ = @stat.s_labs@;
			
			/* parameters / variables */
			@i @stats.1.i_vars@ = @stats.0.i_vars@;
			@i @stat.i_vars@ = @stats.1.s_vars@;
			@i @stats.0.s_vars@ = @stat.s_vars@;
			
			/* check for variable / label name collisions */
			@check 
			struct sym *s = @stats.0.i_fvars@;
			while (s) {
				struct sym *l = NULL;
				if ((l = sym_find(@stats.0.i_flabs@, s->name))) {
					fprintf(stderr, "error: variable '%s' (line %d) collides with label '%s' (line %d)\n",
						s->name,
						s->line,
						l->name,
						l->line);
					ERROR_SEM;
				}
				s = s->next;
			}
		@}
	;

labels	:	/* empty */
		@{ @i @labels.s_labs@ = @labels.i_labs@; @}
	|	labels IDENT ':'
		@{
			@i @labels.1.i_labs@ = @labels.0.i_labs@;
			@i @labels.0.s_labs@ = sym_add(@labels.1.s_labs@, @IDENT.name@, @IDENT.lineno@);
		@}
	;

stat	:	RETURN expr
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@i @stat.s_vars@ = @stat.i_vars@;
		@}
	|	GOTO IDENT
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@check if (!sym_find(@stat.i_flabs@, @IDENT.name@)) {
				fprintf(stderr, "error: line %d: label '%s' not defined\n",
					@IDENT.lineno@,
					@IDENT.name@);
				ERROR_SEM;
			}
			@i @stat.s_vars@ = @stat.i_vars@;
		@}
	|	IF expr THEN stats END
		@{ 
			@i @stats.i_labs@ = @stat.i_labs@; 
			@i @stat.s_labs@ = @stats.s_labs@;
			
			@i @stat.s_vars@ = @stat.i_vars@;
			@i @stats.i_vars@ = @stat.i_fvars@;
			@i @stats.i_fvars@ = @stats.s_vars@;
			@print sym_print(@stats.s_vars@, "variables", "if body");
		@}
	|	VAR IDENT '=' expr			/* Variablendefinition */
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@i @stat.s_vars@ = sym_add(@stat.i_vars@, @IDENT.name@, @IDENT.lineno@);
		@}
	|	lexpr '=' expr				/* Zuweisung */
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@i @stat.s_vars@ = @stat.i_vars@;
		@}
	|	term
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@i @stat.s_vars@ = @stat.i_vars@;
		@}
	;

lexpr	:	IDENT					/* schreibender Variablenzugriff */
		@{
			@check if (!sym_find(@lexpr.i_fvars@, @IDENT.name@)) {
				fprintf(stderr, "error: line %d: variable '%s' not defined\n",
					@IDENT.lineno@,
					@IDENT.name@);
				ERROR_SEM;
			}
		@}
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
		@{
			@check if (!sym_find(@term.i_fvars@, @IDENT.name@)) {
				fprintf(stderr, "error: line %d: variable '%s' not defined\n",
					@IDENT.lineno@,
					@IDENT.name@);
				ERROR_SEM;
			}
		@}
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
