#ifndef TOML_UTILS_H
#define TOML_UTILS_H

#include <stdio.h>

void toml_write_string(FILE *fp, const char *value);
void toml_write_kv_string(FILE *fp, const char *key, const char *value);
void toml_write_kv_string_opt(FILE *fp, const char *key, const char *value);

#endif
