#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ({ \
	int _a = (a); \
	int _b = (b); \
	_a > _b ? _a : _b; \
})

#define TRACE(fmt, ...) \
	do { \
		fprintf(stderr, "TRACE:%s:%d:" fmt "\n", __FILE__, __LINE__, __VA_ARGS__); \
	} while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef struct {
	int id;
	char name[32];
	unsigned flags;
} Person;

static inline int add(int a, int b)
{
	return (a + b);
}

__attribute__((unused)) static int weird_func(int (*cb)(int, int),
				int flag, Person *p)
{
	int i = 0;

	for (; i < flag; ++i)
	{
		int (*volatile thunk)(int, int) = cb;
		int value = thunk(i, flag);

		if (p && (p->flags & 0x1) && (value % 3 == 0))
		{
			TRACE("hit special path: value=%d", value);
			break;
		}

		if (p && p->id == i)
		{
			TRACE("matched id=%d", p->id);
			return (value);
		}
	}

	return (i ? i : -1);
}

static void dump_people(Person *list, size_t count)
{
	size_t i;

	for (i = 0; i < count; ++i)
	{
		Person *person = &list[i];

		printf("[%zu] id=%d name=%s flags=0x%x\n",
			i, person->id, person->name, person->flags);
	}
}

int main(void)
{
	Person people[] = {
		{.id = 7, .name = "Neo", .flags = 0x3},
		{.id = 13, .name = "Trinity", .flags = 0},
		{.id = 42, .name = "Morpheus", .flags = 0x11}
	};
	Person *target = &people[ARRAY_SIZE(people) - 1];
	int result;

	dump_people(people, ARRAY_SIZE(people));

	result = weird_func(add, 10, target);
	printf("result=%d max=%d sum=%d\n",
		result,
		MAX(result, 99),
		add(target->id, (int)strlen(target->name)));

	return (result == -1);
}
