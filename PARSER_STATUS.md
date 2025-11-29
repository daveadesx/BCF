# Betty-Fmt Parser & Formatter Status

## ✅ Working

### Parser
- Built-in types: `int`, `char`, `void`, `short`, `long`, `float`, `double`, `signed`, `unsigned`
- Type qualifiers: `const`, `volatile`, `static`, `extern`
- Pointers and arrays: `int *ptr`, `char arr[10]`
- Structs, enums, unions, typedefs
- Functions with parameters
- Control flow: `if/else`, `while`, `for`, `do-while`, `switch/case`
- All operators: arithmetic, logical, bitwise, comparison, assignment
- `sizeof(type)` and `sizeof(expr)`
- Type casts: `(int)x`
- Ternary: `a ? b : c`
- Member access: `ptr->member`, `obj.member`

### Formatter
- Tab indentation (Betty style)
- `return (value);` with parentheses
- Spaces around binary operators
- Control flow formatting with proper braces
- Switch/case formatting
- Function body formatting

## ❌ Not Working

### Parser
- Array initializers: `int arr[] = {1, 2, 3};`
- Function pointers: `int (*fn)(int)`
- Forward declarations (treated as functions)
- Variadic functions: `printf(fmt, ...)`

### Formatter
- Function signatures (only body is formatted)
- Preserving comments
- Preserving preprocessor directives
- Pointer `*` in type declarations

## Debug Tools

```bash
./tools/dump_tokens <file>   # Print token stream
./tools/dump_ast <file>      # Print AST tree
./betty-fmt <file>           # Format file to stdout
```
