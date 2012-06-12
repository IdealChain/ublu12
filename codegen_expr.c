/* codegen_expr.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "codegen.h"

void cg_plus(struct node *b, struct node *l, struct node *r)
{
	struct node *src = r;
	
	b->reg = reg_alloc_temp(2, l->reg, r->reg);
	
	if (b->reg == r->reg)
		src = l;
	else if (b->reg != l->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	
	out_instr("\taddq\t%R, %R", src->reg, b->reg);
}

void cg_plus_con(struct node *b, struct node *l, long con)
{
	b->reg = reg_alloc_temp(1, l->reg);
	
	if (b->reg != l->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	
	out_instr("\taddq\t%I, %R", con, b->reg);
}

void cg_mult(struct node *b, struct node *l, struct node *r)
{
	struct node *src = r;
	
	b->reg = reg_alloc_temp(2, l->reg, r->reg);
	
	if (b->reg == r->reg)
		src = l;
	else if (b->reg != l->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	
	out_instr("\timulq\t%R, %R", src->reg, b->reg);
}

void cg_mult_con(struct node *b, struct node *l, long con)
{
	if (con == 1) {
		b->reg = l->reg;
		return;
	}

	b->reg = reg_alloc_temp(1, l->reg);
	
	if (b->reg != l->reg)
		out_instr("\timulq\t%I, %R, %R", con, l->reg, b->reg);
	else
		out_instr("\timulq\t%I, %R", con, b->reg);
}

void cg_lea(struct node *b, struct node *base, struct node *index, long scale, long disp)
{
	assert(b);
	assert(scale == 1 || scale == 2 || scale == 4 || scale == 8);
	
	if (!base && index && scale == 1 && !disp) {
		b->reg = index->reg;
		return;
	}
	
	b->reg = reg_alloc_temp(2, base ? base->reg : NULL, index ? index->reg : NULL);
	
	if (base && index)
		out_instr("\tleaq\t%ld(%R,%R,%ld), %R", disp, base->reg, index->reg, scale, b->reg);
	else if (base)
		out_instr("\tleaq\t%ld(%R), %R", disp, base->reg, b->reg);
	else if (index)
		out_instr("\tleaq\t%ld(,%R,%ld), %R", disp, index->reg, scale, b->reg);
	else
		assert(0);
	
}

void cg_and(struct node *b, struct node *l, struct node *r)
{
	struct node *src = r;
	
	b->reg = reg_alloc_temp(2, l->reg, r->reg);
	
	if (b->reg == r->reg)
		src = l;
	else if (b->reg != l->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	
	out_instr("\tandq\t%R, %R", src->reg, b->reg);
}

void cg_and_con(struct node *b, struct node *l, long con)
{
	b->reg = reg_alloc_temp(1, l->reg);
	
	if (l->reg != b->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
		
	out_instr("\tandq\t%I, %R", con, b->reg);
}

void cg_not(struct node *b, struct node *l)
{
	b->reg = reg_alloc_temp(1, l->reg);
	
	if (l->reg != b->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
		
	out_instr("\tnotq\t%R", b->reg);
}

void cg_neg(struct node *b, struct node *l)
{
	b->reg = reg_alloc_temp(1, l->reg);
	
	if (l->reg != b->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
		
	out_instr("\tnegq\t%R", b->reg);
}

void cg_set_cc(struct node *b, struct node *l, struct node *r, char *cc)
{
	b->reg = reg_alloc(); /* re-use of temp registers is not easily possible here (target reg clear invalidates flags) */
	
	out_instr("\txor\t%R, %R", b->reg, b->reg); /* clear full register with xor */
	out_instr("\tcmp\t%R, %R", r->reg, l->reg);
	out_instr("\tset%s\t%B", cc, b->reg); /* set byte-register to flag */
}

void cg_const(struct node *b)
{
	b->reg = reg_alloc();
	out_instr("\tmovq\t%I, %R", b->value, b->reg);
}

void cg_var(struct node *b)
{
	/* check if var is in stack */
	struct stackvar *s = stack_get_var(b->symbol);
	if (s) {
	
		b->reg = reg_alloc();
		out_instr("\tmovq\t%ld(%s), %R", s->disp, "%rbp", b->reg);
		
	} else {
	
		b->reg = reg_get_var(b->symbol, NULL);
	}
	
	/* subtract this reference from the remaining ref. count */
	b->symbol->refcount--;
}

void cg_radr(struct node *b, struct node *l)
{
	b->reg = reg_alloc_temp(1, l->reg);
	out_instr("\tmovq\t(%R), %R", l->reg, b->reg);
}

void cg_function_call(struct node *b)
{
	assert(b);
	assert(b->symbol);
	
	out_verbose("FCALL: '%s'", b->symbol->name);
	
	/* push caller-saved registers on stack
	 * (make sure return register is free, so that it doesn't get overwritten afterwards) */
	reg_relocate(reg_result());
	const struct reg *rdump = reg_save_fcall();
	
	/* evaluate parameters */
	int i = 0;
	struct param *p = b->params;
	
	while (p) {
		
		BURM(p->expr);
		
		if (!p->expr->reg)
			stack_push_immediate(p->expr->value);
		else
			stack_push(p->expr->reg);
		
		i++;
		p = p->next;
	}
	
	/* before call: pop args back */
	while (i) {
		i--;
		struct reg *r = reg_argument(i);
		stack_pop(r);
	}
	
	/* call function */
	out_instr("\tcall\t%s", b->symbol->name);
	
	/* restore caller-saved registers on stack */
	reg_restore_fcall(rdump);
	
	/* return value is in result register */
	b->reg = reg_result();
	reg_busy(b->reg);
}
