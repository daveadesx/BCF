/* Test 8: Control flow */
#include <stdio.h>

int main(void)
{
int x = 5;

if (x > 0)
{
printf("positive\n");
}
else if (x < 0)
{
printf("negative\n");
}
else
{
printf("zero\n");
}

while (x > 0)
{
x--;
}

for (x = 0; x < 10; x++)
{
printf("%d\n", x);
}

do {
x++;
} while (x < 5);

switch (x)
{
case 0:
printf("zero\n");
break;
case 1:
printf("one\n");
break;
default:
printf("other\n");
break;
}

return 0;
}
