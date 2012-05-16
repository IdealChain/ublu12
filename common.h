/* common compiler headers, Daniel Achleitner (0926807) */
#ifndef COMMON_H
#define COMMON_H

#define ERROR_LEX exit(1)
#define ERROR_SYN exit(2)
#define ERROR_SEM exit(3)
#define ERROR_MEM exit(4)
#define ERROR_INT exit(5)

extern int param_verbose;

#define MALLOC_CHECK(t) \
	if (!t) { \
		fputs("error: not enough memory\n", stderr); \
		ERROR_MEM; \
	}

#endif
