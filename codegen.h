/* codegen.h, Daniel Achleitner (0926807) */
#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include "symtable.h"
#include "tree.h"

/* allocates a free register */
struct reg *reg_alloc();

/* resets the current register state and assigns registers to function parameters */
void reg_new_function(struct sym *par);

/* gets a register associated to a variable. if there isn't one yet, a new one is allocated */
struct reg *reg_get_var(struct sym *var);

/* returns a register (marks it as free again) */
void reg_free(struct reg *r);

/* marks all non-variable (temporary) registers as free.
   with one possible exception, if not NULL. */
void reg_free_nonvar(struct reg *except);

/* restore all callee saved registers */
void reg_restore_callee();

/* gets the function result register */
const struct reg *const reg_result();

void cg_program(struct func *fs);
void cg_function(struct func *f);
void cg_stat(struct stat *s);
int cg_stats(struct stat *ss);

void cg_stat_return(struct stat *s);
void cg_stat_goto(struct stat *s);
void cg_stat_if(struct stat *s);
void cg_stat_vardef(struct stat *s);
void cg_stat_memass(struct stat *s);

/* iburg called expression generators */
void cg_plus(struct node *b, struct node *l, struct node *r);
void cg_mult(struct node *b, struct node *l, struct node *r);
void cg_and(struct node *b, struct node *l, struct node *r);
void cg_not(struct node *b, struct node *l);
void cg_neg(struct node *b, struct node *l);
void cg_set_cc(struct node *b, struct node *l, struct node *r, char *cc);

void cg_const(struct node *b);
void cg_var(struct node *b);
void cg_radr(struct node *b, struct node *l);

#endif