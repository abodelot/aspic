#ifndef ASPIC_STDLIB_OS_H
#define ASPIC_STDLIB_OS_H

#include "value.h"

Value aspic_os_cd(Value* argv, int argc);
Value aspic_os_ls(Value* argv, int argc);
Value aspic_os_getenv(Value* argv, int argc);

#endif
