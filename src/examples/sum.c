#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int main(int argc, char **argv)
{
	if (argc < 5)
		return EXIT_FAILURE;

	int a = atoi(argv[1]);
	int b = atoi(argv[2]);
	int c = atoi(argv[3]);
	int d = atoi(argv[4]);

	printf("%d %d\n",pibonacci(a), sum_of_four_integers(a,b,c,d));

	return EXIT_SUCCESS;
	
}
