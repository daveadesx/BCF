/* Test 10: Preprocessor directives */
#ifndef HEADER_H
#define HEADER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX 100
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#ifdef DEBUG
#define LOG(x) printf("%s\n", x)
#else
#define LOG(x)
#endif
int main(void)
{
#ifdef DEBUG
printf("debug mode\n");
#endif
return 0;
}
#endif
