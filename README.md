# Aspic

![workflow](https://github.com/abodelot/aspic/actions/workflows/ci.yml/badge.svg)

## How to compile

Install dependencies:

    sudo apt install libreadline-dev

Compile with make:

    make

Both `clang` (default) and `gcc` are supported. Use `make CC=gcc` to override C compiler.

## How to run

You can start a REPL session:

    ./aspic

Or execute a program:

    ./aspic <path>

## Tests

Run tests with `./spec.sh`

## Credits

- Inspired by [Crafting Interpreters](https://craftinginterpreters.com/), from [Bob Nystrom](https://github.com/munificent)
