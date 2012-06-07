/* output.h, Daniel Achleitner (0926807) */
#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdarg.h>

#include "common.h"
#include "exprtree.h"

/* type of instruction, unspec (0) if not known */
enum instr_type {
	instrUNSPEC = 0,
	instrVERBOSE,
	instrJMP,
	instrLBL
};

/* linked list of output instructions in buffer */
struct instr {
	char output[60];      /* generated output for this instr */
	
	enum instr_type type; /* type of this instruction */
	struct label *lbl;    /* label to jump to or to output */
	
	struct instr *next;   /* next instruction in list, if any */
};

/* adds an instruction to the output buffer */
struct instr *out_instr(const char *format, ...);

/* inserts an instruction in the middle of the output buffer */
struct instr *out_instr_insert(struct instr *after, const char *format, ...);

/* adds a jmp instruction to the output buffer */
struct instr *out_jmp(struct label *to);

/* adds a label to the output buffer */
struct instr *out_lbl(struct label *l);

/* adds a debug output to the output buffer */
struct instr *out_verbose(const char *format, ...);

/* flushes all pending instructions in buffer to stdout */
void out_flush();

#endif
