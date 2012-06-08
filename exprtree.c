/* exptree.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "exprtree.h"
#include "codegen.h"

/* forward declaration (extern) */
struct burm_state *burm_label(struct node *bnode);
void burm_reduce(struct node *bnode, int goalnt);

void burm(struct node *expr, struct reg *target)
{
	assert(expr);

	if (target)
		reg_hint(target);

	burm_label(expr);
	burm_reduce(expr, 1);

	if (target) {
		/* move register or immediate value to target reg, if hint did not work */
		if (expr->reg && expr->reg != target) {
			out_instr("\tmovq\t%R, %R", expr->reg, target);
			reg_free(expr->reg);
		}
		else if (!expr->reg)
			out_instr("\tmovq\t%I, %R", expr->value, target);
		
		expr->reg = target;
		reg_busy(expr->reg);
	}
}

struct node *gen_node(int op, struct node *left, struct node *right, long value, struct sym* symbol)
{
	struct node *t = calloc(1, sizeof(*t));
	MALLOC_CHECK(t)
	
	/* check whether this constant would be suitable for a lea scale parameter */
	if (op == opCONST && (value == 1 || value == 2 || value == 4 || value == 8))
		op = opLEACONST;
	
	OP_LABEL(t) = op;
	LEFT_CHILD(t) = left;
	RIGHT_CHILD(t) = right;
	
	t->value = value;
	t->symbol = symbol;
	
	return t;
}

struct node *gen_call_node(char *function, unsigned int lineno, struct param *params)
{
	struct node *t = calloc(1, sizeof(*t));
	MALLOC_CHECK(t)
	
	OP_LABEL(t) = opCALL;
	
	t->symbol = sym_add(NULL, function, lineno);
	t->params = params;
	
	return t;
}

struct param *add_param(struct param *list, struct node *expr)
{
	struct param *param = malloc(sizeof(*param));
	MALLOC_CHECK(param)
	
	param->expr = expr;
	param->next = NULL;

	/* if list is empty: param gets first item */
	if (!list)
		return param;

	/* add new parameter at the end of the list */
	struct param *l = list;
	
	while (l->next)
		l = l->next;
	
	l->next = param;
	return list;
}

struct node* node_nav(struct node *b, char *path)
{
	char p;
	
	while ((p = *(path++))) {
		switch(p) {
			
			case 'l':
				b = LEFT_CHILD(b);
				break;
			case 'r':
				b = RIGHT_CHILD(b);
				break;
			default:
				assert(0);
		}
	}
	
	return b;
}
