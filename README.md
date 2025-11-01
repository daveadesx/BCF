# Betty Code Formatter

A C code formatter that outputs Betty-compliant code.

## Overview

This project implements a complete C code formatter from scratch using a proper compiler-style architecture:
- **Lexer**: Tokenizes C source code
- **Parser**: Builds an Abstract Syntax Tree (AST)
- **Formatter**: Outputs Betty-style formatted code

## Features

- Full C language lexing (keywords, operators, literals, comments, preprocessor)
- Recursive descent parser with operator precedence
- Support for structs, typedefs, and enums
- Comprehensive expression parsing
- Control flow statements (if/while/for)

## Building

```bash
make
```

## Usage

```bash
./betty-fmt input.c
./betty-fmt input.c --output formatted.c
```

## Testing

```bash
make test-unit
```

## Architecture

The formatter uses a three-phase approach to avoid position shifting issues:

1. **Lexer** → Tokens (preserving all whitespace and comments)
2. **Parser** → Abstract Syntax Tree
3. **Formatter** → Betty-compliant output

This ensures that formatting decisions are made on the AST structure, not raw text positions.
