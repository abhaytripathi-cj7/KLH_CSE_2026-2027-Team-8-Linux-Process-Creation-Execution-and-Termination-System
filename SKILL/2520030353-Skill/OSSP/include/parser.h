#ifndef OSSP_PARSER_H
#define OSSP_PARSER_H
#include "shell.h"
#include <stddef.h>
int parse_line(const char *line, Pipeline *pipeline, char *error, size_t error_size);
void free_pipeline(Pipeline *pipeline);
#endif
