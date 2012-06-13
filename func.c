#include <stdio.h>

long ggt(long, long);

int main(int argc, char **argv) 
{
	long v = 12;
	long w = 8;
	
	printf("calling ggt(%ld, %ld) ...\n", v, w);
	long r = ggt(v, w);
	printf("result: %ld\n", r);
	
	return 0;
}

void print(long a, long b)
{
	printf("ggt(%ld, %ld)\n", a, b);
}

long ggt_print(long a, long b)
{
	print(a, b);
	return ggt(a, b);
}