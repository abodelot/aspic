#ifndef ASPIC_CFUNC_H
#define ASPIC_CFUNC_H

#include "value.h"

/**
 * Return error in argument is not truthy
 * @return true, or error
 */
Value aspic_assert(Value* argv, int argc);

/**
 * Return length of given string
 * @param 1: string
 * @return int
 */
Value aspic_len(Value* argv, int argc);

/**
 * Get user input from stdin
 * @param 1: prompt
 * @return string
 */
Value aspic_input(Value* argv, int argc);

/**
 * Print given arguments, each separated by a space.
 * @param 1-N: list of arguments to print
 * @return null
 */
Value aspic_print(Value* argv, int argc);

/**
 * Get string representation of given value
 * @return string
 */
Value aspic_str(Value* argv, int argc);

/**
 * Get type of given value
 * @return string
 */
Value aspic_type(Value* argv, int argc);

#endif
