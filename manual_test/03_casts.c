/* Test 3: Type casts */
#include <stdio.h>

int main(void)
{
char c = 'A';
int x = (int)c;
unsigned long y = (unsigned long)x;
char *ptr = (char *)malloc(10);
void *vp = (void *)ptr;
unsigned char uc = (unsigned char)x;
return 0;
}
