#define _POSIX_C_SOURCE 200809L
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>

/**
 * hash - Simple hash function for symbol names
 * @name: String to hash
 *
 * Return: Hash value (0 to SYMBOL_TABLE_SIZE-1)
 */
static unsigned int hash(const char *name)
{
	unsigned int h = 0;

	while (*name)
	{
		h = h * 31 + (unsigned char)*name;
		name++;
	}

	return (h % SYMBOL_TABLE_SIZE);
}

/**
 * symbol_table_create - Create a new symbol table
 * @parent: Parent scope (NULL for global scope)
 *
 * Return: New symbol table, or NULL on failure
 */
SymbolTable *symbol_table_create(SymbolTable *parent)
{
	SymbolTable *table;
	int i;

	table = malloc(sizeof(SymbolTable));
	if (!table)
		return (NULL);

	for (i = 0; i < SYMBOL_TABLE_SIZE; i++)
		table->buckets[i] = NULL;

	table->parent = parent;

	return (table);
}

/**
 * symbol_table_destroy - Free a symbol table and all its symbols
 * @table: Symbol table to destroy
 *
 * Note: Does NOT destroy parent tables
 */
void symbol_table_destroy(SymbolTable *table)
{
	Symbol *sym, *next;
	int i;

	if (!table)
		return;

	for (i = 0; i < SYMBOL_TABLE_SIZE; i++)
	{
		sym = table->buckets[i];
		while (sym)
		{
			next = sym->next;
			free(sym->name);
			free(sym);
			sym = next;
		}
	}

	free(table);
}

/**
 * symbol_add - Add a symbol to the table
 * @table: Symbol table
 * @name: Symbol name (will be copied)
 * @kind: Kind of symbol
 */
void symbol_add(SymbolTable *table, const char *name, SymbolKind kind)
{
	Symbol *sym;
	unsigned int h;

	if (!table || !name)
		return;

	/* Check if already exists in current scope */
	h = hash(name);
	for (sym = table->buckets[h]; sym; sym = sym->next)
	{
		if (strcmp(sym->name, name) == 0)
			return; /* Already exists, don't duplicate */
	}

	/* Create new symbol */
	sym = malloc(sizeof(Symbol));
	if (!sym)
		return;

	sym->name = strdup(name);
	if (!sym->name)
	{
		free(sym);
		return;
	}

	sym->kind = kind;
	sym->next = table->buckets[h];
	table->buckets[h] = sym;
}

/**
 * symbol_lookup - Look up a symbol by name
 * @table: Symbol table (searches parent scopes too)
 * @name: Symbol name to find
 *
 * Return: Symbol if found, NULL otherwise
 */
Symbol *symbol_lookup(SymbolTable *table, const char *name)
{
	Symbol *sym;
	unsigned int h;

	if (!name)
		return (NULL);

	while (table)
	{
		h = hash(name);
		for (sym = table->buckets[h]; sym; sym = sym->next)
		{
			if (strcmp(sym->name, name) == 0)
				return (sym);
		}
		table = table->parent;
	}

	return (NULL);
}

/**
 * symbol_is_typedef - Check if a name is a typedef'd type
 * @table: Symbol table
 * @name: Name to check
 *
 * Return: 1 if typedef, 0 otherwise
 */
int symbol_is_typedef(SymbolTable *table, const char *name)
{
	Symbol *sym = symbol_lookup(table, name);

	return (sym && sym->kind == SYM_TYPEDEF);
}
