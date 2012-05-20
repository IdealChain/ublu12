#include <stdio.h>

long func(long);
long a = 6;

int main(int argc, char **argv) 
{
	printf("calling func(%p) ...\n", &a);
	long r = func((long)&a);
	printf("result: %ld\n", r);
	
	return 0;
}
