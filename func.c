#include <stdio.h>

long func(long);
long func2(long);

int main(int argc, char **argv) 
{
	printf("calling func(%ld) ...\n", 0);
	long r = func(0);
	printf("result: %ld\n", r);
	
	return 0;
}
