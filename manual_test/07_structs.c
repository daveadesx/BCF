/* Test 7: Structs and typedefs */
#include <stdio.h>

struct node {
int value;
struct node *next;
};

typedef struct node Node;

typedef struct {
int x;
int y;
} Point;

typedef int (*FuncPtr)(int, int);

int main(void)
{
struct node n;
Node m;
Point p;
return 0;
}
