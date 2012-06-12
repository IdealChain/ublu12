/* exprtree.h, Daniel Achleitner (0926807) */
#ifndef EXPRTREE_H
#define EXPRTREE_H

#include "common.h"
#include "symtable.h"

/* enum values below must be kept in sync with matcher.bfe */
enum operation {
	opCONST = 1,
	opLEACONST,
	opVAR,
	opPLUS,
	opMULT,
	opAND,
	opNOT,
	opNEG,
	opLTOREQ,
	opNEQ,
	opRADR,
	opCALL
};

enum regstatus {
	regCALLEE  = 1 << 0, /* flag: this is a callee saved register */
	regCALLER  = 1 << 1, /* flag: this is a caller saved register */
	regTOUCHED = 1 << 2, /* flag: this register has been touched (and must be saved & restored) */
	regBUSY    = 1 << 3  /* flag: this register is currently in use */
};

/* register state */
struct reg {
	char *name;            /* register name, e.g. %rax */
	char *name_b;          /* byte variant of register name, e.g. %al */
	enum regstatus status; /* current register status, see above */
	struct sym *symbol;    /* associated variable symbol or NULL, if register is not used or temporary */
};

/* variable on stack */
struct stackvar {
	long disp;             /* displacement from %rbp register */
	struct sym *symbol;    /* associated variable symbol */
	
	struct stackvar *next; /* next element in linked list */
};

/* list of generated assembler labels */
struct label {
	unsigned int nr;    /* label number in outputted asm code (unique) */
	struct sym *symbol; /* input code label, or NULL, if its generated implicitely */
	
	struct label *next; /* next element in linked list */
};

/* forward declaration */
struct node;

/* parameter expressions for a function call */
struct param {
	struct node *expr;  /* expression for this parameter */
	struct param *next; /* next element in linked list */
};

/* iburg operation tree node */
struct node {
	enum operation op;
	struct node *kids[2];
	struct burm_state *state;

	/* attributes of node: */
	struct reg *reg;      /* result register */
	
	long value;           /* CONST nodes */
	struct sym *symbol;   /* VAR nodes */
	struct param *params; /* function call parameters */
};

/* type and access macros for iburg */
typedef struct node *NODEPTR_TYPE;

#define OP_LABEL(p)     ((p)->op)
#define LEFT_CHILD(p)   ((p)->kids[0])
#define RIGHT_CHILD(p)  ((p)->kids[1])
#define STATE_LABEL(p)  ((p)->state)
#define PANIC           printf

/* convenience macros */
#define BURM(p)       burm(p, NULL);
#define BURM_REG(p,r) burm(p, r);

#define KIDS(p)      (node_nav(bnode, p))

/* labels and reduces the burm expression expr. if target is given, 
 * the result is guaranteed to be in that register, even if it is 
 * already in use.
 */
void burm(struct node *expr, struct reg *target);

/* generate tree node for iburg */
struct node *gen_node(int op, struct node *left, struct node *right, long value, struct sym* symbol);

/* generate function call node */
struct node *gen_call_node(char *function, unsigned int lineno, struct param *params);

/* add a parameter to the list */
struct param *add_param(struct param *list, struct node *expr);

/* helper function for navigating through expression tree (leaf-ward).
 * b = start node, path = (l|r)+ 
 */
struct node* node_nav(struct node *b, char *path);

#endif
