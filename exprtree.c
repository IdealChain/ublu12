/* exptree.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "stmttree.h"

struct node* gen_node(int op, struct node *left, struct node *right, long value, struct sym* symbol)
{
	struct node *t = malloc(sizeof(*t));
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
