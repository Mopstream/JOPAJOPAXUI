#include <stdlib.h>
#include "ast.h"
#include "string.h"
#include "stdio.h"

ast_node *create_node() {
    ast_node *node = malloc(sizeof(ast_node));
    *node = (ast_node) {0};
    return node;
}

void free_node(ast_node *node) {
    if (node) {
        free_node(node->left);
        free_node(node->right);
        if (node->v_type) free(node->value);
        free(node);
    }
}

ast_node *new_filename_node(char *filename, ast_node *query) {
    ast_node *this = create_node();
    this->type = FILENAME_N;
    this->left = query;
    this->v_type = NAME;
    this->value = filename;
    return this;
}

ast_node *new_insert_node(ast_node *index, insert_body_t obj) {
    ast_node *this = create_node();
    this->type = INSERT_N;
    this->left = index;
    this->right = obj.body;
    insert_target_t *t = malloc(sizeof(insert_target_t));
    *t = obj.target;
    this->v_type = INSERT_TARGET;
    this->value = t;
    return this;
}

ast_node *new_select_node(ast_node *index, ast_node *body) {
    ast_node *this = create_node();
    this->type = SELECT_N;
    this->left = index;
    this->right = body;
    return this;
}

ast_node *new_delete_node(ast_node *index, ast_node *body) {
    ast_node *this = create_node();
    this->type = DELETE_N;
    this->left = index;
    this->right = body;
    return this;
}

ast_node *new_update_node(ast_node *index, set_t *update) {
    ast_node *this = create_node();
    this->type = UPDATE_N;
    this->left = index;
    this->v_type = SET;
    this->value = update;
    return this;
}

ast_node *new_index_node_from_name(char index_name[16]) {
    ast_node *this = create_node();
    this->type = INDEX_N;
    this->v_type = NAME;
    this->value = index_name;
    return this;
}

ast_node *new_index_node_from_attrs(char index_name[16], ast_node *attrs_list) {
    ast_node *this = create_node();
    this->type = INDEX_N;
    this->v_type = NAME;
    this->value = index_name;
    this->left = attrs_list;
    return this;
}

ast_node *add_to_list(ast_node *list, ast_node *val) {
    ast_node *this = create_node();
    this->type = LIST_N;
    this->left = val;
    this->right = list;
    this->v_type = CNT;
    uint32_t * i = malloc(sizeof(uint32_t));
    *i = *(uint32_t *)list->value + 1;
    this->value = i;
    return this;
}

ast_node *list_init(ast_node *val) {
    ast_node *this = create_node();
    this->type = LIST_N;
    this->left = val;
    this->v_type = CNT;
    uint32_t * i = malloc(sizeof(uint32_t));
    *i = 1;
    this->value = i;
    return this;
}

set_t *new_set_expr(uint32_t node_id, char name[16], attr_t *new_val) {
    set_t *this = malloc(sizeof(set_t));
    this->node_id = node_id;
    this->attr_name = name;
    this->new_value = *new_val;
    return this;
}

ast_node *new_filter_node(ast_node *logic, ast_node *inner) {
    ast_node *this = create_node();
    this->type = FILTER_N;
    this->left = logic;
    this->right = inner;
    return this;
}

ast_node *new_logical_node(ast_node *l, ast_node *r, logical_op op) {
    ast_node *this = create_node();
    this->type = LOGICAL_N;
    this->left = l;
    this->right = r;
    logical_op *l_op = malloc(sizeof(logical_op));
    *l_op = op;
    this->v_type = LOGICAL_OP;
    this->value = l_op;
    return this;
}

ast_node *new_conditional_node(ast_node *name, cmp_t cmp, ast_node *val) {
    ast_node *this = create_node();
    this->type = CONDITIONAL_N;
    this->left = name;
    this->right = val;
    cmp_t *c = malloc(sizeof(cmp_t));
    *c = cmp;
    this->v_type = CMP;
    this->value = c;
    return this;
}

ast_node *new_node_node(ast_node *val_list, int id, bool flag) {
    ast_node *this = create_node();
    this->type = NODE_N;
    this->left = val_list;
    if (flag) {
        this->v_type = CNT;
        uint32_t *i = malloc(sizeof(uint32_t));
        *i = id;
        this->value = i;
    }
    return this;
}

ast_node *new_link_node(link_t link) {
    ast_node *this = create_node();
    this->type = LINK_N;
    link_t *l = malloc(sizeof(link_t));
    *l = link;
    this->v_type = LINK;
    this->value = l;
    return this;
}

ast_node *new_val_node(attr_t *attr) {
    ast_node *this = create_node();
    this->type = VAL_N;
    attr_t *v = malloc(sizeof(attr_t));
    *v = *attr;
    this->v_type = ATTR;
    this->value = v;
    return this;
}

attr_t *new_int_attr(int32_t i_val) {
    attr_t *this = malloc(sizeof(attr_t));
    this->type = INT;
    this->val.i = i_val;
    return this;
}

attr_t *new_double_attr(double d_val) {
    attr_t *this = malloc(sizeof(attr_t));
    this->type = DOUBLE;
    this->val.d = d_val;
    return this;
}

attr_t *new_bool_attr(bool b_val) {
    attr_t *this = malloc(sizeof(attr_t));
    this->type = BOOL;
    this->val.b = b_val;
    return this;
}

attr_t *new_str_attr(char *str_val, uint32_t size) {
    attr_t *this = malloc(sizeof(attr_t));
    this->type = STRING;
    this->val.str = (string_t) {.size = size, .str = str_val};
    return this;
}

ast_node *new_name_node(char name[16]) {
    ast_node *this = create_node();
    this->type = NAME_N;
    this->v_type = NAME;
    this->value = name;
    return this;
}

ast_node *new_attr_node(char * name, val_type_t type){
    ast_node * this = create_node();
    this->type = ATTR_DESC_N;
    this->v_type = ATTR_DESC;
    attr_desc_t * a = malloc(sizeof(attr_desc_t));
    a->type = type;
    memcpy(a->name, name, strlen(name));
    this->value = a;
    return this;
}