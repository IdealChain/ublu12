/* codegen_reg.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "codegen.h"

static struct reg regs[] = {
	/* 64-bit-name, 8-bit-name, busy-flag, pointer to variable symbol */
	
	/* caller saved */
	{"%rdi", "%dil",  0,         NULL},
	{"%rsi", "%sil",  0,         NULL},
	{"%rdx", "%dl",   0,         NULL},
	{"%rcx", "%cl",   0,         NULL},
	{"%r8",  "%r8b",  0,         NULL},
	{"%r9",  "%r9b",  0,         NULL},
	{"%r10", "%r10b", 0,         NULL},
	{"%r11", "%r11b", 0,         NULL},
	{"%rax", "%al",   0,         NULL},
	
	/* callee saved */
	{"%rbp", "%bpl",  regCALLEE, NULL},
	{"%r12", "%r12b", regCALLEE, NULL},
	{"%r13", "%r13b", regCALLEE, NULL},
	{"%r14", "%r14b", regCALLEE, NULL},
	{"%r15", "%r15b", regCALLEE, NULL}
};

/* loop to iterate over all registers */
#define ITERATE_REGS(reg) \
	int i; \
	for(i = 0, reg = &regs[0]; i < sizeof(regs) / sizeof(regs[0]); i++, reg = &regs[i])

/* flag access macros */
#define CALLEE(reg)  ((reg)->status & regCALLEE)
#define TOUCHED(reg) ((reg)->status & regTOUCHED)
#define BUSY(reg)    ((reg)->status & regBUSY)

#define SET_STATE(reg, flag) ((reg)->status |= flag)
#define CLR_STATE(reg, flag) ((reg)->status &= ~flag)

struct reg *reg_alloc()
{
	/* return first free (non-busy) register */
	struct reg *r;
	ITERATE_REGS(r) {
	
		if (!BUSY(r)) {
		
			if (CALLEE(r) && !TOUCHED(r))
				SET_STATE(r, regTOUCHED);
			
			SET_STATE(r, regBUSY);
			return r;
		}
	}
	
	fputs("compiler error: ran out of registers\n", stderr);
	ERROR_INT;
}

int reg_alloc_param(struct sym *par, int i) 
{
	/* recursive function that assigns the parameter registers in
	   reverse order (first function parameters are last in symtable) */
	
	if (!par)
		return i;
	
	int count = reg_alloc_param(par->next, i + 1);
	
	/* only used parameters need a register allocation */
	if (par->refcount > 0) {
		struct reg *r = &regs[(count - 1) - i];
		
		SET_STATE(r, regBUSY);
		r->symbol = par;
	}
	
	return count;
}

void reg_new_function(struct sym *par)
{
	/* reset complete register state */
	struct reg *r;
	ITERATE_REGS(r) {
		reg_free(r);
	}
	
	/* assign parameter registers in reverse order
	   (first function parameters are last in symtable) */
	reg_alloc_param(par, 0);
}

struct reg *reg_get_var(struct sym *var)
{
	assert(var);

	/* find register associated to variable */
	struct reg *r;
	ITERATE_REGS(r) {
		if (BUSY(r) && r->symbol && r->symbol == var)
			return r;
	}
	
	/* no register is associated yet - allocate a new one */
	r = reg_alloc();
	r->symbol = var;
	return r;
}

void reg_free(struct reg *r)
{
	assert(r);

	CLR_STATE(r, regBUSY);
	r->symbol = NULL;
}

void reg_free_nonvar(struct reg *except) 
{
	struct reg *r;
	ITERATE_REGS(r) {
		
		if (except && r == except)
			continue;
		
		/* all busy registers that do not point to a variable symbol are temporary */
		if (BUSY(r) && !r->symbol)
			reg_free(r);
	}
}

int reg_istemp(struct reg *r)
{
	assert(r);
	
	return (BUSY(r) && !r->symbol);
}

void reg_save_callee(struct instr *function_header)
{
	struct instr *after = function_header;
	struct reg *r;
	
	ITERATE_REGS(r) {
		if (CALLEE(r) && TOUCHED(r))
			after = out_instr_insert(after, "\tpushq\t%R", r);
	}
}

void reg_restore_callee()
{
	int i;
	struct reg *r;
	/* iterate in reverse direction to pop saved values from stack */
	for(i = sizeof(regs) / sizeof(regs[0]); i >= 0; --i) {
		
		r = &regs[i];
		if (CALLEE(r) && TOUCHED(r)) {
			out_instr("\tpopq\t%R", r);
			CLR_STATE(r, regTOUCHED);
		}
	}
}

const struct reg *const reg_result()
{
	return &regs[8];
}

char *reg_busy_graph()
{
	char *g = malloc(sizeof(regs) / sizeof(regs[0]) + 1);
	MALLOC_CHECK(g)
	
	int i;
	
	for(i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
		g[i] = (BUSY(&regs[i]) ? 'X' : '_');
	}
	
	g[14] = '\0';
	
	return g;
}
