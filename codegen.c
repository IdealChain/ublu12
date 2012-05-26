/* codegen.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "codegen.h"

/* a label that will be placed at the end of the current function
 * (for return statements, before restoring of callee registers) */
static struct label *func_end;

void cg_program(struct func *fs)
{
	while (fs) {
		cg_function(fs);
		fs = fs->next;
	}
}

void cg_function(struct func *f)
{
	out_instr("\t.text");
	out_instr(".globl %s", f->name);
	out_instr("\t.type %s, @function", f->name);
	struct instr *header = out_instr("%s:", f->name);
	
	/* assign parameter registers */
	reg_new_function(f->pars);
	
	{ /* debug output */
		struct sym *p = f->pars;
		
		while (p) {
			if (p->refcount) {
				struct reg *r = reg_get_var(p);
				out_verbose("PAR %s: REG %R (%d ref. in func.): %s", p->name, r, p->refcount, reg_busy_graph());
			} else
				out_verbose("PAR %s: no reference", p->name);
				
			p = p->next;
		}
	}
	
	/* clear anonymous function ending label */
	func_end = NULL;
	
	/* generate statements */
	cg_stats(f->body);
	
	/* ensure that the return register gets a value */
	/* TODO: is this necessary? */
	out_instr("\tmovq\t%I, %R", 0, reg_result());
	
	/* insert function ending label for return statements to jump to */
	if (func_end)
		out_instr("%L:", func_end);
	
	/* insert instructions to save callee registers after function header */
	reg_save_callee(header);
	/* ...and restore instructions before function return */
	reg_restore_callee();
	
	out_instr("\tret\n");
	
	/* flush generated function code to stdout */
	out_flush();
}

void cg_stats(struct stat *ss)
{
	int in_deadcode = 0;
	
	while (ss) {
		
		//until the next used label ...
		if (ss->type == statLABEL && ss->data.label->refcount)
			in_deadcode = 0;
		
		if (!in_deadcode)
			cg_stat(ss);
		
		//...everything is dead code after a return or goto
		if (ss->type == statRETURN || ss->type == statGOTO)
			in_deadcode = 1;
		
		
		ss = ss->next;
	}
}

void cg_stat(struct stat *s)
{
	switch (s->type) {
		case statRETURN:
			cg_stat_return(s);
			break;
		case statLABEL:
			cg_stat_label(s);
			break;
		case statGOTO:
			cg_stat_goto(s);
			break;
		case statIF:
			cg_stat_if(s);
			break;
		case statVARDEF:
			cg_stat_vardef(s);
			break;
		case statMEMASS:
			cg_stat_memass(s);
			break;
		case statEVAL:
			cg_stat_eval(s);
			break;
		default:
			assert(0);
	}
	
	reg_free_nonvar(NULL);
}

void cg_stat_return(struct stat *s)
{
	assert(s);
	assert(s->expr);
	
	BURM(s->expr);

	/* move return value to result register */
	if (s->expr->reg) {
		
		if (s->expr->reg != reg_result())
			out_instr("\tmovq\t%R, %R", s->expr->reg, reg_result());
	}
	else
		out_instr("\tmovq\t%I, %R", s->expr->value, reg_result());
	
	/* fetch and place jump to end of current function */
	if (!func_end)
		func_end = label_anon();
	
	out_instr("\tjmp\t%L", func_end);
}

void cg_stat_label(struct stat *s)
{
	assert(s);
	assert(s->data.label);
	
	if (!s->data.label->refcount) {
		out_verbose("LABEL %s: no references", s->data.label->name);
		return;
	}
	
	struct label *l = label_get(s->data.label);
	
	out_verbose("LABEL %s: %L", l->symbol->name, l);
	out_instr("%L:", l);
}

void cg_stat_goto(struct stat *s)
{
	assert(s);
	assert(s->data.label);
	
	struct label *l = label_get(s->data.label);
	out_instr("\tjmp\t%L", l);
}

void cg_stat_if(struct stat *s)
{
	assert(s);
	assert(s->expr);
	assert(s->data.body);
	
	struct node *expr = s->expr;
	struct label *l_end = label_anon();
	
	/* evaluate condition */
	BURM(expr);
	
	if (expr->reg) {
		
		l_end = label_anon();
		struct reg *cond = (reg_istemp(expr->reg) ? expr->reg : reg_alloc());
		
		if (cond != expr->reg) 
			out_instr("\tmovq\t%R, %R", expr->reg, cond);
		
		out_instr("\ttestb\t%I, %B", 1, cond);
		out_instr("\tje\t%L", l_end);
		
	} else {
		
		/* when compile-time condition is false: always jump to end */
		/* (the body must be generated anyway, because a goto target could be in there) */
		if (!(expr->value & 1))
			out_instr("\tjmp\t%L", l_end);
	}
	
	/* if body */
	cg_stats(s->data.body);
	
	/* end label */
	out_instr("%L:", l_end);
}

void cg_stat_vardef(struct stat *s)
{
	assert(s);
	assert(s->expr);
	assert(s->data.var);
	
	struct node *expr = s->expr;
	BURM(expr);
	
	struct reg *r = reg_get_var(s->data.var);
	
	/* move register or value to variable register */
	if (expr->reg) {
		
		if (r == expr->reg) 
			return;
			
		out_instr("\tmovq\t%R, %R", expr->reg, r);
		
	} else {
		
		out_instr("\tmovq\t%I, %R", expr->value, r);
	}
}

void cg_stat_memass(struct stat *s)
{
	assert(s);
	assert(s->expr);
	assert(s->data.adr);
	
	struct node *expr = s->expr;
	BURM(expr);
	
	struct node *adr = s->data.adr;
	BURM(adr);
	
	if (!adr->reg) {
		/* an adress as compile-time value... this shouldn't happen too often */
		adr->reg = reg_alloc();
		out_instr("\tmovq\t%I, %R", adr->value, adr->reg);
	}
	
	if (expr->reg)
		out_instr("\tmovq\t%R, (%R)", expr->reg, adr->reg);
	else
		out_instr("\tmovq\t%I, (%R)", expr->value, adr->reg);
}

void cg_stat_eval(struct stat *s)
{
	assert(s);
	
	BURM(s->expr);
}

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
	b->symbol->refcount--; /* subtract this reference from the remaining ref. count */
	
	out_verbose("VAR %s: REG %R (%d ref. left): %s", b->symbol->name, b->reg, b->symbol->refcount, reg_busy_graph());
	
	/* when no more references follow: turn variable register to temporary register */
	if (b->symbol->refcount < 1)
		b->symbol = NULL;
}

void cg_radr(struct node *b, struct node *l)
{
	b->reg = reg_alloc();
	out_instr("\tmovq\t(%R), %R", l->reg, b->reg);
}
