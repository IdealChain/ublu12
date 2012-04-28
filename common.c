/* common compiler functions, Daniel Achleitner (0926807) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

int yyparse(void);

void usage(char *command) 
{
	printf("Usage: %s [OPTION] < FILE                    \n\n"
	       "  -h, -?  show this usage notice               \n"
	       "  -v      verbose mode                       \n\n"
	       "Daniel Achleitner (0926807), compiled: %s / %s \n",
	       command, __DATE__, __TIME__);
}

int main(int argc, char **argv)
{
	char c;
	
	while ((c = getopt(argc, argv, "hv?")) != EOF)
	{
		switch(c)
		{
			case '?':
			case 'h':
				usage(argv[0]);
				exit(EXIT_SUCCESS);
			case 'v':
				//todo: enable verbose mode
				fputs("not yet implemented: verbose mode\n", stderr);
				exit(EXIT_FAILURE);
				break;
			default:
				break;
		}
	}

	yyparse();

	return EXIT_SUCCESS;
}