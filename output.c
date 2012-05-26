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

static struct conversion convs[] = {
	{ 'I', conv_immediate },
	{ 'L', conv_label },
	{ 'R', conv_register },
	{ 'B', conv_byte_register }
};

struct conversion *find_conv(char specifier) 
{
	int i;
	struct conversion *c;
	
	for(i = 0, c = &convs[0]; i < sizeof(convs) / sizeof(*c); i++, c = &convs[i])
	{
		if (c->specifier == specifier) {
			return c;
		}
	}
	return NULL;
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
		struct conversion *c = NULL;
		
		/* find (first) conversion specifier we should handle */
		while (*q) {
			if (*q == '%' && *(q + 1)) {
				
				c = find_conv(*(q + 1));
				
				if (c) {
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
			dst += c->convert(dst, maxlen, va);
		}
		
		p = q;
		
	} while (*p);
	
	return i;
}

struct instr *out_instr(const char *format, ...)
{
	va_list va;
	
	va_start(va, format);
	struct instr *r = out(format, va);
	va_end(va);
	
	return queue_instr(r);
}

struct instr *out_instr_insert(struct instr *after, const char *format, ...)
{
	assert(after);
	va_list va;
	
	va_start(va, format);
	struct instr *r = out(format, va);
	va_end(va);
	
	if (after->next)
		r->next = after->next;
	
	after->next = r;
	
	return r;
}

struct instr *out_verbose(const char *format, ...)
{
	if (!param_verbose)
		return NULL;
	
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
	
	return queue_instr(r);
}

void out_flush()
{
	if (!instructions)
		return;
	
	struct instr *i = instructions;
	
	while (i) {
		puts(i->output);
		i = i->next;
	}
	
	instructions = last_instr = NULL;
}
