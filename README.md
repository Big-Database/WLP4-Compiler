# WLP4 Compiler

## Introduction to WLP4

WLP4 (pronounced *“Wool P Four”*) is a restricted subset of C++ whose name stands for **“Waterloo, Language, Plus, Pointers, Plus, Procedures.”** The language supports core imperative programming constructs, including procedures, integers, pointers, arrays, conditional statements, and while loops. WLP4 is taught as part of the University of Waterloo course **CS 241: Foundations of Sequential Programs**.

For additional background on WLP4, see:
- [WLP4 Programming Language Tutorial](https://www.student.cs.uwaterloo.ca/~cs241/wlp4/WLP4tutorial.html)
- [WLP4 Programming Language Specification](https://www.student.cs.uwaterloo.ca/~cs241/wlp4/WLP4.html)

---

## WLP4 Compiler

This project implements a full **WLP4 compiler**, including lexical analysis, parsing, context-sensitive analysis, and code generation. The compiler translates valid WLP4 source programs into **MIPS assembly language**, following the formal specifications provided by the course.

The source code for this project is not publicly available. If you would like to view the implementation, please message me on **GitHub** or email me at [rminocha@uwaterloo.ca](mailto:rminocha@uwaterloo.ca).

---

## Compiler Pipeline

### 1) Scanning (Lexical Analysis)

The first stage of compilation performs tokenization of the input WLP4 program. Scanning is implemented using a **simplified Maximal Munch algorithm**, which repeatedly consumes input characters until no further progress can be made. At that point, the scanner checks whether the consumed input corresponds to an accepting state. If so, a token is produced; otherwise, the input is rejected as invalid.

The scanner operates over a **nondeterministic finite automaton (NFA)** constructed from the lexical specification of WLP4. The complete lexical syntax is defined in the official language specification.

<!-- A3 -->

---

### 2) Parsing

Parsing determines whether the token stream conforms to the **context-free grammar (CFG)** of WLP4. This compiler uses a **pushdown automaton–based bottom-up parsing strategy**, specifically an **SLR(1) parser**.

Successful parsing produces a **derivation** of the input program, which uniquely defines a **parse tree** representing the syntactic structure of the program. The CFG used for parsing is taken directly from the WLP4 language specification.

<!-- A5 -->

---

### 3) Context-Sensitive Analysis

Following parsing, the compiler performs **context-sensitive (semantic) analysis** to verify that the program satisfies WLP4’s semantic and type rules. Examples of such rules include:
- Preventing multiple declarations of the same identifier within a scope
- Disallowing the use of variables before they are declared

The complete set of semantic and type-inference rules is defined in the official WLP4 documentation. This phase traverses the parse tree to enforce these rules and reject semantically invalid programs.

<!-- A6 -->

---

### 4) Code Generation

The final stage of the compiler generates **MIPS assembly code** corresponding to the validated WLP4 program. The compiler supports code generation for:
- The `main` procedure
- Integer variables and constants
- Variable declarations and assignments
- Arithmetic expressions
- Control flow constructs
- Input/output (printing)

Additional features include support for pointers, multiple procedures, and dynamic memory allocation.

<!-- A7 / A8 -->
