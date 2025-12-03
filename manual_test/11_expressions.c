/* Test 11: Complex expressions */
#include <stdio.h>

int main(void)
{
int a = 5, b = 10, c = 15;
int result;

result = a + b * c;
result = (a + b) * c;
result = a > b ? a : b;
result = a && b || c;
result = a | b & c;
result = a << 2;
result = b >> 1;
result = ~a;
result = !a;
result = -a;
result = +a;

return 0;
}
