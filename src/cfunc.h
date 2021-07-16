#ifndef ASPIC_CFUNC_H
#define ASPIC_CFUNC_H

#include "value.h"

/**
 * Return error in argument is not truthy
 * @return true, or error
 */
Value aspic_assert(Value* argv, int argc);

/**
 * Returns an approximation of processor time used by the program.
 * @return number of seconds used
 */
Value aspic_clock(Value* argv, int argc);

/**
 * Get user input from stdin
 * @param 1: prompt
 * @return string
 */
Value aspic_input(Value* argv, int argc);

/**
 * Convert to integer
 * @param 1: string representation
 * @param 2: base (default: 10)
 */
Value aspic_int(Value* argv, int argc);

/**
 * Return length of given array or string
 * @param 1: string|array
 * @return int
 */
Value aspic_len(Value* argv, int argc);

/**
 * Remove value from array
 * @param 1: array
 * @return removed value
 */
Value aspic_pop(Value* argv, int argc);

/**
 * Print given arguments, each separated by a space.
 * @param 1-N: list of arguments to print
 * @return null
 */
Value aspic_print(Value* argv, int argc);

/**
 * Append value to array
 * @param 1: array
 * @param 2: value to push
 * @return array
 */
Value aspic_push(Value* argv, int argc);

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
