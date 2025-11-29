/**
 * test_minimal.c - Minimal test to find parser issues
 */

#include <stdio.h>

/* Simple enum */
typedef enum color_e
{
	RED = 0,
	GREEN = 1,
	BLUE = 2
} color_t;

/* Simple struct */
typedef struct point_s
{
	int x;
	int y;
} point_t;

/* Simple union */
typedef union data_u
{
	int i;
	float f;
} data_t;

/* Global variable */
static int g_counter;

/**
 * add - Add two numbers
 * @a: First number
 * @b: Second number
 *
 * Return: Sum
 */
int add(int a, int b)
{
	return (a + b);
}

/**
 * factorial - Calculate factorial
 * @n: Input
 *
 * Return: n!
 */
int factorial(int n)
{
	if (n <= 1)
		return (1);
	return (n * factorial(n - 1));
}

/**
 * test_loops - Test loop constructs
 * @n: Iteration count
 *
 * Return: Sum
 */
int test_loops(int n)
{
	int i, sum = 0;

	for (i = 0; i < n; i++)
	{
		sum += i;
	}

	i = 0;
	while (i < n)
	{
		sum += i;
		i++;
	}

	i = 0;
	do
	{
		sum += i;
		i++;
	} while (i < n);

	return (sum);
}

/**
 * test_switch - Test switch statement
 * @op: Operation
 * @a: First operand
 * @b: Second operand
 *
 * Return: Result
 */
int test_switch(int op, int a, int b)
{
	int result = 0;

	switch (op)
	{
	case 0:
		result = a + b;
		break;
	case 1:
		result = a - b;
		break;
	case 2:
		result = a * b;
		break;
	default:
		result = 0;
		break;
	}

	return (result);
}

/**
 * test_ternary - Test ternary operator
 * @a: First value
 * @b: Second value
 *
 * Return: Max of a and b
 */
int test_ternary(int a, int b)
{
	int max = a > b ? a : b;
	int min = a < b ? a : b;

	return (max - min);
}

/**
 * test_bitwise - Test bitwise operations
 * @a: First operand
 * @b: Second operand
 *
 * Return: Result
 */
int test_bitwise(int a, int b)
{
	int result;

	result = a & b;
	result |= a ^ b;
	result &= ~a;
	result <<= 1;
	result >>= 1;

	return (result);
}

/**
 * main - Entry point
 *
 * Return: 0
 */
int main(void)
{
	int x = 5;
	int y = 10;
	int result;

	result = add(x, y);
	result = factorial(5);
	result = test_loops(10);
	result = test_switch(2, x, y);
	result = test_ternary(x, y);
	result = test_bitwise(x, y);

	printf("Result: %d\n", result);

	return (0);
}
