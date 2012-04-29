/* symtable.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "symtable.h"

struct sym *sym_add(struct sym *table, char *name, int lineno)
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
	if (!s) {
		fputs("not enough memory\n", stderr);
		ERROR_MEM;
	}
	
	s->name = name;
	s->line = lineno;
	
	s->next = table; /* may be NULL */
	
	return s;
}

const struct sym *sym_find(const struct sym *table, const char *name)
{
	assert(name);
	const struct sym *s = table;

	while (s) {
		if (!strcmp(name, s->name))
			return s;
		
		s = s->next;
	}
	
	return NULL;
}

void sym_print(const struct sym *table, const char *caption, const char *subject)
{
	assert(caption);
	const struct sym *s = table;
	
	fprintf(stderr, "symbol table %s of \"%s\":\n", caption, subject);
	
	int i = 0;
	while (s) {
		fprintf(stderr, "%2d symbol: %s (line %2d)\n", 
			i, s->name, s->line);
		i++;
		
		s = s->next;
	}
	
	if (i == 0)
		fputs("<empty>\n", stderr);
	
	fputs("\n", stderr);
}