#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

/**
 * enum SymbolKind - Types of symbols we track
 */
typedef enum SymbolKind
{
	SYM_TYPEDEF,
	SYM_VARIABLE,
	SYM_FUNCTION,
	SYM_STRUCT,
	SYM_ENUM,
	SYM_UNION
} SymbolKind;

/**
 * struct Symbol - A symbol table entry
 * @name: Symbol name (owned, must be freed)
 * @kind: What kind of symbol this is
 * @next: Next symbol in hash chain
 */
typedef struct Symbol
{
	char *name;
	SymbolKind kind;
	struct Symbol *next;
} Symbol;

#define SYMBOL_TABLE_SIZE 256

/**
 * struct SymbolTable - Hash table for symbol lookup
 * @buckets: Hash buckets (array of linked lists)
 * @parent: Parent scope (for nested scopes)
 */
typedef struct SymbolTable
{
	Symbol *buckets[SYMBOL_TABLE_SIZE];
	struct SymbolTable *parent;
} SymbolTable;

/* Symbol table management */
SymbolTable *symbol_table_create(SymbolTable *parent);
void symbol_table_destroy(SymbolTable *table);

/* Symbol operations */
void symbol_add(SymbolTable *table, const char *name, SymbolKind kind);
Symbol *symbol_lookup(SymbolTable *table, const char *name);
int symbol_is_typedef(SymbolTable *table, const char *name);

#endif /* SYMBOL_TABLE_H */
