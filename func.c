#include <stdio.h>

long func(long);
long called(long arg1, long arg2, long arg3);

int main(int argc, char **argv) 
{
	long v = 7;
	
	printf("calling func(%ld) ...\n", v);
	long r = func(v);
	printf("result: %ld\n", r);
	
	return 0;
}

long called(long arg1, long arg2, long arg3)
{
	return 2 * arg1;
}