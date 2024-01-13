#include "query_constructor.h"
#include <stdlib.h>
#include "stdio.h"
#include "includes/schema.h"
#include <string.h>
#include <unistd.h>

attr_val_type_t attr_types[] = {
        [VAL_TYPE__INT] = INT,
        [VAL_TYPE__DOUBLE] = DOUBLE,
        [VAL_TYPE__STRING] = STRING,
        [VAL_TYPE__BOOL] = BOOL,
};

cmp_t cmps[] = {
        [CMP__GT] = GREATER,
        [CMP__LT] = LOWER,
        [CMP__EQ] = EQUAL,
        [CMP__GT_EQ] = GREATER_EQUAL,
        [CMP__LT_EQ] = LOWER_EQUAL,
        [CMP__SUBSTR] = SUBSTR,
};

cond_t parse_cond(Ast *c) {
    cond_t cond;
    cond.cmp = cmps[c->val->cmp];
    cond.attr_name = c->left->val->name;
    switch (c->right->val->attr->type) {
        case VAL_TYPE__INT: {
            cond.val.i = c->right->val->attr->i;
            break;
        }
        case VAL_TYPE__DOUBLE: {
            cond.val.d = c->right->val->attr->d;
            break;
        }
        case VAL_TYPE__STRING: {
            String *curr_str = c->right->val->attr->str;
            cond.val.str = (string_t) {.str = curr_str->str, .size = curr_str->size};
            break;
        }
        case VAL_TYPE__BOOL: {
            cond.val.b = c->right->val->attr->b;
            break;
        }
        default: {
            fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
        }
    }
}

void parse_log(Ast *op, uint32_t *cnt, cond_t **conditionals) {
    uint32_t curr_cnt = *cnt;
    *cnt += 1;
    if (op->left->type != AST_NODE_TYPE__CONDITIONAL_N) {
        parse_log(op->left, cnt, conditionals);
    } else {
        *cnt += 1;
        *conditionals = malloc((*cnt) * sizeof(cond_t));
        (*conditionals)[curr_cnt + 1] = parse_cond(op->left);
    }
    *conditionals[curr_cnt] = parse_cond(op->right);
}

query_t *construct(Ast *ast) {
    query_t *q = malloc(sizeof(query_t));
    q->filename = ast->val->name;
    Ast *query_ast = ast->left;
    Ast *index_ast = query_ast->left;
    if (!index_ast) {
        q->target = Q_INDEX;
        q->q_type = SELECT;
        q->index = 0;
        return q;
    }
    if (!query_ast->right) {
        char *index_name = index_ast->val->name;
        Ast *attrs_list = index_ast->left;
        uint32_t cnt = attrs_list->val->cnt;
        attr_type_t *attrs = malloc(cnt * sizeof(attr_type_t));
        for (uint32_t i = 0; i < cnt; ++i) {
            Ast *attr = attrs_list->left;
            AttrType *curr_attr = attr->val->attr_type;
            memcpy(attrs[i].type_name, curr_attr->name, 16);
            attrs[i].type = attr_types[curr_attr->val];
            attrs_list = attrs_list->right;
        }

        q->index = create_index(index_name, attrs, cnt);
        q->q_type = ADD;
        q->target = Q_INDEX;
        return q;
    }
    schema_t *schema = init_db(q->filename);
    q->index = get_first_index(schema, index_ast->val->name);
    close(schema->fd);
    free(schema);
    if (query_ast->type == AST_NODE_TYPE__UPDATE_N) {
        q->q_type = SET;
        q->target = Q_NODE;
        Set *curr_set = query_ast->val->set;
        Attr *curr_attr = curr_set->new_value;
        q->query_body.set = (set_q) {.node_id = curr_set->node_id};
        memcpy(q->query_body.set.attr_name, curr_set->attr_name, 16);
        switch (curr_attr->type) {
            case VAL_TYPE__INT: {
                q->query_body.set.new_value.i = curr_attr->i;
                break;
            }
            case VAL_TYPE__DOUBLE: {
                q->query_body.set.new_value.d = curr_attr->d;
                break;
            }
            case VAL_TYPE__STRING: {
                q->query_body.set.new_value.str = (string_t) {.size = curr_attr->str->size, .str = curr_attr->str->str};
                break;
            }
            case VAL_TYPE__BOOL: {
                q->query_body.set.new_value.b = curr_attr->b;
                break;
            }
            default: {
                fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
            }
        }
        return q;
    }
    Ast *body = query_ast->right;
    if (query_ast->type == AST_NODE_TYPE__DELETE_N) {
        q->q_type = DELETE;
        if (!body) {
            q->target = Q_INDEX;
        } else {
            q->target = Q_NODE;
            Attr *id = body->val->attr;
            q->query_body.del.id = id->i;
        }
        return q;
    }
    if (query_ast->type == AST_NODE_TYPE__INSERT_N) {
        q->q_type = ADD;
        switch (body->val->target) {
            case INSERT_TARGET__I_NODE: {
                q->target = Q_NODE;
                node_t *node = malloc(sizeof(node_t));
                q->query_body.add.node = node;
                Ast *val_list = body->left;
                uint32_t cnt = val_list->val->cnt;
                value_t *values = malloc(cnt * sizeof(value_t));
                for (uint32_t i = 0; i < cnt; ++i) {
                    Attr *value = val_list->left->val->attr;
                    switch (value->type) {
                        case VAL_TYPE__INT: {
                            values[i].i = value->i;
                            break;
                        }
                        case VAL_TYPE__DOUBLE: {
                            values[i].d = value->d;
                            break;
                        }
                        case VAL_TYPE__STRING: {
                            values[i].str = (string_t) {.size = value->str->size, .str = value->str->str};
                            break;
                        }
                        case VAL_TYPE__BOOL: {
                            values[i].b = value->b;
                            break;
                        }
                        default: {
                            fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
                        }
                    }
                    val_list = val_list->right;
                }
                q->query_body.add.node = create_node(values, cnt);
                break;
            }
            case INSERT_TARGET__I_LINK: {
                q->target = Q_LINK;
                link_t *link = malloc(sizeof(link_t));
                q->query_body.add.link = link;
                Link *curr_link = body->val->link;
                link->node_from_id = curr_link->node_from_id;
                link->node_from_type_id = curr_link->node_from_type_id;
                link->node_to_id = curr_link->node_to_id;
                link->node_to_type_id = curr_link->node_to_type_id;
                break;
            }
            default: {
                fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
            }

        }
        return q;
    }
    q->q_type = SELECT;
    q->target = Q_NODE;
    Ast *operation = body;
    uint32_t cond_cnt = 0;
    cond_t *conditionals;
    parse_log(operation, &cond_cnt, &conditionals);
    select_q * sel = malloc(sizeof(select_q));
    sel->conditionals = conditionals;
    sel->cond_cnt = cond_cnt;
    q->query_body.sel = sel;
    return q;
}
