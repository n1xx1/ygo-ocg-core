#pragma once

#define IMPL_READ_VALUE(location, failReturn, eof, name, type)                 \
    type name = reader.read<type>(&(eof));                                     \
    do {                                                                       \
        if (eof) {                                                             \
            printf("debug: eof reading \"%s\" in %s\n", location, #name);      \
            return failReturn;                                                 \
        }                                                                      \
    } while (0)

#define IMPL_READ_VALUE_SET(location, failReturn, eof, name, type, jstype,     \
                            jstarget)                                          \
    type name = reader.read<type>(&(eof));                                     \
    do {                                                                       \
        if (eof) {                                                             \
            printf("debug: eof reading \"%s\" in %s\n", location, #name);      \
            return failReturn;                                                 \
        }                                                                      \
        jstarget.Set(#name, jstype::New(env, name));                           \
    } while (0)

#define READ_VALUE_VSET(location, failReturn, eof, name, type, jsvalue,        \
                        jstarget)                                              \
    type name = reader.read<type>(&(eof));                                     \
    do {                                                                       \
        if (*eof) {                                                            \
            printf("debug: eof reading \"%s\" in %s\n", location, #name);      \
            return failReturn;                                                 \
        }                                                                      \
        jstarget.Set(#name, jsvalue);                                          \
    } while (0)
