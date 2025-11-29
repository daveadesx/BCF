/**
 * comprehensive_test.c - Comprehensive C file to stress-test the parser
 *
 * This file contains a wide variety of C constructs including:
 * - Preprocessor directives
 * - Typedefs, structs, enums, unions
 * - Function declarations with various signatures
 * - Control flow: if/else, switch/case, for, while, do-while
 * - Expressions: binary, unary, ternary, compound assignment
 * - Pointers, arrays, function pointers
 * - Member access with . and ->
 * - sizeof, casts, complex expressions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Preprocessor constants */
#define MAX_SIZE 1024
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SQUARE(x) ((x) * (x))
#define ABS(x) ((x) < 0 ? -(x) : (x))

/* Forward declarations */
struct node_s;
typedef struct node_s node_t;

/**
 * enum color_e - Color enumeration
 * @RED: Red color
 * @GREEN: Green color
 * @BLUE: Blue color
 * @ALPHA: Alpha channel
 */
typedef enum color_e
{
	RED = 0,
	GREEN = 1,
	BLUE = 2,
	ALPHA = 255
} color_t;

/**
 * enum status_e - Status codes
 */
typedef enum status_e
{
	STATUS_OK,
	STATUS_ERROR,
	STATUS_PENDING,
	STATUS_TIMEOUT
} status_t;

/**
 * struct point_s - 2D point structure
 * @x: X coordinate
 * @y: Y coordinate
 */
typedef struct point_s
{
	int x;
	int y;
} point_t;

/**
 * struct rect_s - Rectangle structure
 * @origin: Top-left corner
 * @width: Width of rectangle
 * @height: Height of rectangle
 */
typedef struct rect_s
{
	point_t origin;
	unsigned int width;
	unsigned int height;
} rect_t;

/**
 * struct node_s - Linked list node
 * @data: Integer data
 * @next: Pointer to next node
 * @prev: Pointer to previous node
 */
struct node_s
{
	int data;
	struct node_s *next;
	struct node_s *prev;
};

/**
 * struct tree_s - Binary tree node
 * @value: Node value
 * @left: Left child
 * @right: Right child
 * @parent: Parent node
 */
typedef struct tree_s
{
	int value;
	struct tree_s *left;
	struct tree_s *right;
	struct tree_s *parent;
} tree_t;

/**
 * union data_u - Union for different data types
 * @i: Integer value
 * @f: Float value
 * @c: Character array
 * @ptr: Generic pointer
 */
typedef union data_u
{
	int i;
	float f;
	char c[4];
	void *ptr;
} data_t;

/**
 * struct variant_s - Tagged union / variant type
 * @type: Type tag (0=int, 1=float, 2=string)
 * @data: Actual data
 */
typedef struct variant_s
{
	int type;
	data_t data;
} variant_t;

/* Global variables - simplified for parser testing */
static int g_counter;
static node_t *g_head;

/**
 * simple_add - Add two integers
 * @a: First operand
 * @b: Second operand
 *
 * Return: Sum of a and b
 */
int simple_add(int a, int b)
{
	return (a + b);
}

/**
 * simple_subtract - Subtract two integers
 * @a: First operand
 * @b: Second operand
 *
 * Return: Difference of a and b
 */
int simple_subtract(int a, int b)
{
	return (a - b);
}

/**
 * simple_multiply - Multiply two integers
 * @a: First operand
 * @b: Second operand
 *
 * Return: Product of a and b
 */
int simple_multiply(int a, int b)
{
	return (a * b);
}

/**
 * safe_divide - Divide with zero check
 * @a: Dividend
 * @b: Divisor
 *
 * Return: Quotient, or 0 if b is zero
 */
int safe_divide(int a, int b)
{
	if (b == 0)
		return (0);
	return (a / b);
}

/**
 * factorial_iterative - Calculate factorial iteratively
 * @n: Input number
 *
 * Return: n factorial
 */
unsigned long factorial_iterative(unsigned int n)
{
	unsigned long result = 1;
	unsigned int i;

	for (i = 2; i <= n; i++)
	{
		result *= i;
	}

	return (result);
}

/**
 * factorial_recursive - Calculate factorial recursively
 * @n: Input number
 *
 * Return: n factorial
 */
unsigned long factorial_recursive(unsigned int n)
{
	if (n <= 1)
		return (1);
	return (n * factorial_recursive(n - 1));
}

/**
 * fibonacci - Calculate nth Fibonacci number
 * @n: Index in Fibonacci sequence
 *
 * Return: nth Fibonacci number
 */
unsigned long fibonacci(unsigned int n)
{
	unsigned long a = 0, b = 1, temp;
	unsigned int i;

	if (n == 0)
		return (0);
	if (n == 1)
		return (1);

	for (i = 2; i <= n; i++)
	{
		temp = a + b;
		a = b;
		b = temp;
	}

	return (b);
}

/**
 * is_prime - Check if a number is prime
 * @n: Number to check
 *
 * Return: 1 if prime, 0 otherwise
 */
int is_prime(unsigned int n)
{
	unsigned int i;

	if (n <= 1)
		return (0);
	if (n <= 3)
		return (1);
	if (n % 2 == 0 || n % 3 == 0)
		return (0);

	for (i = 5; i * i <= n; i += 6)
	{
		if (n % i == 0 || n % (i + 2) == 0)
			return (0);
	}

	return (1);
}

/**
 * gcd - Greatest common divisor using Euclidean algorithm
 * @a: First number
 * @b: Second number
 *
 * Return: GCD of a and b
 */
unsigned int gcd(unsigned int a, unsigned int b)
{
	unsigned int temp;

	while (b != 0)
	{
		temp = b;
		b = a % b;
		a = temp;
	}

	return (a);
}

/**
 * lcm - Least common multiple
 * @a: First number
 * @b: Second number
 *
 * Return: LCM of a and b
 */
unsigned int lcm(unsigned int a, unsigned int b)
{
	return ((a / gcd(a, b)) * b);
}

/**
 * power - Calculate base raised to exponent
 * @base: Base number
 * @exp: Exponent
 *
 * Return: base^exp
 */
long power(int base, unsigned int exp)
{
	long result = 1;

	while (exp > 0)
	{
		if (exp & 1)
			result *= base;
		base *= base;
		exp >>= 1;
	}

	return (result);
}

/**
 * swap_int - Swap two integers
 * @a: Pointer to first integer
 * @b: Pointer to second integer
 */
void swap_int(int *a, int *b)
{
	int temp;

	if (a == NULL || b == NULL)
		return;

	temp = *a;
	*a = *b;
	*b = temp;
}

/**
 * swap_xor - Swap without temporary variable using XOR
 * @a: Pointer to first integer
 * @b: Pointer to second integer
 */
void swap_xor(int *a, int *b)
{
	if (a == NULL || b == NULL || a == b)
		return;

	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

/**
 * bubble_sort - Sort array using bubble sort
 * @arr: Array to sort
 * @n: Number of elements
 */
void bubble_sort(int *arr, int n)
{
	int i, j, swapped;

	if (arr == NULL || n <= 1)
		return;

	for (i = 0; i < n - 1; i++)
	{
		swapped = 0;
		for (j = 0; j < n - i - 1; j++)
		{
			if (arr[j] > arr[j + 1])
			{
				swap_int(&arr[j], &arr[j + 1]);
				swapped = 1;
			}
		}
		if (!swapped)
			break;
	}
}

/**
 * selection_sort - Sort array using selection sort
 * @arr: Array to sort
 * @n: Number of elements
 */
void selection_sort(int *arr, int n)
{
	int i, j, min_idx;

	if (arr == NULL || n <= 1)
		return;

	for (i = 0; i < n - 1; i++)
	{
		min_idx = i;
		for (j = i + 1; j < n; j++)
		{
			if (arr[j] < arr[min_idx])
				min_idx = j;
		}
		if (min_idx != i)
			swap_int(&arr[i], &arr[min_idx]);
	}
}

/**
 * insertion_sort - Sort array using insertion sort
 * @arr: Array to sort
 * @n: Number of elements
 */
void insertion_sort(int *arr, int n)
{
	int i, j, key;

	if (arr == NULL || n <= 1)
		return;

	for (i = 1; i < n; i++)
	{
		key = arr[i];
		j = i - 1;

		while (j >= 0 && arr[j] > key)
		{
			arr[j + 1] = arr[j];
			j--;
		}
		arr[j + 1] = key;
	}
}

/**
 * binary_search - Search for value in sorted array
 * @arr: Sorted array
 * @n: Number of elements
 * @target: Value to find
 *
 * Return: Index of target, or -1 if not found
 */
int binary_search(int *arr, int n, int target)
{
	int left = 0, right = n - 1, mid;

	if (arr == NULL || n <= 0)
		return (-1);

	while (left <= right)
	{
		mid = left + (right - left) / 2;

		if (arr[mid] == target)
			return (mid);
		else if (arr[mid] < target)
			left = mid + 1;
		else
			right = mid - 1;
	}

	return (-1);
}

/**
 * linear_search - Search for value in array
 * @arr: Array to search
 * @n: Number of elements
 * @target: Value to find
 *
 * Return: Index of target, or -1 if not found
 */
int linear_search(int *arr, int n, int target)
{
	int i;

	if (arr == NULL || n <= 0)
		return (-1);

	for (i = 0; i < n; i++)
	{
		if (arr[i] == target)
			return (i);
	}

	return (-1);
}

/**
 * reverse_array - Reverse an array in place
 * @arr: Array to reverse
 * @n: Number of elements
 */
void reverse_array(int *arr, int n)
{
	int i, j;

	if (arr == NULL || n <= 1)
		return;

	for (i = 0, j = n - 1; i < j; i++, j--)
	{
		swap_int(&arr[i], &arr[j]);
	}
}

/**
 * rotate_array - Rotate array by k positions
 * @arr: Array to rotate
 * @n: Number of elements
 * @k: Number of positions to rotate
 */
void rotate_array(int *arr, int n, int k)
{
	if (arr == NULL || n <= 1)
		return;

	k = k % n;
	if (k < 0)
		k += n;

	reverse_array(arr, n);
	reverse_array(arr, k);
	reverse_array(arr + k, n - k);
}

/**
 * array_sum - Calculate sum of array elements
 * @arr: Array
 * @n: Number of elements
 *
 * Return: Sum of all elements
 */
long array_sum(int *arr, int n)
{
	long sum = 0;
	int i;

	if (arr == NULL || n <= 0)
		return (0);

	for (i = 0; i < n; i++)
	{
		sum += arr[i];
	}

	return (sum);
}

/**
 * array_max - Find maximum element
 * @arr: Array
 * @n: Number of elements
 *
 * Return: Maximum value
 */
int array_max(int *arr, int n)
{
	int max, i;

	if (arr == NULL || n <= 0)
		return (0);

	max = arr[0];
	for (i = 1; i < n; i++)
	{
		if (arr[i] > max)
			max = arr[i];
	}

	return (max);
}

/**
 * array_min - Find minimum element
 * @arr: Array
 * @n: Number of elements
 *
 * Return: Minimum value
 */
int array_min(int *arr, int n)
{
	int min, i;

	if (arr == NULL || n <= 0)
		return (0);

	min = arr[0];
	for (i = 1; i < n; i++)
	{
		if (arr[i] < min)
			min = arr[i];
	}

	return (min);
}

/**
 * create_node - Create a new linked list node
 * @data: Data for the node
 *
 * Return: Pointer to new node, or NULL on failure
 */
node_t *create_node(int data)
{
	node_t *node;

	node = malloc(sizeof(node_t));
	if (node == NULL)
		return (NULL);

	node->data = data;
	node->next = NULL;
	node->prev = NULL;

	return (node);
}

/**
 * list_push_front - Add node at beginning of list
 * @head: Pointer to head pointer
 * @data: Data for new node
 *
 * Return: Pointer to new node
 */
node_t *list_push_front(node_t **head, int data)
{
	node_t *node;

	if (head == NULL)
		return (NULL);

	node = create_node(data);
	if (node == NULL)
		return (NULL);

	node->next = *head;
	if (*head != NULL)
		(*head)->prev = node;
	*head = node;

	return (node);
}

/**
 * list_push_back - Add node at end of list
 * @head: Pointer to head pointer
