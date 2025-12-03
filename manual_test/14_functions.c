/* Test 14: Function calls and returns */
#include <stdio.h>
#include <stdlib.h>

int add(int a, int b)
{
return a + b;
}

void print_num(int n)
{
printf("%d\n", n);
}

int *alloc_int(void)
{
return malloc(sizeof(int));
}

int main(void)
{
int result = add(5, 3);
print_num(result);
printf("Sum: %d\n", add(1, 2));
free(alloc_int());
return 0;
}
