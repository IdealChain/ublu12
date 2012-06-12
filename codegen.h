/* codegen.h, Daniel Achleitner (0926807) */
#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include "symtable.h"
#include "exprtree.h"
#include "stmttree.h"
#include "output.h"


/* allocates a free register */
struct reg *reg_alloc();

/* allocates a free, temporary registers. supplied candidates are 
 * allocated registers that may be re-used. */
struct reg *reg_alloc_temp(int candidates, ...);

/* resets the current register state and assigns registers to function parameters */
void reg_new_function(struct sym *par);

/* gets a register associated to a variable; a new one will be allocated if
 * necessary. candidate can be a re-usable temporary register, or NULL. */
struct reg *reg_get_var(struct sym *var, struct reg *candidate);

/* frees all registers associated to a list of variables */
void reg_free_vars(const struct sym *vars);

/* returns a register (marks it as free again) */
void reg_free(struct reg *r);

/* marks a register as busy (non-free) */
void reg_busy(struct reg *r);

/* ensures that a register is available, moving its content */
void reg_relocate(struct reg *r);

/* moves the content of one register to another one */
void reg_move(struct reg *from, struct reg *to);

/* marks all non-variable (temporary) registers as free */
void reg_free_nonvar();

/* saves all callee saved registers that have been touched */
/* (is called after the function code is generated, but inserts its code after the function header) */
struct instr *reg_save_callee(struct instr *function_header);

/* restore all callee saved registers */
void reg_restore_callee();

/* saves all caller saved registers that are currently in use (before a function call) */
const struct reg *reg_save_fcall();

/* restores all previously saved caller saved registers (after a function call) */
void reg_restore_fcall(const struct reg *dump);

/* gets the function result register */
struct reg *reg_result();

/* gets the register that is used for function call argument number n */
struct reg *reg_argument(unsigned int n);

/* hints that the next associated register should be a specific register, if it is available */
void reg_hint(struct reg *r);

/* gets an ascii bar indicating the amount of currently busy registers */
char *reg_busy_graph();


/* resets the current stack state */
void stack_new_function();

/* reserves space on the stack */
struct instr *stack_reservespace(struct instr *function_header);

/* returns the reserved space */
void stack_returnspace();

/* pushes register content onto the stack */
struct stackvar *stack_push(struct reg *r);

/* pushes an immediate value onto the stack */
struct stackvar *stack_push_immediate(long value);

/* pops a stack value to a register */
void stack_pop(struct reg *r);

/* gets a stack struct associated to a variable. NULL if not found */
struct stackvar *stack_get_var(struct sym *var);


/* gets an asm label for a input label; will be generated if necessary */
struct label *label_get(struct sym *lab);

/* gets an implicitely created (anonymous) label, e.g. for an if statement */
struct label *label_anon();


/* the code generator functions */
void cg_function(char *name, struct sym *pars, struct sym *scope_vars, struct stat *body);
void cg_stat(struct stat *s);
void cg_stats(struct stat *ss, struct sym *scope_vars);

void cg_stat_return(struct stat *s);
void cg_stat_label(struct stat *s);
void cg_stat_goto(struct stat *s);
void cg_stat_if(struct stat *s);
void cg_stat_vardef(struct stat *s);
void cg_stat_memass(struct stat *s);
void cg_stat_eval(struct stat *s);

/* iburg called expression generators */
void cg_plus(struct node *b, struct node *l, struct node *r);
void cg_plus_con(struct node *b, struct node *l, long con);
void cg_mult(struct node *b, struct node *l, struct node *r);
void cg_mult_con(struct node *b, struct node *l, long con);
void cg_lea(struct node *b, struct node *base, struct node *index, long scale, long disp);
void cg_and(struct node *b, struct node *l, struct node *r);
void cg_and_con(struct node *b, struct node *l, long con);
void cg_not(struct node *b, struct node *l);
void cg_neg(struct node *b, struct node *l);
void cg_set_cc(struct node *b, struct node *l, struct node *r, char *cc);

void cg_const(struct node *b);
void cg_var(struct node *b);
void cg_radr(struct node *b, struct node *l);

void cg_function_call(struct node *b);

#endif
