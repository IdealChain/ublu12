/* codegen.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "tree.h"
#include "codegen.h"

int param_verbose;

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
				fprintf(stderr, "# PAR %s: REG %s (%d ref. in func.)\n", p->name, r->name, p->refcount);
			} else
				fprintf(stderr, "# PAR %s: no reference\n", p->name);
				
			p = p->next;
		}
	}
	
	/* generate statements */
	int had_return = cg_stats(f->body);
	
	/* ensure that the return register gets a value */
	if (!had_return)
		printf("\tmovq\t$0, %s\n", reg_result()->name);
	
	reg_restore_callee();
	printf("\tret\n\n");
}

int cg_stats(struct stat *ss)
{
	while (ss) {
		cg_stat(ss);
		
		if (ss->type == statRETURN)
			return 1;
		
		ss = ss->next;
	}
	
	return 0;
}

void cg_stat(struct stat *s)
{
	switch (s->type) {
		case statRETURN:
			cg_stat_return(s);
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
		default:
			assert(0);
	}
	
	reg_free_nonvar(NULL);
}

void cg_stat_return(struct stat *s)
{
	struct node *expr = s->expr;
	BURM(expr);

	if (expr->reg && expr->reg != reg_result())
		printf("\tmovq\t%s, %s\n", REG(expr), reg_result()->name);
}

void cg_stat_goto(struct stat *s)
{
}

void cg_stat_if(struct stat *s)
{
}

void cg_stat_vardef(struct stat *s)
{
	struct node *expr = s->expr;
	BURM(expr);
	
	struct reg *r = reg_get_var(s->data.var);
	if (r == expr->reg) 
		return;
		
	printf("\tmovq\t%s, %s\n", REG(expr), r->name);
}

void cg_stat_memass(struct stat *s)
{
	struct node *expr = s->expr;
	BURM(expr);
	reg_free_nonvar(expr->reg); /* intermediate flushing :) */
	
	struct node *adr = s->data.adr;
	BURM(adr);
	
	printf("\tmovq\t%s, (%s)\n", REG(expr), REG(adr));
}


void cg_plus(struct node *b, struct node *l, struct node *r)
{
	b->reg = reg_alloc();
	printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	printf("\taddq\t%s, %s\n", REG(r), REG(b));
}

void cg_mult(struct node *b, struct node *l, struct node *r)
{
	b->reg = reg_alloc();
	printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	printf("\timulq\t%s, %s\n", REG(r), REG(b));
}

void cg_and(struct node *b, struct node *l, struct node *r)
{
	b->reg = reg_alloc();
	printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	printf("\tandq\t%s, %s\n", REG(r), REG(b));
}

void cg_not(struct node *b, struct node *l)
{
	b->reg = reg_alloc();
	printf("\tmovq\t%s, %s\n", REG(l), REG(b));
	printf("\tnotq\t%s\n", REG(b));
}

void cg_neg(struct node *b, struct node *l)
{
	b->reg = reg_alloc();
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