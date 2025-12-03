/* Test 12: Comments preservation */
#include <stdio.h>

/* This is a block comment */

/**
 * main - Entry point
 * @argc: argument count
 * @argv: argument vector
 *
 * Return: 0 on success
 */
int main(void)
{
int x = 5; /* inline comment */

/* Comment before statement */
x++;

// Single line comment
x--;

return 0; /* return comment */
}
