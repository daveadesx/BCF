# Betty-Fmt Parser & Formatter Status

## ✅ Working

### Parser
- Built-in types: `int`, `char`, `void`, `short`, `long`, `float`, `double`, `signed`, `unsigned`
- Type qualifiers: `const`, `volatile`, `static`, `extern`
- Pointers with qualifiers: `char * const ptr`, `const int * const`
- Arrays: `int arr[10]`, `int matrix[3][3]`
- Structs, enums, unions, typedefs (definitions with bodies)
- Functions with parameters and return types
- Function prototypes: `int add(int a, int b);`
- Variadic functions: `int printf(const char *fmt, ...);`
- `va_arg(ap, type)`: Full support for type arguments including pointers
- Control flow: `if/else`, `while`, `for`, `do-while`, `switch/case`
- All operators: arithmetic, logical, bitwise, comparison, assignment
- `sizeof(type)` and `sizeof(expr)`
- Type casts: `(int)x`
- Ternary: `a ? b : c`
- Member access: `ptr->member`, `obj.member`
- Global variable declarations
- Typedef'd types with storage class specifiers: `static node_t *ptr`
- Brace initializers: `= {1, 2, 3}`, `= {{1, 2}, {3, 4}}`
- Function pointers (global): `int (*callback)(int, int);`
- Function pointers (local): `int (*fn)(int);` inside functions
- Function pointer typedefs: `typedef int (*compare_fn)(int, int);`
- Common C library typedefs: `size_t`, `va_list`, `FILE`, `time_t`, etc.
- **Comments**: Block (`/* */`) and line (`//`) comments collected and attached to AST nodes
- **Preprocessor directives**: `#include`, `#define`, `#ifdef`, `#ifndef`, `#else`, `#endif`, etc.

### Formatter
- Tab indentation (Betty style)
- `return (value);` with parentheses
- Spaces around binary operators
- Control flow formatting with proper braces
- Switch/case formatting (case at switch level)
- Function signatures on single line with return type
- Function prototypes with semicolon
- Variadic functions: `...` in parameter list
- `va_arg(ap, type)`: Properly formats type arguments
- Struct/enum bodies with member declarations
- Simple typedefs: `typedef int myint;`
- Pointer typedefs: `typedef char *string_t;`
- Global variable declarations
- Multiple variables on same line: `int i, j, k;`
- Variable initializations: `int x = 0;`
- Array declarations and initializers
- Struct initializers: `struct point p = {0, 0};`
- Function pointers: `int (*fn)(int, int);`
- **Comments**:
  - Leading comments above declarations/statements preserved
  - Trailing comments (same line as code) preserved on same line
  - Multiline block comments preserved (formatting unchanged)
  - C99 line comments (`// ...`) converted to C89 (`/* ... */`)
  - Multiple consecutive comments preserved
  - Comments attached to correct AST nodes (not displaced)
- **Blank lines**:
  - After variable declarations (before first statement)
  - After functions (before any following code)
  - After struct/enum/typedef definitions
  - After global variable declarations
  - After preprocessor blocks (before code)
  - Preserves user-added blank lines (collapses 2+ to 1)
  - No blank lines between consecutive preprocessor directives
- **Preprocessor directives**: Output verbatim, no reformatting

## ⚠️ Partial / Known Limitations

### Parser Limitations (files left unchanged if encountered)
- **GCC attributes**: `__attribute__((unused))`, `__attribute__((format(...)))` not supported
- **Inline struct/union in typedef**: `typedef struct { ... } name;` with body on same line
- **Compound literals**: `(int[]){1, 2, 3}`
- **Designated initializers**: `.field = value`, `[0] = value`
- **Statement expressions**: `({ ... })` (GCC extension)
- **Complex macros**: Some macro patterns may confuse the parser

### Formatter Limitations
- **Multi-line function calls**: Collapses to single line when within 80 chars
- **Array initializer lists**: May collapse multi-line initializers to single line
- **Comments in structs**: Inline member comments may shift position
- **Operator precedence parentheses**: May remove "unnecessary" parens that aid readability

### Graceful Degradation
When the parser encounters unsupported syntax, the file is left **completely unchanged** rather than producing incorrect output. This ensures the formatter never breaks valid code.

## Debug Tools

```bash
./tools/dump_tokens <file>   # Print token stream
./tools/dump_ast <file>      # Print AST tree
./betty-fmt <file>           # Format file to stdout
```

## CLI Options

```bash
./betty-fmt [options] <files...>

Options:
  -i, --in-place      Modify files in place
  -o, --output FILE   Write to FILE instead of stdout
  -c, --check         Check if files are formatted (exit 1 if not)
  -d, --diff          Show unified diff of changes
  -h, --help          Show help message
  -v, --version       Show version

Examples:
  ./betty-fmt main.c                    Print formatted to stdout
  ./betty-fmt -i *.c                    Format all .c files in place
  ./betty-fmt -c src/*.c                Check if files need formatting
  ./betty-fmt --diff file.c             Show what would change
```