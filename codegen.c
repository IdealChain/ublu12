/* codegen.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "codegen.h"

int param_verbose;

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
	printf("\t.text\n");
	printf(".globl %s\n", f->name);
	printf("\t.type %s, @function\n", f->name);
	printf("%s:\n", f->name);
	
	/* assign parameter registers */
	reg_new_function(f->pars);
	
	if (param_verbose) {
		struct sym *p = f->pars;
		
		while (p) {
			if (p->refcount) {
				struct reg *r = reg_get_var(p);
				printf("# PAR %s: REG %s (%d ref. in func.)\n", p->name, r->name, p->refcount);
			} else
				printf("# PAR %s: no reference\n", p->name);
				
			p = p->next;
		}
	}
	
	/* clear anonymous function ending register */
	func_end = NULL;
	
	/* generate statements */
	cg_stats(f->body);
	
	/* ensure that the return register gets a value */
	/* TODO: is this necessary? */
	printf("\tmovq\t$0, %s\n", reg_result()->name);
	
	if (func_end)
		printf(".L%u:\n", func_end->nr);
		
	reg_restore_callee();
	printf("\tret\n\n");
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
			printf("\tmovq\t%s, %s\n", REG(s->expr), reg_result()->name);
	}
	else
		printf("\tmovq\t$%ld, %s\n", s->expr->value, reg_result()->name);
	
	/* fetch and place jump to end of current function */
	if (!func_end)
		func_end = label_anon();
	
	printf("\tjmp\t.L%u\n", func_end->nr);
}

void cg_stat_label(struct stat *s)
{
	assert(s);
	assert(s->data.label);
	
	if (!s->data.label->refcount) {
		if (param_verbose)
			printf("# LABEL %s: no references\n", s->data.label->name);
		return;
	}
	
	struct label *l = label_get(s->data.label);
	
	if (param_verbose)
		printf("# LABEL %s: .L%u\n", l->symbol->name, l->nr);
	
	printf(".L%u:\n", l->nr);
}

void cg_stat_goto(struct stat *s)
{
	assert(s);
	assert(s->data.label);
	
	struct label *l = label_get(s->data.label);
	printf("\tjmp\t.L%u\n", l->nr);
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
			printf("\tmovq\t%s, %s\n", expr->reg->name, cond->name);
		
		printf("\ttestb\t$1, %s\n", cond->name_b);
		printf("\tje\t.L%u\n", l_end->nr);
		
	} else {
		
		/* when compile-time condition is false: always jump to end */
		/* (the body must be generated anyway, because a goto target could be in there) */
		if (!(expr->value & 1))
			printf("\tjmp\t.L%u\n", l_end->nr);
	}
	
	/* if body */
	cg_stats(s->data.body);
	
	/* end label */
	printf(".L%u:\n", l_end->nr);
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
			
		printf("\tmovq\t%s, %s\n", REG(expr), r->name);
		
	} else {
		
		printf("\tmovq\t$%ld, %s\n", expr->value, r->name);
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
		printf("\tmovq\t$%ld, %s\n", adr->value, REG(adr));
	}
	
	if (expr->reg)
		printf("\tmovq\t%s, (%s)\n", REG(expr), REG(adr));
	else
		printf("\tmovq\t$%ld, (%s)\n", expr->value, REG(adr));
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
		printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	}
	
	printf("\taddq\t%s, %s\n", REG(src), REG(b));
}

void cg_plus_con(struct node *b, struct node *l, long con)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (b->reg != l->reg)
		printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	
	printf("\taddq\t$%ld, %s\n", con, REG(b));
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
		printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	}
	
	printf("\timulq\t%s, %s\n", REG(src), REG(b));
}

void cg_mult_con(struct node *b, struct node *l, long con)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (b->reg != l->reg)
		printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	
	printf("\timulq\t$%ld, %s\n", con, REG(b));
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
		printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	}
	
	printf("\tandq\t%s, %s\n", REG(src), REG(b));
}

void cg_and_con(struct node *b, struct node *l, long con)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (l->reg != b->reg)
		printf("\tmovq\t%s, %s\n", REG(l), REG(b));
		
	printf("\tandq\t$%ld, %s\n", con, REG(b));
}

void cg_not(struct node *b, struct node *l)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (l->reg != b->reg)
		printf("\tmovq\t%s, %s\n", REG(l), REG(b));
		
	printf("\tnotq\t%s\n", REG(b));
}

void cg_neg(struct node *b, struct node *l)
{
	b->reg = (reg_istemp(l->reg) ? l->reg : reg_alloc());
	
	if (l->reg != b->reg)
		printf("\tmovq\t%s, %s\n", REG(l), REG(b));
		
	printf("\tnegq\t%s\n", REG(b));
}

void cg_set_cc(struct node *b, struct node *l, struct node *r, char *cc)
{
	b->reg = reg_alloc();
	
	/* clear register with xor */
	printf("\txor\t%s, %s\n", REG(b), REG(b)); 
	printf("\tcmp\t%s, %s\n", REG(r), REG(l));
	printf("\tset%s\t%s\n", cc, REGB(b));
}

void cg_const(struct node *b)
{
	b->reg = reg_alloc();
	printf("\tmovq\t$%ld, %s\n", b->value, REG(b));
}

void cg_var(struct node *b)
{
	b->reg = reg_get_var(b->symbol);
	b->symbol->refcount--; /* subtract this reference from the remaining ref. count */
	
	if (param_verbose)
		printf("# VAR %s: REG %s (%d ref. left)\n", b->symbol->name, REG(b), b->symbol->refcount);
	
	/* when no more references follow: turn variable register to temporary register */
	if (b->symbol->refcount < 1)
		b->symbol = NULL;
}

void cg_radr(struct node *b, struct node *l)
{
	b->reg = reg_alloc();
	printf("\tmovq\t(%s), %s\n", REG(l), REG(b));
}
