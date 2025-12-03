/* Test 6: Pointers and double pointers */
#include <stdio.h>

void func(int *ptr, int **dptr, char ***tptr)
{
int *a;
int **b;
char ***c;
int *arr[10];

*ptr = 5;
**dptr = 10;
***tptr = 'x';
}

int main(void)
{
return 0;
}
