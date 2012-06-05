/* codegen_expr.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "codegen.h"

void cg_plus(struct node *b, struct node *l, struct node *r)
{
	struct node *src = r;
	
	if (reg_istemp(l->reg)) {
	
		b->reg = l->reg;
		
	} else if (reg_istemp(r->reg)) {
	
		b->reg = r->reg;
		src = l;
		
	} else {
	
		b->reg = reg_alloc();
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	}
	
	out_instr("\taddq\t%R, %R", src->reg, b->reg);
}

void cg_plus_con(struct node *b, struct node *l, long con)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (b->reg != l->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	
	out_instr("\taddq\t%I, %R", con, b->reg);
}

void cg_mult(struct node *b, struct node *l, struct node *r)
{
	struct node *src = r;
	
	if (reg_istemp(l->reg)) {
	
		b->reg = l->reg;
		
	} else if (reg_istemp(r->reg)) {
	
		b->reg = r->reg;
		src = l;
		
	} else {
	
		b->reg = reg_alloc();
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	}
	
	out_instr("\timulq\t%R, %R", src->reg, b->reg);
}

void cg_mult_con(struct node *b, struct node *l, long con)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (b->reg != l->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	
	out_instr("\timulq\t%I, %R", con, b->reg);
}

void cg_lea(struct node *b, struct node *base, struct node *index, long scale, long disp)
{
	assert(b);
	assert(scale == 1 || scale == 2 || scale == 4 || scale == 8);
	
	if (base && reg_istemp(base->reg))
		b->reg = base->reg;
	else if (index && reg_istemp(index->reg))
		b->reg = index->reg;
	else
		b->reg = reg_alloc();
	
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
	
	if (reg_istemp(l->reg)) {
	
		b->reg = l->reg;
		
	} else if (reg_istemp(r->reg)) {
	
		b->reg = r->reg;
		src = l;
		
	} else {
	
		b->reg = reg_alloc();
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
	}
	
	out_instr("\tandq\t%R, %R", src->reg, b->reg);
}

void cg_and_con(struct node *b, struct node *l, long con)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (l->reg != b->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
		
	out_instr("\tandq\t%I, %R", con, b->reg);
}

void cg_not(struct node *b, struct node *l)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (l->reg != b->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
		
	out_instr("\tnotq\t%R", b->reg);
}

void cg_neg(struct node *b, struct node *l)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (l->reg != b->reg)
		out_instr("\tmovq\t%R, %R", l->reg, b->reg);
		
	out_instr("\tnegq\t%R", b->reg);
}

void cg_set_cc(struct node *b, struct node *l, struct node *r, char *cc)
{
	b->reg = reg_alloc();
	
	/* clear register with xor */
	out_instr("\txor\t%R, %R", b->reg, b->reg); 
	out_instr("\tcmp\t%R, %R", r->reg, l->reg);
	out_instr("\tset%s\t%B", cc, b->reg);
}

void cg_const(struct node *b)
{
	b->reg = reg_alloc();
	out_instr("\tmovq\t%I, %R", b->value, b->reg);
}

void cg_var(struct node *b)
{
	b->reg = reg_get_var(b->symbol);
	/* subtract this reference from the remaining ref. count */
	b->symbol->refcount--;
	
	/*out_verbose("Ref. VAR %s (REG %R, %d left)",
		b->symbol->name, b->reg, b->symbol->refcount); */
}

void cg_radr(struct node *b, struct node *l)
{
	b->reg = reg_alloc();
	out_instr("\tmovq\t(%R), %R", l->reg, b->reg);
}
