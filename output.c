/* output.c, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "output.h"

int param_verbose;

/* instructions in buffer */
static struct instr *instructions;

/* last element in instruction list */
static struct instr *last_instr;

/* type for converter callback */
typedef int (*converter)(char *str, size_t size, va_list va);

/* Converter for %I: immediate value */
int conv_immediate(char *str, size_t size, va_list va) 
{
	long value = va_arg(va, long);
	return snprintf(str, size, "$%ld", value);
}

/* Converter for %L: label */
int conv_label(char *str, size_t size, va_list va) 
{
	struct label *l = va_arg(va, struct label*);
	return snprintf(str, size, ".L%u", l->nr);
}

/* Converter for %R: register name */
int conv_register(char *str, size_t size, va_list va)
{
	struct reg *r = va_arg(va, struct reg*);
	return snprintf(str, size, "%s", r->name);
}

/* Converter for %B: byte-register name */
int conv_byte_register(char *str, size_t size, va_list va) 
{
	struct reg *r = va_arg(va, struct reg*);
	return snprintf(str, size, "%s", r->name_b);
}

/* Converter for %S: stack address (displacement to %rbp register) */
int conv_stack(char *str, size_t size, va_list va)
{
	struct stackvar *s = va_arg(va, struct stackvar*);
	return snprintf(str, size, "%ld(%s)", s->disp, "%rbp");
}

converter find_conv(char specifier) 
{
	switch(specifier) {
		case 'I': 
			return conv_immediate;
		case 'L':
			return conv_label;
		case 'R':
			return conv_register;
		case 'B':
			return conv_byte_register;
		case 'S':
			return conv_stack;
		default:
			return NULL;
	};
}


struct instr *alloc_instr()
{
	struct instr *i = malloc(sizeof(*i));
	MALLOC_CHECK(i)
	
	memset(i, 0, sizeof(*i));
	
	return i;
}

struct instr *queue_instr(struct instr *i)
{
	assert(i);
	assert(!i->next);
	
	if (last_instr) {
		last_instr->next = i;
		last_instr = i;
	} else {
		instructions = last_instr = i;
	}
	
	return i;
}

/* Variadic instruction output function that wraps printf:
 * - applies custom conversions (see above) ...
 * - and leaves the usual conversions for printf to handle
 */
struct instr *out(const char *format, va_list va)
{
	struct instr *i = alloc_instr();
	char *dst = i->output;
	
	/* mutable format string */
	char *p = strdup(format);
	
	do {
		char *q = p;
		converter c = NULL;
		
		/* find (first) conversion specifier we should handle */
		while (*q) {
			if (*q == '%' && *(q + 1)) {
				
				c = find_conv(*(q + 1));
				
				if (c) {
					/* end printf string here */
					*q++ = '\0';
					*q++ = '\0';
					break;
				}
				
				q++;
			}
			q++;
		}
		
		/* let vsnprintf do what it does best */
		size_t maxlen = sizeof(i->output) - (dst - i->output);
		dst += vsnprintf(dst, maxlen, p, va);
		
		if (c) {
			/* consume and convert one argument */
			maxlen = sizeof(i->output) - (dst - i->output);
			dst += c(dst, maxlen, va);
		}
		
		p = q;
		
	} while (*p);
	
	return i;
}

struct instr *out_instr(const char *format, ...)
{
	assert(format);
	va_list va;
	
	va_start(va, format);
	struct instr *r = out(format, va);
	va_end(va);
	
	return queue_instr(r);
}

struct instr *out_instr_insert(struct instr *after, const char *format, ...)
{
	assert(after);
	assert(format);
	va_list va;
	
	{ /* check that after is indeed in buffer */
		struct instr *i = instructions;
		while (i && i != after) 
			i = i->next;
		assert(i);
	}
	
	va_start(va, format);
	struct instr *r = out(format, va);
	va_end(va);
	
	r->next = after->next;
	after->next = r;
	
	return r;
}

struct instr *out_jmp(struct label *to)
{
	assert(to);
	
	struct instr *i = out_instr("\tjmp\t%L", to);
	i->type = instrJMP;
	i->lbl = to;
	
	return i;
}

struct instr *out_lbl(struct label *l)
{
	assert(l);

	struct instr *i = out_instr("%L:", l);
	i->type = instrLBL;
	i->lbl = l;
	
	return i;
}

struct instr *out_verbose(const char *format, ...)
{
	if (!param_verbose)
		return NULL;

	assert(format);
	va_list va;
	
	char *f = malloc(strlen(format) + 3);
	MALLOC_CHECK(f)
	
	/* prepend asm comment character */
	strcpy(f, "# ");
	strcpy(f + 2, format);
	
	va_start(va, format);
	struct instr *r = out(f, va);
	va_end(va);
	
	free(f);
	
	r->type = instrVERBOSE;
	
	return queue_instr(r);
}

/* filters out redundand jmp instructions, where the 
 * called-for label follows immediately afterward 
 * (kind of a peephole optimization) */
int out_jmp_filter(struct instr *i)
{
	if (i->type == instrJMP) {

		struct instr *n = i->next;
		
		while (n && n->type == instrVERBOSE)
			n = n->next;
		
		if (n && n->type == instrLBL && n->lbl == i->lbl)
			return 1;
	}
	
	return 0;
}

void out_flush()
{
	if (!instructions)
		return;
	
	struct instr *i = instructions;
	
	while (i) {
		
		int f = out_jmp_filter(i);
		
		if (f) {
			if (param_verbose) 
				printf("# F: %s\n", i->output);
		}
		else
			puts(i->output);
		
		i = i->next;
	}
	
	instructions = last_instr = NULL;
}
