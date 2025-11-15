#include <stdio.h>

/**
 * factorial - calculates factorial recursively
 * @n: number
 *
 * Return: factorial of n
 */
unsigned long factorial(int n)
{
	if (n <= 1)
		return (1);
	return (n * factorial(n - 1));
}

/**
 * is_prime - checks if number is prime
 * @n: number to check
 *
 * Return: 1 if prime, 0 otherwise
 */
int is_prime(int n)
{
	int i;

	if (n <= 1)
		return (0);

	for (i = 2; i * i <= n; i++)
	{
		if (n % i == 0)
			return (0);
	}

	return (1);
}

/**
 * bubble_sort - sorts array using bubble sort
 * @array: array to sort
 * @size: size of array
 */
void bubble_sort(int *array, int size)
{
	int i, j, temp;
	int swapped;

	if (!array || size <= 1)
		return;

	for (i = 0; i < size - 1; i++)
	{
		swapped = 0;

		for (j = 0; j < size - i - 1; j++)
		{
			if (array[j] > array[j + 1])
			{
				temp = array[j];
				array[j] = array[j + 1];
				array[j + 1] = temp;
				swapped = 1;
			}
		}

		if (!swapped)
			break;
	}
}

/**
 * find_max - finds maximum in array
 * @arr: array
 * @n: size
 *
 * Return: maximum value
 */
int find_max(int arr[], int n)
{
	int max, i;

	if (n <= 0)
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
 * reverse_string - reverses a string in place
 * @str: string to reverse
 */
void reverse_string(char *str)
{
	int left = 0;
	int right;
	char temp;

	if (!str)
		return;

	right = 0;
	while (str[right] != '\0')
		right++;
	right--;

	while (left < right)
	{
		temp = str[left];
		str[left++] = str[right];
		str[right--] = temp;
	}
}
