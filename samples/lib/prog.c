#include <stdio.h>

void ctest1(int *);

int main()
{
	int x;
	ctest1(&x);
	printf("x is %d\n", x);
	return 0;
}
