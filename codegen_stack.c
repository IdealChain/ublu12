/* codegen_stack.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "codegen.h"

/* how much space to reserve on stack frame */
static long stackframe_reserve = 0;

/* current stack head */
static struct stackvar *head = NULL;

void stack_new_function()
{
	stackframe_reserve = 0;
	head = NULL;
}

struct instr *stack_reservespace(struct instr *function_header)
{
	struct instr *after = function_header;
	
	if (stackframe_reserve) {
		after = out_instr_insert(after, "\tpushq\t%s", "%rbp");
		after = out_instr_insert(after, "\tmovq\t%s, %s", "%rsp", "%rbp");
		after = out_instr_insert(after, "\tsubq\t%I, %s", stackframe_reserve, "%rsp");
	}
	
	return after;
}

void stack_returnspace()
{
	if (stackframe_reserve) 
		out_instr("\tleave");
}

struct stackvar *stack_alloc()
{
	/* get current position, relative to %rbp */
	long pos = (head ? head->disp : 0);
	pos -= sizeof(long);
	
	/* set reserved space to maximum */
	if (-pos > stackframe_reserve)
		stackframe_reserve = -pos;
	
	/* data structure */
	struct stackvar *s = calloc(1, sizeof(*s));
	MALLOC_CHECK(s)
	
	s->disp = pos;
	
	/* linked list grows to the front */
	if (head) {
		s->next = head;
		head = s;
	} else {
		s->next = NULL;
		head = s;
	}
	
	return s;
}

struct stackvar *stack_push(struct reg *r)
{
	assert(r);
	
	/* data structure */
	struct stackvar *s = stack_alloc();
	s->symbol = r->symbol;
	
	/* move value */
	out_instr("\tmovq\t%R, %S", r, s);
	
	return s;
}

struct stackvar *stack_push_immediate(long value)
{
	/* data structure */
	struct stackvar *s = stack_alloc();
	
	/* move value */
	out_instr("\tmovq\t%I, %S", value, s);
	
	return s;
}

void stack_pop(struct reg *r)
{
	assert(r);
	assert(head);
	
	/* remove head element from list */
	struct stackvar *s = head;
	head = s->next;
	
	/* move value */
	out_instr("\tmovq\t%S, %R", s, r);
	reg_busy(r);
	r->symbol = s->symbol;
}

struct stackvar *stack_get_var(struct sym *var)
{
	struct stackvar *s = head;
	
	while (s) {
		/* compare by name string
		 * (sym struct could have been duplicated by sym_concat) 
		 */
		if (s->symbol && s->symbol->name == var->name)
			return s;
		
		s = s->next;
	}
	
	return NULL;
}