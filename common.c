/* common compiler functions, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <limits.h>

#include "common.h"

int param_verbose;
int yyparse(void);

void usage(char *command) 
{
	printf("Usage: %s [OPTION] < FILE                    \n\n"
	       "  -h, -?  show this usage notice               \n"
	       "  -v      verbose mode                       \n\n"
	       "Limits: MIN %ld, MAX %ld                       \n"
	       "Daniel Achleitner (0926807), compiled: %s / %s \n",
	       command, LONG_MIN, LONG_MAX, __DATE__, __TIME__);
}

int main(int argc, char **argv)
{
	char c;
	param_verbose = 0;
	
	while ((c = getopt(argc, argv, "hv?")) != EOF)
	{
		switch(c)
		{
			case '?':
			case 'h':
				usage(argv[0]);
				exit(EXIT_SUCCESS);
			case 'v':
				param_verbose = 1;
				break;
			default:
				break;
		}
	}

	yyparse();
	
	if (param_verbose)
		fprintf(stderr, "allocated memory: %dkB\n", mallinfo().uordblks / 1024);

	return EXIT_SUCCESS;
}
