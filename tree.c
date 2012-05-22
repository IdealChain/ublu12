/* tree.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "tree.h"

struct node* gen_node(int op, struct node *left, struct node *right, long value, struct sym* symbol)
{
	struct node *t = malloc(sizeof(*t));
	MALLOC_CHECK(t)
	
	OP_LABEL(t) = op;
	LEFT_CHILD(t) = left;
	RIGHT_CHILD(t) = right;
	
	t->value = value;
	t->symbol = symbol;
	
	return t;
}

struct stat *stat_alloc()
{
	struct stat *s = malloc(sizeof(*s));
	MALLOC_CHECK(s)
	memset(s, 0, sizeof(*s));
	
	return s;
}

struct stat *stat_add(struct stat *list, struct stat *s)
{
	/* if list is empty: s gets first item */
	if (!list)
		return s;

	/* add new element at the end of the list */
	struct stat *l = list;
	
	while (l->next)
		l = l->next;
	
	l->next = s;
	return list;
}

struct stat *stat_add_return(struct stat *list, struct node *expr)
{
	assert(expr);
	
	struct stat *s = stat_alloc();
	s->type = statRETURN;
	s->expr = expr;
	
	return stat_add(list, s);
}

struct stat *stat_add_goto(struct stat *list, struct sym *label)
{
	struct stat *s = stat_alloc();
	s->type = statGOTO;
	s->data.label = label;  /* may be NULL (if label is not defined -> check traversal not run yet) */
	
	return stat_add(list, s);
}

struct stat *stat_add_if(struct stat *list, struct node *expr, struct stat *body)
{
	assert(expr);
	
	struct stat *s = stat_alloc();
	s->type = statIF;
	s->expr = expr;
	s->data.body = body;
	
	return stat_add(list, s);
}

struct stat *stat_add_vardef(struct stat *list, struct node *expr, struct sym *var)
{
	assert(expr);
	
	struct stat *s = stat_alloc();
	s->type = statVARDEF;
	s->expr = expr;
	s->data.var = var; /* may be NULL (if variable is not defined -> check traversal not run yet) */
	
	return stat_add(list, s);
}

struct stat *stat_add_memass(struct stat *list, struct node *expr, struct node *adr)
{
	assert(expr);
	assert(adr);
	
	struct stat *s = stat_alloc();
	s->type = statMEMASS;
	s->expr = expr;
	s->data.adr = adr;
	
	return stat_add(list, s);
}

struct func *func_add(struct func *list, char *name, struct sym *pars, struct stat *body)
{
	assert(name);
	
	struct func *f = malloc(sizeof(*f));
	MALLOC_CHECK(f);
	memset(f, 0, sizeof(*f));
	
	f->name = name;
	f->pars = pars; /* may be NULL */
	f->body = body; /* may be NULL */
	
	/* add new element at the end of the list */
	struct func *l = list;
	if (!list) return f;
	while (l->next)
		l = l->next;
	
	l->next = f;
	return list;
}
