/* Test 15: Enums */
#include <stdio.h>

enum color {
RED,
GREEN,
BLUE
};

enum size {
SMALL = 1,
MEDIUM = 5,
LARGE = 10
};

typedef enum {
OFF,
ON
} State;

int main(void)
{
enum color c = RED;
enum size s = MEDIUM;
State st = ON;

return 0;
}
