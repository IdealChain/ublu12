/* parser, Daniel Achleitner (0926807) */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "symtable.h"
#include "exprtree.h"
#include "stmttree.h"
#include "codegen.h"

int yylex(void);
void yyerror(const char *str);

/* ensures that a used symbol is defined - the ref counter will be increased according to the the inc_ref value */
void check_defined(struct sym *table, const char *symbol, int line, int inc_ref);

/* ensures that no collisions between variable names and label names occur
   within the respective scope */
void check_collisions(struct sym *vars, struct sym *labels);

%}

%start start

%token END	RETURN	GOTO	IF
%token THEN	VAR	NOT	AND
%token LTOREQ	NUM	IDENT

/* lexer synthesized attributes */
@attributes { char *name; int lineno; } IDENT
@attributes { unsigned long value;    } NUM
@attributes { int lineno;             } IF

/* grammar attribute definition

   'i_fs', 's_fs', 'i_ss' and 's_ss' are the inherited and synthesized incomplete lists 
   that are used to collect all functions and statements to generate code for.
  
   'i_labs', 'i_vars', 's_labs' and 's_vars' are the inherited and synthesized incomplete
   symbol tables that are used to collect all label and variable declarations in the 
   respective scope.
   
   'n' is a pointer to the tree node for iburg.
   
   'i_flabs' and 'i_fvars' are the auto-inherited, complete symbol tables containing all
   labels and variables visible in the respective scope. */
@autoinh i_flabs i_fvars
@autosyn n
@attributes { struct func *i_fs;  struct func *s_fs;                                           } program funcdef
@attributes { struct sym *i_labs; struct sym *s_labs; struct sym *i_flabs; 
              struct sym *i_vars; struct sym *s_vars; struct sym *i_fvars;                 
              struct stat *i_ss;  struct stat *s_ss;                                           } stats stat
@attributes { struct sym *i_labs; struct sym *s_labs;
              struct stat *i_ss;  struct stat *s_ss;  struct sym *i_flabs;                     } labels
@attributes { struct sym *i_vars; struct sym *s_vars;                                          } parlist
@attributes { struct stat *i_ss;  struct stat *s_ss;  struct sym *i_fvars; struct node *rexpr; } lexpr
@attributes {                                         struct sym *i_fvars;                     } exprlist
@attributes {                                         struct sym *i_fvars; struct node *n;     } expr plusterm multterm andterm unary term



/* checks whether referenced labels/parameters/variables exist */
@traversal @preorder check

/* prints symbol table debug information */
@traversal @preorder print

/* generates the program code */
@traversal @preorder codegen

%%

start	:	program
		@{ 
			@i @program.i_fs@ = NULL; 
			@codegen cg_program(@program.s_fs@); 
		@}
	;

program	:	/* empty */
		@{ @i @program.s_fs@ = @program.i_fs@; @}
	|	program funcdef ';'
		@{
			@i @program.1.i_fs@ = @program.0.i_fs@;
			@i @funcdef.i_fs@ = @program.1.s_fs@;
			@i @program.0.s_fs@ = @funcdef.s_fs@;
		@}
	;

funcdef	:	IDENT '(' parlist ')' stats END		/* Funktionsdefinition */
		@{
			/* labels */
			@i @stats.i_labs@ = NULL;
			@i @stats.i_flabs@ = @stats.s_labs@;
			@print sym_print(@stats.s_labs@, "labels", @IDENT.name@);
			
			/* parameters / variables */
			@i @parlist.i_vars@ = NULL;
			@i @stats.i_vars@ = @parlist.s_vars@;
			@i @stats.i_fvars@ = @stats.s_vars@;
			@print sym_print(@stats.s_vars@, "variables", @IDENT.name@);
			
			/* add function to list */
			@i @stats.i_ss@ = NULL; 
			@i @funcdef.s_fs@ = func_add(@funcdef.i_fs@, @IDENT.name@, @parlist.s_vars@, @stats.s_ss@);
		@}
	;	

parlist	:	/* empty */
		@{ @i @parlist.s_vars@ = @parlist.i_vars@; @}
	|	IDENT
		@{ @i @parlist.s_vars@ = sym_add(@parlist.i_vars@, @IDENT.name@, @IDENT.lineno@); @}
	|	IDENT ',' parlist
		@{
			@i @parlist.1.i_vars@ = sym_add(@parlist.0.i_vars@, @IDENT.name@, @IDENT.lineno@);
			@i @parlist.0.s_vars@ = @parlist.1.s_vars@;
		@}
	;

stats	:	/* empty */
		@{ 
			@i @stats.s_labs@ = @stats.i_labs@; 
			@i @stats.s_vars@ = @stats.i_vars@;
			@i @stats.s_ss@ = @stats.i_ss@;
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
			
			/* statements */
			@i @stats.1.i_ss@ = @stats.0.i_ss@;
			@i @labels.i_ss@ = @stats.1.s_ss@;
			@i @stat.i_ss@ = @labels.s_ss@;
			@i @stats.0.s_ss@ = @stat.s_ss@;
			
			/* check for variable / label name collisions */
			@check check_collisions(@stats.0.i_fvars@, @stats.0.i_flabs@);
		@}
	;

labels	:	/* empty */
		@{ 
			@i @labels.s_labs@ = @labels.i_labs@; 
			@i @labels.s_ss@ = @labels.i_ss@;
		@}
	|	labels IDENT ':'
		@{
			/* label symbol */
			@i @labels.1.i_labs@ = @labels.0.i_labs@;
			@i @labels.0.s_labs@ = sym_add(@labels.1.s_labs@, @IDENT.name@, @IDENT.lineno@);
			
			/* label statement */
			@i @labels.1.i_ss@ = @labels.0.i_ss@;
			@i @labels.0.s_ss@ = stat_add_label(@labels.1.s_ss@, sym_find(@labels.0.i_flabs@, @IDENT.name@));
		@}
	;

stat	:	RETURN expr
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@i @stat.s_vars@ = @stat.i_vars@;
			@i @stat.s_ss@ = stat_add_return(@stat.i_ss@, @expr.n@);
		@}
	|	GOTO IDENT
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@check check_defined(@stat.i_flabs@, @IDENT.name@, @IDENT.lineno@, 1);
			@i @stat.s_vars@ = @stat.i_vars@;
			@i @stat.s_ss@ = stat_add_goto(@stat.i_ss@, sym_find(@stat.i_flabs@, @IDENT.name@));
		@}
	|	IF expr THEN stats END
		@{ 
			/* labels */
			@i @stats.i_labs@ = @stat.i_labs@; 
			@i @stat.s_labs@ = @stats.s_labs@;
			
			/*  parameters / variables */
			@i @stat.s_vars@ = @stat.i_vars@;
			@i @stats.i_vars@ = @stat.i_fvars@;
			@i @stats.i_fvars@ = @stats.s_vars@;
			@print { 
				char subject[20];
				snprintf(subject, sizeof(subject), "if body [line %2d]", @IF.lineno@);
				sym_print(@stats.s_vars@, "variables", subject);
			}
			
			/* statements */
			@i @stats.i_ss@ = NULL;
			@i @stat.s_ss@ = stat_add_if(@stat.i_ss@, @expr.n@, @stats.s_ss@);
		@}
	|	VAR IDENT '=' expr			/* Variablendefinition */
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@i @stat.s_vars@ = sym_add(@stat.i_vars@, @IDENT.name@, @IDENT.lineno@);
			@i @stat.s_ss@ = stat_add_vardef(@stat.i_ss@, @expr.n@, sym_find(@stat.i_fvars@, @IDENT.name@));
		@}
	|	lexpr '=' expr				/* Zuweisung */
		@{ 
			@i @stat.s_labs@ = @stat.i_labs@; 
			@i @stat.s_vars@ = @stat.i_vars@;
			
			@i @lexpr.i_ss@ = @stat.i_ss@;
			@i @stat.s_ss@ = @lexpr.s_ss@;
			
			@i @lexpr.rexpr@ = @expr.n@;
		@}
	|	term
		@{ 
			/* labels / parameters / variables */
			@i @stat.s_labs@ = @stat.i_labs@; 
			@i @stat.s_vars@ = @stat.i_vars@;
			
			/* statements: add term evaluation statement */
			@i @stat.s_ss@ = stat_add_eval(@stat.i_ss@, @term.n@);
		@}
	;

lexpr	:	IDENT					/* schreibender Variablenzugriff */
		@{
			@check check_defined(@lexpr.i_fvars@, @IDENT.name@, @IDENT.lineno@, 0);
			@i @lexpr.s_ss@ = stat_add_vardef(@lexpr.i_ss@, @lexpr.rexpr@, sym_find(@lexpr.i_fvars@, @IDENT.name@));
		@}
	|	'*' unary				/* schreibender Speicherzugriff */
		@{ 
			@i @lexpr.s_ss@ = stat_add_memass(@lexpr.i_ss@, @lexpr.rexpr@, @unary.n@); 
		@}
	;

expr	:	unary
	|	plusterm
	|	multterm
	|	andterm
	|	term LTOREQ term
		@{ @i @expr.n@ = gen_node(opLTOREQ, @term.0.n@, @term.1.n@, 0, NULL); @}
	|	term '#' term
		@{ @i @expr.n@ = gen_node(opNEQ, @term.0.n@, @term.1.n@, 0, NULL); @}
	;

plusterm:	plusterm '+' term
		@{ @i @plusterm.0.n@ = gen_node(opPLUS, @plusterm.1.n@, @term.n@, 0, NULL); @}
	|	term '+' term
		@{ @i @plusterm.n@ = gen_node(opPLUS, @term.0.n@, @term.1.n@, 0, NULL); @}
	;
	
multterm:	multterm '*' term
		@{ @i @multterm.0.n@ = gen_node(opMULT, @multterm.1.n@, @term.n@, 0, NULL); @}
	|	term '*' term
		@{ @i @multterm.n@ = gen_node(opMULT, @term.0.n@, @term.1.n@, 0, NULL); @}
	;
	
andterm	:	andterm AND term
		@{ @i @andterm.0.n@ = gen_node(opAND, @andterm.1.n@, @term.n@, 0, NULL); @}
	|	term AND term
		@{ @i @andterm.n@ = gen_node(opAND, @term.0.n@, @term.1.n@, 0, NULL); @}
	;

unary	:	NOT unary
		@{ @i @unary.0.n@ = gen_node(opNOT, @unary.1.n@, NULL, 0, NULL); @}
	|	'-' unary
		@{ @i @unary.0.n@ = gen_node(opNEG, @unary.1.n@, NULL, 0, NULL); @}
	|	'*' unary				/* lesender Speicherzugriff */
		@{ @i @unary.0.n@ = gen_node(opRADR, @unary.1.n@, NULL, 0, NULL); @}
	|	term
	;

term	:	'(' expr ')'
	|	NUM
		@{ @i @term.n@ = gen_node(opCONST, NULL, NULL, @NUM.value@, NULL); @}
	|	IDENT					/* Variablenverwendung */
		@{
			@check check_defined(@term.i_fvars@, @IDENT.name@, @IDENT.lineno@, 1);
			@i @term.n@ = gen_node(opVAR, NULL, NULL, 0, sym_find(@term.i_fvars@, @IDENT.name@));
		@}
	|	IDENT '(' exprlist ')'			/* Funktionsaufruf */
		@{ @i @term.n@ = NULL; @}
	;

exprlist:	/* empty */
	|	expr
	|	expr ',' exprlist
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

void check_defined(struct sym *table, const char *symbol, int line, int inc_ref)
{
	struct sym *s = sym_find(table, symbol);
	if (!s) {
		fprintf(stderr, "error: line %d: label or variable '%s' not defined\n",
			line,
			symbol);
		ERROR_SEM;
	}
	s->refcount += inc_ref;
}

void check_collisions(struct sym *vars, struct sym *labels)
{
	struct sym *s = vars;
	
	while (s) {
		struct sym *l = NULL;
		
		if ((l = sym_find(labels, s->name))) {
			fprintf(stderr, "error: variable '%s' (line %d) collides with label '%s' (line %d)\n",
				s->name,
				s->line,
				l->name,
				l->line);
			ERROR_SEM;
		}
		
		s = s->next;
	}
}
