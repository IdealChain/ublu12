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
	opRADR
};

enum regstatus {
	regCALLEE  = 1 << 0,  /* flag indicating whether this is a callee saved register */
	regTOUCHED = 1 << 1,  /* flag indicating whether this callee saved register has been touched (and must be saved & restored) */
	regBUSY    = 1 << 2   /* flag indiciating whether this register is currently in use */
};

/* register state */
struct reg {
	char *name;             /* register name, e.g. %rax */
	char *name_b;           /* byte variant of register name, e.g. %al */
	enum regstatus status;  /* current register status, see above */
	struct sym *symbol;     /* associated variable symbol or NULL, if register is not used or temporary */
};

/* list of generated assembler labels */
struct label {
	unsigned int nr;        /* label number in outputted asm code (unique) */
	struct sym *symbol;     /* input code label, or NULL, if its generated implicitely */
	
	struct label *next;     /* next element in linked list */
};

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

/* convenience macros */
#define BURM(p) burm_label(p); \
                burm_reduce(p, 1);

#define KIDS(p) (node_nav(bnode, p))

/* forward declaration (extern) */
struct burm_state *burm_label(struct node *bnode);
void burm_reduce(struct node *bnode, int goalnt);

/* generate tree node for iburg */
struct node* gen_node(int op, struct node *left, struct node *right, long value, struct sym* symbol);

/* helper function for navigating through expression tree (leaf-ward). b = start node, path = (l|r)+ */
struct node* node_nav(struct node *b, char *path);

#endif
