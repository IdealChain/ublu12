/* symtable.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "symtable.h"

int param_verbose;

struct sym *sym_add(struct sym *table, char *name, unsigned int lineno)
{
	assert(name);

	/* check whether a symbol with that name already exists */
	const struct sym *coll = sym_find(table, name);
	if (coll) {
		fprintf(stderr, "error: line %d: symbol '%s' already defined (line %d)\n", 
			lineno, name, coll->line);
		ERROR_SEM;
	}

	struct sym *s = malloc(sizeof(*s));
	MALLOC_CHECK(s)
	
	memset(s, 0, sizeof(*s));
	
	s->name = name;
	s->line = lineno;
	s->next = table; /* add new element at the beginning of the list; may be NULL */
	
	return s;
}

struct sym *sym_concat(struct sym *s, struct sym *tail)
{
	/* recursive function that concatenates two symtables,
	 * while duplicating the first one (to leave the original intact) */
	if (!s)
		return tail;

	tail = sym_concat(s->next, tail);

	/* duplicate s */
	struct sym *c = malloc(sizeof(*s));
	MALLOC_CHECK(c)
	memcpy(c, s, sizeof(*s));

	/* and add it to front of current tail */
	c->next = tail;
	return c;
}

struct sym *sym_find(struct sym *table, const char *name)
{
	assert(name);
	struct sym *s = table;

	while (s) {
		if (!strcmp(name, s->name))
			return s;
		
		s = s->next;
	}
	
	return NULL;
}

void sym_print(const struct sym *table, const char *caption, const char *subject)
{
	if (!param_verbose)
		return;

	assert(caption);
	const struct sym *s = table;
	
	fprintf(stderr, "symbol table %s of \"%s\":\n", caption, subject);
	
	int i = 0;
	while (s) {
		
		fprintf(stderr, "%2d symbol: %8s, line: %2d,  refcount: %2d\n", 
			i, s->name, s->line, s->refcount);
		
		i++;
		s = s->next;
	}
	
	if (i == 0)
		fputs("<empty>\n", stderr);
	
	fputs("\n", stderr);
}
