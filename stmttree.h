/* stmttree.h, Daniel Achleitner (0926807) */
#ifndef STMTTREE_H
#define STMTTREE_H

#include "exprtree.h"

/* statement tree, used for code generation */
enum stat_type {
	statRETURN,
	statGOTO,
	statIF,
	statVARDEF,
	statMEMASS
};

struct stat {
	enum stat_type type;   /* type of the statement */
	struct node *expr;     /* e.g. return value, if test, assignment value ... */
	
	union data {
		struct sym *label; /* goto label */
		struct stat *body; /* if statement body */
		struct node *adr;  /* memory assignment address */
		struct sym *var;   /* variable assignment */
	} data;
	
	struct stat *next;     /* pointer to next statement in list, or NULL */
};

/* methods to add new statements to list */
struct stat *stat_add_return(struct stat *list, struct node *expr);
struct stat *stat_add_goto(struct stat *list, struct sym *label);
struct stat *stat_add_if(struct stat *list, struct node *expr, struct stat *body);
struct stat *stat_add_vardef(struct stat *list, struct node *expr, struct sym *var);
struct stat *stat_add_memass(struct stat *list, struct node *expr, struct node *adr);

/* function list, used for code generation */
struct func {
	char *name;
	struct sym *pars;      /* parameter symbol table */
	struct stat *body;     /* function statement tree */
	
	struct func *next; /* pointer to next function in list, or NULL */
};

/* method to add new function to list */
struct func *func_add(struct func *list, char *name, struct sym *pars, struct stat *body);

#endif
