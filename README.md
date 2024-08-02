# TinyLang

TinyLang is a dynamically typed programming language written in C. It supports basic control flow structures such as while loops, if statements, variable declarations, and printing.

## Requirements

To build and run TinyLang, you will need:
- GCC (GNU Compiler Collection)
- `make` utility

## Building TinyLang

1. Clone the repository:
    ```sh
    git clone https://github.com/yourusername/tinylang.git
    cd tinylang
    ```

2. Build the interpreter:
    ```sh
    make
    ```

## Running TinyLang

To run a TinyLang program, use the following command:
```sh
./interpreter <test.tl>
```
Replace `<test.tl>` with the path to your TinyLang source file.

## TinyLang Syntax

### Variable Declarations

In TinyLang, variables are dynamically typed and can be declared using the `var` keyword:

```plaintext
var seed = 5;
var a = 1664525;
var c = 1013904223;
var m = 4294967296;
```

### If Statements

TinyLang supports standard if statements. You can use them to execute code conditionally:

```plaintext
if (seed <= 10) 
    print("please use a seed larger than 10");
else {
    print("Generating pseudorandom numbers:");
    // Additional statements
};
```

### While Loops

While loops are used to execute a block of code repeatedly as long as a condition is true:

```plaintext
while (count < max) {
    expr seed = (a * seed + c) % m;
    print(seed);
    expr count = count + 1;
};
```

