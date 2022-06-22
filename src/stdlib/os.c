#include "os.h"
#include "object.h"
#include "value_array.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <unistd.h>

Value aspic_os_cd(Value* argv, int argc)
{
    if (argc != 1) {
        return make_error("cd() expects 1 argument, got %d", argc);
    }
    ObjectString* string = to_string(argv[0]);
    if (!string) {
        return make_error("cd() expects a string, got '%s'", value_type(argv[0]));
    }
    if (chdir(string->chars) != 0) {
        return make_error("cd(): cannot go to directory");
    }
    return make_null();
}

Value aspic_os_ls(Value* argv, int argc)
{
    if (argc > 1) {
        return make_error("ls() takes at most 1 argument, got %d", argc);
    }
    const char* dirname = ".";
    if (argc == 1) {
        dirname = ((ObjectString*)argv[0].as.object)->chars;
    }
    DIR* rep = opendir(dirname);
    if (rep == NULL) {
        return make_error("Cannot read %s", dirname);
    }
    Value result = make_array(NULL, 0);
    ValueArray* array = &((ObjectArray*)result.as.object)->array;
    struct dirent* f = NULL;
    while ((f = readdir(rep)) != NULL) {
        // Ignore "." and ".."
        if (strcmp(f->d_name, ".") != 0 && strcmp(f->d_name, "..") != 0) {
            value_array_push(array, make_string_from_cstr(f->d_name));
        }
    }
    if (closedir(rep) == -1) {
        exit(-1);
    }
    return result;
}

Value aspic_os_getenv(Value* argv, int argc)
{
    if (argc != 1) {
        return make_error("getenv() expects 1 argument, got %d", argc);
    }
    ObjectString* string = to_string(argv[0]);
    if (!string) {
        return make_error("getenv() expects a string, got '%s'", value_type(argv[0]));
    }
    const char* result = getenv(string->chars);
    return result == NULL ? make_null() : make_string_from_cstr(result);
}
