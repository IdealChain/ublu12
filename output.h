/* output.h, Daniel Achleitner (0926807) */
#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdarg.h>

#include "common.h"
#include "exprtree.h"

/* Linked list of output instructions in buffer */
struct instr {
	char output[60];
	
	struct instr *next;
};

/* printf-like specifier replacement
 * see static array in output.c for the list */
struct conversion {
	char specifier;
	int (*convert)(char *str, size_t size, va_list va);
};

/* adds an instruction to the output buffer */
struct instr *out_instr(const char *format, ...);

/* inserts an instruction in the middle of the output buffer */
struct instr *out_instr_insert(struct instr *after, const char *format, ...);

/* adds a debug output to the output buffer */
struct instr *out_verbose(const char *format, ...);

/* flushes all pending instructions in buffer to stdout */
void out_flush();

#endif
