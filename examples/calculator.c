#include <stdio.h>

#define MAX_VAL 100
#define MIN_VAL -100
#define MULTIPLY(a, b) ((a) * (b))

/**
 * calculate - performs various calculations
 * @a: first operand
 * @b: second operand
 * @op: operation to perform
 *
 * Return: result of calculation
 */
int calculate(int a, int b, char op)
{
	int result = 0;

	switch (op)
	{
		case '+':
			result = a + b;
			break;
		case '-':
			result = a - b;
			break;
		case '*':
			result = a * b;
			break;
		case '/':
			if (b != 0)
				result = a / b;
			break;
		case '%':
			if (b != 0)
				result = a % b;
			break;
		default:
			result = 0;
	}

	/* Clamp result */
	if (result > MAX_VAL)
		result = MAX_VAL;
	else if (result < MIN_VAL)
		result = MIN_VAL;

	return (result);
}

/**
 * bitwise_ops - demonstrates bitwise operations
 * @x: first value
 * @y: second value
 */
void bitwise_ops(unsigned int x, unsigned int y)
{
	unsigned int and, or, xor, not, lshift, rshift;

	and = x & y;        // AND
	or = x | y;         // OR
	xor = x ^ y;        // XOR
	not = ~x;           // NOT
	lshift = x << 2;    // Left shift
	rshift = x >> 2;    // Right shift

	// Compound assignments
	x += 5;
	x -= 3;
	x *= 2;
	x /= 4;
	x %= 10;
	x &= 0xFF;
	x |= 0x10;
	x ^= 0x01;
	x <<= 1;
	x >>= 1;
}

/**
 * compare - compares two values
 * @a: first value
 * @b: second value
 *
 * Return: 1 if equal, 0 otherwise
 */
int compare(int a, int b)
{
	int equal = (a == b);
	int not_equal = (a != b);
	int less = (a < b);
	int greater = (a > b);
	int less_eq = (a <= b);
	int greater_eq = (a >= b);
	int logical_and = (a > 0 && b > 0);
	int logical_or = (a > 0 || b > 0);
	int ternary = (a > b) ? a : b;

	return (equal);
}
