#ifndef LLP_LAB2_AST_H
#define LLP_LAB2_AST_H

#include "types.h"

typedef enum ast_node_type {
    FILENAME_N = 0,
    INSERT_N = 1,
    SELECT_N = 2,
    UPDATE_N = 3,
    DELETE_N = 4,
    INDEX_N = 5,
    NODE_N = 6,
    LINK_N = 7,
    CONDITIONAL_N = 8,
    FILTER_N = 9,
    SET_N = 10,
    VAL_N = 11,
    NAME_N = 12,
    LIST_N = 13,
    ATTR_DESC_N = 14,
} ast_node_type;

typedef enum {
    NONE = 0,
    NAME = 1,
    INSERT_TARGET = 2,
    SET = 3,
    ATTR = 4,
    LOGICAL_OP = 5,
    CMP = 6,
    LINK = 7,
    CNT = 8,
    ATTR_DESC = 9,
} value_type;

typedef enum {
    AND = 0,
    OR = 1,
    NOT = 2,
} logical_op;

typedef enum {
    I_NODE = 0,
    I_LINK = 1,
    I_INDEX = 2,
} insert_target_t;

typedef struct {
    uint32_t node_id;
    char *attr_name;
    attr_t new_value;
} set_t;

typedef struct {
    uint32_t node_from_type_id;
    uint32_t node_from_id;
    uint32_t node_to_type_id;
    uint32_t node_to_id;
} link_t;

typedef struct ast_node {
    ast_node_type type;
    struct ast_node *left;
    struct ast_node *right;
    value_type v_type;
    void *value;
} ast_node;

typedef struct {
    insert_target_t target;
    ast_node *body;
} insert_body_t;


ast_node *create_node();

void free_node(ast_node *node);

ast_node *new_filename_node(char *filename, ast_node *query);

ast_node *new_insert_node(ast_node *index, insert_body_t obj);

ast_node *new_select_node(ast_node *index, ast_node *body);

ast_node *new_delete_node(ast_node *index, ast_node *body);

ast_node *new_update_node(ast_node *index, set_t *update);

ast_node *new_index_node_from_name(char index_name[16]);

ast_node *new_index_node_from_attrs(char index_name[16], ast_node *attrs_list);

ast_node *add_to_list(ast_node *list, ast_node *val);

set_t *new_set_expr(uint32_t node_id, char name[16], attr_t *new_val);

ast_node *new_filter_node(ast_node *l, ast_node *r, logical_op op);

ast_node *new_conditional_node(ast_node *name, cmp_t cmp, ast_node *val);

ast_node *new_node_node(ast_node *val_list);

ast_node *new_link_node(link_t link);

ast_node *new_val_node(attr_t *attr);

attr_t *new_int_attr(int32_t i_val);

attr_t *new_double_attr(double d_val);

attr_t *new_bool_attr(bool b_val);

attr_t *new_str_attr(char *str_val, uint32_t size);

ast_node *new_name_node(char name[16]);

ast_node *new_attr_node(char * name, val_type_t type);

ast_node *list_init(ast_node *val);

#endif
