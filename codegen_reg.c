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
	{"%rdi", "%dil",  regCALLER, NULL},
	{"%rsi", "%sil",  regCALLER, NULL},
	{"%rdx", "%dl",   regCALLER, NULL},
	{"%rcx", "%cl",   regCALLER, NULL},
	{"%r8",  "%r8b",  regCALLER, NULL},
	{"%r9",  "%r9b",  regCALLER, NULL},
	{"%r10", "%r10b", regCALLER, NULL},
	{"%r11", "%r11b", regCALLER, NULL},
	{"%rax", "%al",   regCALLER, NULL},
	
	/* callee saved */
	{"%r12", "%r12b", regCALLEE, NULL},
	{"%r13", "%r13b", regCALLEE, NULL},
	{"%r14", "%r14b", regCALLEE, NULL},
	{"%r15", "%r15b", regCALLEE, NULL}
};

/* hints that the next associated register should be a 
 * specific register, if it is available */
static struct reg *hint = NULL;

/* number of available registers */
#define REGNO (sizeof(regs) / sizeof(regs[0]))

/* loops to iterate over all registers */
#define ITERATE_REGS(register, body) \
	{ \
		int i; \
		struct reg *register; \
		for(i = 0; i < REGNO; i++) { \
			register = &regs[i]; \
			body \
		} \
	}

#define ITERATE_REGS_REVERSE(register, body) \
	{ \
		int i; \
		struct reg *register; \
		for(i = REGNO; i >= 0; --i) { \
			register = &regs[i]; \
			body \
		} \
	}

/* flag access macros */
#define CALLEE(reg)  ((reg)->status & regCALLEE)
#define CALLER(reg)  ((reg)->status & regCALLER)
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
	ITERATE_REGS(r, {
	
		if (!BUSY(r)) {
		
			reg_busy(r);
			return r;
		}
	})
	
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
	ITERATE_REGS(r, {
		reg_free(r);
		CLR_STATE(r, regTOUCHED);
	})
	hint = NULL;
	
	/* assign parameter registers in reverse order
	   (first function parameters are last in symtable) */
	reg_alloc_param(par, 0);
}

struct reg *reg_get_var(struct sym *var, struct reg *candidate)
{
	assert(var);

	/* 1. find variable in registers */
	ITERATE_REGS(r, {
		
		/* compare by name string
		 * (sym struct could have been duplicated by sym_concat) 
		 */
		if (BUSY(r) && r->symbol && r->symbol->name == var->name)
			return r;
	})
	
	/* 2. find variable in stack */
	struct stackvar *s = stack_get_var(var);
	if (s) {
		struct reg *r = reg_alloc_temp(1, candidate);
		r->symbol = s->symbol;
		out_instr("\tmovq\t%S, %R", s, r);
		return r;
	}
	
	/* 3. no register is associated yet - allocate a new one */
	struct reg *r = reg_alloc_temp(1, candidate);
	r->symbol = var;
	out_verbose("%s Alloc VAR %s: REG %R", reg_busy_graph(), var->name, r);
	return r;
}

void reg_free_vars(const struct sym *vars)
{
	const struct sym *v = vars;
	
	while (v) {

		/* find and free register associated to variable */
		ITERATE_REGS(r, {
			
			/* compare by name string
			 * (sym struct could have been duplicated by sym_concat)
			 */
			if (BUSY(r) && r->symbol && r->symbol->name == v->name) {
			
				reg_free(r);
				out_verbose("%s Freed VAR %s (REG %R)", reg_busy_graph(), v->name, r);
				return;
			}
		})

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

void reg_relocate(struct reg *r)
{
	assert(r);
	
	if (!BUSY(r))
		return;
	
	/* alloc new */
	struct reg *new = reg_alloc();
	out_verbose("Relocate %R => %R", r, new);
	reg_move(r, new);
}

void reg_move(struct reg *from, struct reg *to)
{
	assert(from);
	assert(to);
	
	if (!BUSY(from))
		return;
	
	/* copy content */
	out_instr("\tmovq\t%R, %R", from, to);
	
	/* copy metadata */
	to->symbol = from->symbol;
	
	/* free old, busy new */
	reg_free(from);
	reg_busy(to);
}

void reg_free_nonvar() 
{
	ITERATE_REGS(r, {
	
		/* free all temp registers */
		if (TEMP(r))
			reg_free(r);
	})
}

struct instr *reg_save_callee(struct instr *function_header)
{
	struct instr *after = function_header;
	
	ITERATE_REGS(r, {
		if (CALLEE(r) && TOUCHED(r))
			after = out_instr_insert(after, "\tpushq\t%R", r);
	})
	
	return after;
}

void reg_restore_callee()
{
	/* iterate in reverse direction to pop saved values from stack */
	ITERATE_REGS_REVERSE(r, {
		if (CALLEE(r) && TOUCHED(r)) {
			out_instr("\tpopq\t%R", r);
			CLR_STATE(r, regTOUCHED);
		}
	})
}

const struct reg *reg_save_fcall()
{
	/* copy current caller register state to dump */
	struct reg *dump = calloc(REGNO, sizeof(*dump));
	MALLOC_CHECK(dump)
	memcpy(dump, regs, REGNO * sizeof(*dump));

	/* push regs to stack */
	ITERATE_REGS(r, {
		if (CALLER(r) && BUSY(r) && !TEMP(r)) {
			stack_push(r);
			reg_free(r);
		}
	})
	
	return dump;
}

void reg_restore_fcall(const struct reg *dump)
{
	/* restore caller reg state from dump */
	memcpy(regs, dump, REGNO * sizeof(*dump));
	
	/* iterate in reverse direction to pop saved values from stack */
	ITERATE_REGS_REVERSE(r, {
		if (CALLER(r) && BUSY(r) && !TEMP(r))
			stack_pop(r);
	})
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
	
	ITERATE_REGS(r, {
		g[i] = (BUSY(r) ? 'X' : '_');
	})
	
	g[REGNO] = '\0';
	
	return g;
}
