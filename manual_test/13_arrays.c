/* Test 13: Arrays and strings */
#include <stdio.h>

int main(void)
{
int arr[10];
int arr2[5] = {1, 2, 3, 4, 5};
char str[] = "hello";
char *str2 = "world";
int matrix[3][3];
int i;

for (i = 0; i < 10; i++)
arr[i] = i;

arr[0] = arr[1] + arr[2];

return 0;
}
