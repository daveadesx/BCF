/* Test 5: Increment/decrement operators */
#include <stdio.h>

int main(void)
{
int i = 0;
int j = 0;

i++;
j--;
++i;
--j;

while (i++ < 10)
	j++;

for (i = 0; i < 10; i++)
	j--;

return 0;
}
