/* Test 4: Sizeof expressions */
#include <stdio.h>

int main(void)
{
int a = sizeof(int);
int b = sizeof(char *);
int c = sizeof(unsigned long);
int d = sizeof(struct node *);
int e = sizeof(a);
int f = sizeof a;
return 0;
}
