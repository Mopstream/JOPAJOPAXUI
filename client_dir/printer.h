#ifndef LLP_LAB2_PRINTER_H
#define LLP_LAB2_PRINTER_H

#include "ast.h"
#include "inttypes.h"
#include "../spec.pb-c.h"
void print_node(ast_node *node, int32_t nesting_level);
void print_response(Response * res, int32_t nesting_level);
#endif