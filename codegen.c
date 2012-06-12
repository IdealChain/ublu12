/* codegen.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "codegen.h"

/* a label that will be placed at the end of the current function
 * (for return statements, before restoring of callee registers) */
static struct label *func_end;

/* whether we are currently in dead code (...that cannot be reached 
 * except when a referenced label comes along) */
static int in_deadcode;

void cg_function(char *name, struct sym *pars, struct sym *scope_vars, struct stat *body)
{
	/* clear anonymous function ending label and deadcode flag */
	func_end = NULL;
	in_deadcode = 0;
	
	out_instr("\t.text");
	out_instr(".globl %s", name);
	out_instr("\t.type %s, @function", name);
	struct instr *header = out_instr("%s:", name);
	
	/* assign parameter registers */
	reg_new_function(pars);
	stack_new_function();
	
	{ /* debug output */
		struct sym *p = pars;
		
		while (p) {
			if (p->refcount) {
				struct reg *r = reg_get_var(p, NULL);
				out_verbose("%s PAR %s: REG %R (%d ref. in func.)", reg_busy_graph(), p->name, r, p->refcount);
			} else
				out_verbose("%s PAR %s: no reference", reg_busy_graph(), p->name);
				
			p = p->next;
		}
	}
	
	/* generate statements */
	cg_stats(body, scope_vars);
	
	/* ensure that the return register gets a value
	 * (is not really necessary but helps avoiding random results) 
	 */
	if (!in_deadcode)
		out_instr("\tmovq\t%I, %R", 0, reg_result());
	
	/* insert function ending label for return statements to jump to */
	if (func_end)
		out_lbl(func_end);
	
	/* insert instructions to save callee registers and reserve stack space after function header */
	header = reg_save_callee(header);
	header = stack_reservespace(header);
	
	/* restore stack space and callee saved registers before return */
	stack_returnspace();
	reg_restore_callee();
	
	out_instr("\tret\n");
	
	/* flush generated function code to stdout */
	out_flush();
}

void cg_stats(struct stat *ss, struct sym *scope_vars)
{
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

	/* free vars whose scope ends here */
	reg_free_vars(scope_vars);
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
	
	/* free temporary registers */
	reg_free_nonvar();
}

void cg_stat_return(struct stat *s)
{
	assert(s);
	assert(s->expr);
	
	BURM_REG(s->expr, reg_result());
	
	/* fetch and place jump to end of current function */
	if (!func_end)
		func_end = label_anon();
	
	out_jmp(func_end);
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
	out_lbl(l);
}

void cg_stat_goto(struct stat *s)
{
	assert(s);
	assert(s->data.label);
	
	struct label *l = label_get(s->data.label);
	out_jmp(l);
}

/* Generates a special if instruction in the case that the if body only
 * contains one statement, which is either a variable assignment
 * (definition) or a return statement (when is_return is set). 
 * This can be done using conditional move and jump instructions
 * cmovCC and jCC.
 */
void cg_stat_if_cvardef_creturn(struct node *cond, struct stat *s, int is_return)
{
	assert(cond);
	assert(s);
	assert(s->data.var || is_return);
	assert(!s->next);
	
	/* evaluate expression */
	BURM(s->expr);
	
	/* evaluate condition */
	BURM(cond);
	
	const struct reg *r = (is_return ? reg_result() : reg_get_var(s->data.var, NULL));
	
	if (cond->reg) {
		
		/* cmovCC instruction cannot handle immediate values */
		if (!s->expr->reg) {
			s->expr->reg = reg_alloc();
			out_instr("\tmovq\t%I, %R", s->expr->value, s->expr->reg);
		}
		
		/* when lowest bit is set: 
		 * set variable register to value / jump to end of function */
		out_instr("\ttestb\t%I, %B", 1, cond->reg);
		out_instr("\tcmovne\t%R, %R", s->expr->reg, r);
		
		if (is_return) {
			if (!func_end)
				func_end = label_anon();
				
			out_instr("\tjne\t%L", func_end);
		}
		
	} else {
		
		/* when compile-time condition...
		 * ... is true: set register to value / jump to end of function */
		if (cond->value & 1) {
			
			if (s->expr->reg)
				out_instr("\tmovq\t%R, %R", s->expr->reg, r);
			else
				out_instr("\tmovq\t%I, %R", s->expr->value, r);
				
			if (is_return) {
				if (!func_end)
					func_end = label_anon();
				
				out_jmp(func_end);
				in_deadcode = 1;
			}
		}
	}
}

/* Generates a special if instruction in the case that the if body only
 * contains one statement, which is a goto statement. This can be done
 * using conditional jCC jumps.
 */
void cg_stat_if_cgoto(struct node *cond, struct stat *s)
{
	assert(cond);
	assert(s);
	assert(s->data.label);
	assert(!s->next);
	
	struct label *l = label_get(s->data.label);
	/* evaluate condition */
	BURM(cond);
	
	if (cond->reg) {
		
		/* when lowest bit is set: jump to label */
		out_instr("\ttestb\t%I, %B", 1, cond->reg);
		out_instr("\tjne\t%L", l);
		
	} else {
		
		/* when compile-time condition...
		 * ... is true: jump to label */
		if (cond->value & 1) {
			out_jmp(l);
			in_deadcode = 1;
		}
	}
}

void cg_stat_if(struct stat *s)
{
	assert(s);
	assert(s->expr);
	
	if (!s->data.body)
		return;
	
	/* if there's only one statement in the body, it can be 
	 * simplified to a cmov or cjmp. */
	if (s->data.body && s->data.body->next == NULL) {
		
		switch(s->data.body->type) {
		
			case statVARDEF:
				cg_stat_if_cvardef_creturn(s->expr, s->data.body, 0);
				/* if this was a var definition, it's scope could have ended immediately */
				reg_free_vars(s->scope_vars);
				return;
			case statRETURN:
				/* return is a special type of variable assignment */
				cg_stat_if_cvardef_creturn(s->expr, s->data.body, 1);
				return;
			case statGOTO:
				cg_stat_if_cgoto(s->expr, s->data.body);
				return;
			default:
				break;
		}
	}
	
	struct node *cond = s->expr;
	struct label *l_end = NULL;
	
	/* evaluate condition */
	BURM(cond);
	
	if (cond->reg) {
		
		/* when lowest bit is not set: jump to end of if body */
		out_instr("\ttestb\t%I, %B", 1, cond->reg);
		
		l_end = label_anon();
		out_instr("\tje\t%L", l_end);
		
	} else {
		
		/* when compile-time condition...
		 * ... is false: always jump to end
		 * ... is true: don't jump anywhere
		 * (the body must be generated anyway, 
		 * because a goto target could be in there)
		 */
		if (!(cond->value & 1)) {
			l_end = label_anon();
			out_jmp(l_end);
			in_deadcode = 1;
		}
	}
	
	/* if body */
	cg_stats(s->data.body, s->scope_vars);
	
	/* end label */
	if (l_end) {
		out_lbl(l_end);
		in_deadcode = 0;
	}
}

void cg_stat_vardef(struct stat *s)
{
	assert(s);
	assert(s->expr);
	assert(s->data.var);
	
	struct node *expr = s->expr;
	BURM(expr);
	
	/* check if var is in stack */
	struct stackvar *sv = stack_get_var(s->data.var);
	if (sv) {
		if (expr->reg)
			out_instr("\tmovq\t%R, %ld(%s)", expr->reg, sv->disp, "%rbp");
		else
			out_instr("\tmovq\t%I, %ld(%s)", expr->value, sv->disp, "%rbp");
		
		return;
	}
	
	struct reg *r = reg_get_var(s->data.var, expr->reg);
	
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
		/* an adress as compile-time value... 
		 * this shouldn't happen too often */
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
