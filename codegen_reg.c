/* codegen_reg.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "codegen.h"

static struct reg regs[] = {
	/* 64-bit-name, 8-bit-name, current status, pointer to variable symbol */
	
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

/* hints that the next associated register should be a 
 * specific register, if it is available */
static struct reg *hint = NULL;

/* number of registers */
#define REGNO (sizeof(regs) / sizeof(regs[0]))

/* loop to iterate over all registers */
#define ITERATE_REGS(reg) \
	int i; \
	for(i = 0, reg = &regs[0]; i < REGNO; i++, reg = &regs[i])

/* flag access macros */
#define CALLEE(reg)  ((reg)->status & regCALLEE)
#define TOUCHED(reg) ((reg)->status & regTOUCHED)
#define BUSY(reg)    ((reg)->status & regBUSY)

/* quasi-flag: all busy registers, that do not point
 * to a variable symbol, are temporary */
#define TEMP(reg)    (BUSY(reg) && !reg->symbol)

#define SET_STATE(reg, flag) ((reg)->status |= flag)
#define CLR_STATE(reg, flag) ((reg)->status &= ~flag)

struct reg *reg_alloc()
{
	/* try hint */
	if (hint && !BUSY(hint)) {
	
		out_verbose("Alloc HINT-REG %R", hint);
		
		reg_busy(hint);
		return hint;
	}

	/* return first free (non-busy) register */
	struct reg *r;
	ITERATE_REGS(r) {
	
		if (!BUSY(r)) {
		
			reg_busy(r);
			return r;
		}
	}
	
	fputs("compiler error: ran out of registers\n", stderr);
	ERROR_INT;
}

struct reg *reg_alloc_temp(int candidates, ...)
{
	/* try to re-use already-allocated temporary registers (=candidates) */
	va_list ap;
	va_start(ap, candidates);
	int i;
	
	struct reg *use_hint = NULL;
	struct reg *use_temp = NULL;
	
	for (i = 0; i < candidates;  i++) {
		struct reg *r = va_arg(ap, struct reg*);
		
		if (!r) continue;
		
		/* prefer hint regs */
		if (hint && TEMP(hint) && r == hint)
			use_hint = r;
		
		/* and then the first temporary candidate */
		if (!use_temp && TEMP(r))
			use_temp = r;
	}
	
	va_end(ap);
	
	if (use_hint) {
		out_verbose("Reuse HINT-REG %R", use_hint);
		return use_hint;
	}
	
	if (use_temp) {
		out_verbose("Reuse TEMP-REG %R", use_temp);
		return use_temp;
	}
	
	/* no re-use candidate - allocate new reg */
	return reg_alloc();
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
		
		reg_busy(r);
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
		CLR_STATE(r, regTOUCHED);
	}
	hint = NULL;
	
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
		
		/* compare by name string
		 * (sym struct could have been duplicated by sym_concat) 
		 */
		if (BUSY(r) && r->symbol && r->symbol->name == var->name)
			return r;
	}
	
	/* no register is associated yet - allocate a new one */
	r = reg_alloc();
	r->symbol = var;
	out_verbose("%s Assoc VAR %s: REG %R", reg_busy_graph(), var->name, r);
	return r;
}

void reg_free_vars(const struct sym *vars)
{
	const struct sym *v = vars;
	
	while (v) {

		/* find and free register associated to variable */
		struct reg *r;
		ITERATE_REGS(r) {
			
			/* compare by name string
			 * (sym struct could have been duplicated by sym_concat)
			 */
			if (BUSY(r) && r->symbol && r->symbol->name == v->name) {
			
				reg_free(r);
				out_verbose("%s Freed VAR %s (REG %R)", reg_busy_graph(), v->name, r);
				return;
			}
		}

		v = v->next;
	}
}

void reg_free(struct reg *r)
{
	assert(r);

	if (!BUSY(r))
		return;

	CLR_STATE(r, regBUSY);
	r->symbol = NULL;
}

void reg_busy(struct reg *r)
{
	assert(r);

	if (BUSY(r))
		return;

	SET_STATE(r, regBUSY);
	SET_STATE(r, regTOUCHED);
}

void reg_free_nonvar() 
{
	struct reg *r;
	ITERATE_REGS(r) {
	
		/* free all temp registers */
		if (TEMP(r))
			reg_free(r);
	}
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
	for(i = REGNO; i >= 0; --i) {
		
		r = &regs[i];
		if (CALLEE(r) && TOUCHED(r)) {
			out_instr("\tpopq\t%R", r);
			CLR_STATE(r, regTOUCHED);
		}
	}
}

struct reg *reg_result()
{
	return &regs[8];
}

struct reg *reg_argument(unsigned int n)
{
	if (n > 5) {
		fputs("compiler error: cannot handle more than 6 function call args", stderr);
		ERROR_INT;
	}

	return &regs[n];
}

void reg_hint(struct reg *r)
{
	assert(r);
	
	if (BUSY(r))
		out_verbose("W: HINT-REG %R already in use", r);
	
	hint = r;
}

char *reg_busy_graph()
{
	char *g = malloc(REGNO + 1);
	MALLOC_CHECK(g)
	
	int i;
	
	for(i = 0; i < REGNO; i++) {
		g[i] = (BUSY(&regs[i]) ? 'X' : '_');
	}
	
	g[REGNO] = '\0';
	
	return g;
}
