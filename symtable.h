/* symtable.h, Daniel Achleitner (0926807) */
#ifndef SYMTABLE_H
#define SYMTABLE_H

struct sym {
	char *name;            /* name of symbol */
	unsigned int line;     /* line of definition in source file */
	unsigned int refcount; /* number of (remaining) references */

	struct sym *next; /* next symbol in linked list, if any */
};

/* adds a symbol to an existing symbol table (or new, if NULL given) */
struct sym *sym_add(struct sym *table, char *name, unsigned int lineno);

/* concatenates two symbol tables */
struct sym *sym_concat(struct sym *s, struct sym *tail);

/* finds a symbol by name and returns a pointer to it or NULL, if not found */
struct sym *sym_find(struct sym *table, const char *name);

/* prints content of symbol table for debugging purposes */
void sym_print(const struct sym *table, const char *caption, const char *subject);

#endif
