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
 * @data: Data for new node
 *
 * Return: Pointer to new node
 */
node_t *list_push_back(node_t **head, int data)
{
	node_t *node, *current;

	if (head == NULL)
		return (NULL);

	node = create_node(data);
	if (node == NULL)
		return (NULL);

	if (*head == NULL)
	{
		*head = node;
		return (node);
	}

	current = *head;
	while (current->next != NULL)
	{
		current = current->next;
	}

	current->next = node;
	node->prev = current;

	return (node);
}

/**
 * list_pop_front - Remove node from beginning
 * @head: Pointer to head pointer
 *
 * Return: Data from removed node, or 0 if empty
 */
int list_pop_front(node_t **head)
{
	node_t *temp;
	int data;

	if (head == NULL || *head == NULL)
		return (0);

	temp = *head;
	data = temp->data;
	*head = temp->next;

	if (*head != NULL)
		(*head)->prev = NULL;

	free(temp);
	return (data);
}

/**
 * list_length - Get length of list
 * @head: Head of list
 *
 * Return: Number of nodes
 */
int list_length(node_t *head)
{
	int count = 0;

	while (head != NULL)
	{
		count++;
		head = head->next;
	}

	return (count);
}

/**
 * list_find - Find node with given data
 * @head: Head of list
 * @data: Data to find
 *
 * Return: Pointer to node, or NULL if not found
 */
node_t *list_find(node_t *head, int data)
{
	while (head != NULL)
	{
		if (head->data == data)
			return (head);
		head = head->next;
	}

	return (NULL);
}

/**
 * list_delete - Delete node with given data
 * @head: Pointer to head pointer
 * @data: Data of node to delete
 *
 * Return: 1 if deleted, 0 if not found
 */
int list_delete(node_t **head, int data)
{
	node_t *current, *temp;

	if (head == NULL || *head == NULL)
		return (0);

	current = *head;

	/* Check head node */
	if (current->data == data)
	{
		*head = current->next;
		if (*head != NULL)
			(*head)->prev = NULL;
		free(current);
		return (1);
	}

	/* Search rest of list */
	while (current != NULL && current->data != data)
	{
		current = current->next;
	}

	if (current == NULL)
		return (0);

	/* Unlink and free */
	if (current->prev != NULL)
		current->prev->next = current->next;
	if (current->next != NULL)
		current->next->prev = current->prev;

	free(current);
	return (1);
}

/**
 * list_reverse - Reverse a linked list
 * @head: Pointer to head pointer
 */
void list_reverse(node_t **head)
{
	node_t *current, *temp;

	if (head == NULL || *head == NULL)
		return;

	current = *head;
	while (current != NULL)
	{
		temp = current->prev;
		current->prev = current->next;
		current->next = temp;
		current = current->prev;
	}

	if (temp != NULL)
		*head = temp->prev;
}

/**
 * list_free - Free all nodes in list
 * @head: Pointer to head pointer
 */
void list_free(node_t **head)
{
	node_t *current, *next;

	if (head == NULL)
		return;

	current = *head;
	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}

	*head = NULL;
}

/**
 * create_tree_node - Create a new tree node
 * @value: Value for the node
 *
 * Return: Pointer to new node
 */
tree_t *create_tree_node(int value)
{
	tree_t *node;

	node = malloc(sizeof(tree_t));
	if (node == NULL)
		return (NULL);

	node->value = value;
	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;

	return (node);
}

/**
 * tree_insert - Insert value into BST
 * @root: Pointer to root pointer
 * @value: Value to insert
 *
 * Return: Pointer to inserted node
 */
tree_t *tree_insert(tree_t **root, int value)
{
	tree_t *node, *current, *parent;

	if (root == NULL)
		return (NULL);

	node = create_tree_node(value);
	if (node == NULL)
		return (NULL);

	if (*root == NULL)
	{
		*root = node;
		return (node);
	}

	current = *root;
	parent = NULL;

	while (current != NULL)
	{
		parent = current;
		if (value < current->value)
			current = current->left;
		else if (value > current->value)
			current = current->right;
		else
		{
			free(node);
			return (current);
		}
	}

	node->parent = parent;
	if (value < parent->value)
		parent->left = node;
	else
		parent->right = node;

	return (node);
}

/**
 * tree_find - Find value in BST
 * @root: Root of tree
 * @value: Value to find
 *
 * Return: Pointer to node, or NULL if not found
 */
tree_t *tree_find(tree_t *root, int value)
{
	while (root != NULL)
	{
		if (value == root->value)
			return (root);
		else if (value < root->value)
			root = root->left;
		else
			root = root->right;
	}

	return (NULL);
}

/**
 * tree_min - Find minimum value in tree
 * @root: Root of tree
 *
 * Return: Pointer to node with minimum value
 */
tree_t *tree_min(tree_t *root)
{
	if (root == NULL)
		return (NULL);

	while (root->left != NULL)
		root = root->left;

	return (root);
}

/**
 * tree_max - Find maximum value in tree
 * @root: Root of tree
 *
 * Return: Pointer to node with maximum value
 */
tree_t *tree_max(tree_t *root)
{
	if (root == NULL)
		return (NULL);

	while (root->right != NULL)
		root = root->right;

	return (root);
}

/**
 * tree_height - Calculate height of tree
 * @root: Root of tree
 *
 * Return: Height of tree
 */
int tree_height(tree_t *root)
{
	int left_height, right_height;

	if (root == NULL)
		return (-1);

	left_height = tree_height(root->left);
	right_height = tree_height(root->right);

	return (1 + (left_height > right_height ? left_height : right_height));
}

/**
 * tree_size - Count nodes in tree
 * @root: Root of tree
 *
 * Return: Number of nodes
 */
int tree_size(tree_t *root)
{
	if (root == NULL)
		return (0);

	return (1 + tree_size(root->left) + tree_size(root->right));
}

/**
 * tree_free - Free all nodes in tree
 * @root: Pointer to root pointer
 */
void tree_free(tree_t **root)
{
	if (root == NULL || *root == NULL)
		return;

	tree_free(&((*root)->left));
	tree_free(&((*root)->right));
	free(*root);
	*root = NULL;
}

/**
 * point_distance_sq - Calculate squared distance between points
 * @p1: First point
 * @p2: Second point
 *
 * Return: Squared distance
 */
int point_distance_sq(point_t p1, point_t p2)
{
	int dx = p2.x - p1.x;
	int dy = p2.y - p1.y;

	return (dx * dx + dy * dy);
}

/**
 * rect_area - Calculate area of rectangle
 * @r: Rectangle
 *
 * Return: Area
 */
unsigned int rect_area(rect_t r)
{
	return (r.width * r.height);
}

/**
 * rect_perimeter - Calculate perimeter of rectangle
 * @r: Rectangle
 *
 * Return: Perimeter
 */
unsigned int rect_perimeter(rect_t r)
{
	return (2 * (r.width + r.height));
}

/**
 * rect_contains_point - Check if rectangle contains point
 * @r: Rectangle
 * @p: Point
 *
 * Return: 1 if contains, 0 otherwise
 */
int rect_contains_point(rect_t r, point_t p)
{
	return (p.x >= r.origin.x &&
		p.x < r.origin.x + (int)r.width &&
		p.y >= r.origin.y &&
		p.y < r.origin.y + (int)r.height);
}

/**
 * rect_intersects - Check if two rectangles intersect
 * @r1: First rectangle
 * @r2: Second rectangle
 *
 * Return: 1 if intersect, 0 otherwise
 */
int rect_intersects(rect_t r1, rect_t r2)
{
	return (r1.origin.x < r2.origin.x + (int)r2.width &&
		r1.origin.x + (int)r1.width > r2.origin.x &&
		r1.origin.y < r2.origin.y + (int)r2.height &&
		r1.origin.y + (int)r1.height > r2.origin.y);
}

/**
 * bitwise_operations - Demonstrate bitwise operations
 * @a: First operand
 * @b: Second operand
 */
void bitwise_operations(unsigned int a, unsigned int b)
{
	unsigned int and_result, or_result, xor_result;
	unsigned int not_a, left_shift, right_shift;
	unsigned int set_bit, clear_bit, toggle_bit, check_bit;
	int bit_pos = 3;

	and_result = a & b;
	or_result = a | b;
	xor_result = a ^ b;
	not_a = ~a;
	left_shift = a << 2;
	right_shift = a >> 2;

	/* Bit manipulation */
	set_bit = a | (1 << bit_pos);
	clear_bit = a & ~(1 << bit_pos);
	toggle_bit = a ^ (1 << bit_pos);
	check_bit = (a >> bit_pos) & 1;

	/* Compound assignments */
	a &= 0xFF;
	a |= 0x80;
	a ^= 0x55;
	a <<= 1;
	a >>= 1;
}

/**
 * compound_assignments - Demonstrate compound assignment operators
 * @x: Input value
 */
void compound_assignments(int x)
{
	int a = x;

	a += 10;
	a -= 5;
	a *= 2;
	a /= 3;
	a %= 7;
	a &= 0xFF;
	a |= 0x80;
	a ^= 0xAA;
	a <<= 2;
	a >>= 1;
}

/**
 * ternary_expressions - Demonstrate ternary operator
 * @a: First value
 * @b: Second value
 *
 * Return: Result of various ternary operations
 */
int ternary_expressions(int a, int b)
{
	int max = a > b ? a : b;
	int min = a < b ? a : b;
	int abs_diff = a > b ? a - b : b - a;
	int sign = a > 0 ? 1 : (a < 0 ? -1 : 0);
	int clamped = a < 0 ? 0 : (a > 100 ? 100 : a);

	return (max + min + abs_diff + sign + clamped);
}

/**
 * switch_demo - Demonstrate switch statement
 * @op: Operation code
 * @a: First operand
 * @b: Second operand
 *
 * Return: Result of operation
 */
int switch_demo(int op, int a, int b)
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
	case 3:
		if (b != 0)
			result = a / b;
		break;
	case 4:
		if (b != 0)
			result = a % b;
		break;
	case 5:
	case 6:
	case 7:
		result = a & b;
		break;
	default:
		result = 0;
		break;
	}

	return (result);
}

/**
 * loop_demo - Demonstrate various loop constructs
 * @n: Iteration count
 *
 * Return: Sum computed by loops
 */
int loop_demo(int n)
{
	int i, j, sum = 0;
	int arr[10];

	/* For loop */
	for (i = 0; i < n; i++)
	{
		sum += i;
	}

	/* While loop */
	i = 0;
	while (i < n)
	{
		sum += i * 2;
		i++;
	}

	/* Do-while loop */
	i = 0;
	do
	{
		sum += i * 3;
		i++;
	} while (i < n);

	/* Nested loops */
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 5; j++)
		{
			sum += i * j;
		}
	}

	/* Loop with break and continue */
	for (i = 0; i < 100; i++)
	{
		if (i % 2 == 0)
			continue;
		if (i > 50)
			break;
		sum += i;
	}

	/* Enhanced for-style with arrays */
	for (i = 0; i < 10; i++)
	{
		sum += arr[i];
	}

	return (sum);
}

/**
 * pointer_arithmetic - Demonstrate pointer operations
 * @arr: Array to process
 * @n: Number of elements
 *
 * Return: Sum using pointer arithmetic
 */
int pointer_arithmetic(int *arr, int n)
{
	int *ptr, *end;
	int sum = 0;

	if (arr == NULL || n <= 0)
		return (0);

	ptr = arr;
	end = arr + n;

	while (ptr < end)
	{
		sum += *ptr;
		ptr++;
	}

	/* More pointer arithmetic */
	ptr = arr;
	sum += *(ptr + 0);
	sum += *(ptr + 1);
	sum += ptr[2];
	sum += 3[ptr];

	/* Pointer difference */
	n = end - arr;

	return (sum);
}

/**
 * string_length - Calculate string length
 * @str: String to measure
 *
 * Return: Length of string
 */
int string_length(const char *str)
{
	const char *ptr;

	if (str == NULL)
		return (0);

	ptr = str;
	while (*ptr != '\0')
		ptr++;

	return (ptr - str);
}

/**
 * string_copy - Copy string
 * @dest: Destination buffer
 * @src: Source string
 *
 * Return: Pointer to destination
 */
char *string_copy(char *dest, const char *src)
{
	char *ptr;

	if (dest == NULL || src == NULL)
		return (NULL);

	ptr = dest;
	while ((*ptr++ = *src++) != '\0')
		;

	return (dest);
}

/**
 * string_compare - Compare two strings
 * @s1: First string
 * @s2: Second string
 *
 * Return: 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int string_compare(const char *s1, const char *s2)
{
	if (s1 == NULL && s2 == NULL)
		return (0);
	if (s1 == NULL)
		return (-1);
	if (s2 == NULL)
		return (1);

	while (*s1 != '\0' && *s1 == *s2)
	{
		s1++;
		s2++;
	}

	return ((unsigned char)*s1 - (unsigned char)*s2);
}

/**
 * apply_function - Apply function to array elements
 * @arr: Array to process
 * @n: Number of elements
 * @context: Context pointer for callback
 */
void apply_function(int *arr, int n, void *context)
{
	int i;

	if (arr == NULL || n <= 0)
		return;

	for (i = 0; i < n; i++)
	{
		arr[i] = arr[i] + 1;
	}
	(void)context;
}

/**
 * compare_int - Compare function for integers
 * @a: First value
 * @b: Second value
 *
 * Return: Comparison result
 */
int compare_int(int a, int b)
{
	return (a - b);
}

/**
 * compare_int_desc - Compare function for descending order
 * @a: First value
 * @b: Second value
 *
 * Return: Comparison result (reversed)
 */
int compare_int_desc(int a, int b)
{
	return (-compare_int(a, b));
}

/**
 * complex_expression_demo - Demonstrate complex expressions
 * @x: Input value
 *
 * Return: Result of complex calculations
 */
int complex_expression_demo(int x)
{
	int a, b, c, result;
	int arr[5];

	/* Complex arithmetic */
	a = (x + 5) * 3 - (x / 2);
	b = ((x << 2) | 0x0F) & 0xFF;
	c = x > 0 ? x * x : -x * x;

	/* Chained comparisons (split for C) */
	result = (x > 0) && (x < 100) && (x != 50);

	/* Array and pointer expressions */
	result += arr[x % 5] + *(arr + (x % 5));

	/* Logical expressions */
	result += (a && b) || (!c && (a > b));

	/* Increment/decrement in expressions */
	result += ++a + b-- + --c;

	/* Sizeof expressions */
	result += sizeof(int) + sizeof(arr) + sizeof(x);

	return (result);
}

/**
 * main - Main entry point
 *
 * Return: 0 on success
 */
int main(void)
{
	int arr[10];
	int n = sizeof(arr) / sizeof(arr[0]);
	node_t *list = NULL;
	tree_t *tree = NULL;
	point_t p1;
	point_t p2;
	rect_t r;
	int i, result;

	/* Test sorting */
	bubble_sort(arr, n);

	/* Test searching */
	result = binary_search(arr, n, 5);

	/* Test linked list */
	for (i = 0; i < 10; i++)
	{
		list_push_back(&list, i * 10);
	}
	list_reverse(&list);
	list_free(&list);

	/* Test tree */
	for (i = 0; i < 10; i++)
	{
		tree_insert(&tree, arr[i]);
	}
	result = tree_height(tree);
	tree_free(&tree);

	/* Test geometry */
	result = point_distance_sq(p1, p2);
	result = rect_area(r);
	result = rect_contains_point(r, p1);

	/* Test various demos */
	result = ternary_expressions(10, 20);
	result = switch_demo(2, 5, 3);
	result = loop_demo(10);
	result = complex_expression_demo(42);

	/* Test math functions */
	result = factorial_iterative(10);
	result = fibonacci(20);
	result = is_prime(97);
	result = gcd(48, 18);

	printf("All tests completed successfully!\n");

	return (0);
}
