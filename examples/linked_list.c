#include <stdlib.h>

/**
 * struct node_s - singly linked list node
 * @data: integer data
 * @next: pointer to next node
 */
typedef struct node_s
{
	int data;
	struct node_s *next;
} node_t;

/**
 * create_node - creates a new node
 * @value: value to store
 *
 * Return: pointer to new node, or NULL on failure
 */
node_t *create_node(int value)
{
	node_t *new_node;

	new_node = malloc(sizeof(node_t));
	if (new_node == NULL)
		return (NULL);

	new_node->data = value;
	new_node->next = NULL;
	return (new_node);
}

/**
 * insert_front - inserts node at beginning
 * @head: pointer to head pointer
 * @value: value to insert
 *
 * Return: pointer to new node
 */
node_t *insert_front(node_t **head, int value)
{
	node_t *new_node;

	if (head == NULL)
		return (NULL);

	new_node = create_node(value);
	if (new_node != NULL)
	{
		new_node->next = *head;
		*head = new_node;
	}

	return (new_node);
}
