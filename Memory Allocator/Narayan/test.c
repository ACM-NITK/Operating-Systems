#include <stdio.h>
#include "mem_alloc.h"

void *head;
typedef unsigned long ul;

int main()
{
	Mem_Init(4096);
	printf("head : %lu\n", (ul)head);
	int *a = (int *)Mem_Alloc(4);
	int *b = (int *)Mem_Alloc(4);
	int *c = (int *)Mem_Alloc(4);
	printf("a : %lu\n", (ul)a);
	printf("b : %lu\n", (ul)b);
	printf("c : %lu\n", (ul)c);
	Mem_Free((void *)((ul)a + 2));
	Mem_Free((void *)((ul)c + 1));
	Mem_Free((void *)((ul)b + 3));
	long long *e = (long long *)Mem_Alloc(12);
	printf("e : %lu", (ul)e);
	return 0;
}