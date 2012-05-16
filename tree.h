/* tree.h, Daniel Achleitner (0926807) */
#ifndef TREE_H
#define TREE_H

#include "common.h"
#include "symtable.h"

/* enum values below must be kept in sync with matcher.bfe */
enum operation {
	opCONST = 1,
	opVAR,
	opPLUS,
	opMULT,
	opAND,
	opNOT,
	opNEG,
	opLTOREQ,
	opNEQ,
	opRADR
};

enum regstatus {
	regCALLEE = 1 << 0,  /* flag indicating whether this is a callee saved register */
	regSAVED  = 1 << 1,  /* flag indicating whether this callee saved register has been saved to stack */
	regBUSY   = 1 << 2   /* flag indiciating whether this register is currently in use */
};

/* register state */
struct reg {
	char *name;             /* register name, e.g. %rax */
	char *name_b;           /* byte variant of register name, e.g. %al */
	enum regstatus status;  /* current register status, see above */
	struct sym *symbol;     /* associated variable symbol or NULL, if register is not used or temporary */
};

/* convenience macros */
#define REG(p)  (p)->reg->name
#define REGB(p) (p)->reg->name_b

#define BURM(p) burm_label(p); \
                burm_reduce(p, 1);

/* iburg operation tree node */
struct node {
	enum operation op;
	struct node *kids[2];
	struct burm_state *state;

	/* attributes of node: */
	struct reg *reg;    /* result register */
	
	long value;         /* CONST nodes */
	struct sym *symbol; /* VAR nodes */
};

/* type and access macros for iburg */
typedef struct node *NODEPTR_TYPE;

#define OP_LABEL(p)     ((p)->op)
#define LEFT_CHILD(p)   ((p)->kids[0])
#define RIGHT_CHILD(p)  ((p)->kids[1])
#define STATE_LABEL(p)  ((p)->state)
#define PANIC           printf

/* forward declaration (extern) */
struct burm_state *burm_label(struct node *bnode);
void burm_reduce(struct node *bnode, int goalnt);

/* generate tree node for iburg */
struct node* gen_node(int op, struct node *left, struct node *right, long value, struct sym* symbol);

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