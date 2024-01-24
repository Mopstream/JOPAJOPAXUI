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
    return cond;
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

select_q *parse_sel(schema_t *schema, Ast *sel_body) {
    if (!sel_body) return 0;
    select_q *sel = malloc(sizeof(select_q));
    Ast *ind_ast = sel_body->left;
    sel->index = get_first_index(schema, ind_ast->val->name);
    sel->inner = 0;

    Ast *filter = sel_body->right;
    if (!filter) {
        sel->cond_cnt = 0;
        sel->conditionals = 0;
        sel->inner = 0;
        return sel;
    }

    Ast *conds = filter->left;
    Ast *new_sel_body = filter->right;
    if (!conds) {
        sel->cond_cnt = 0;
        sel->conditionals = 0;
        sel->inner = parse_sel(schema, new_sel_body);
        return sel;
    }
    uint32_t cond_cnt = 0;
    cond_t *conditionals;
    if (conds->type == AST_NODE_TYPE__LOGICAL_N) {
        parse_log(conds, &cond_cnt, &conditionals);
    } else {
        cond_cnt = 1;
        conditionals = malloc(sizeof(cond_t));
        *conditionals = parse_cond(conds);
    }
    sel->conditionals = conditionals;
    sel->cond_cnt = cond_cnt;

    if (new_sel_body) {
        sel->inner = parse_sel(schema, new_sel_body);
    }
    return sel;
}

query_t *norm_construct(Ast *ast) {
    query_t *q = malloc(sizeof(query_t));
    memcpy(q->filename, ast->val->name, strlen(ast->val->name) + 1);
    Ast *query_ast = ast->left;
    Ast *index_ast = query_ast->left;
    switch (query_ast->type) {
        case AST_NODE_TYPE__SELECT_N: {
            q->q_type = SELECT;

            if (!index_ast) {
                q->target = Q_INDEX;
                q->index = 0;
                return q;
            }

            schema_t *schema = init_db(q->filename);
            q->index = get_first_index(schema, index_ast->val->name);

            if (q->index->kind == I_LINK) {
                q->target = Q_LINK;
                return q;
            }
            q->target = Q_NODE;
            select_q *sel = parse_sel(schema, query_ast);
            close(schema->fd);
            free(schema);

            q->query_body.sel = sel;
            return q;
        }
        case AST_NODE_TYPE__INSERT_N: {
            q->q_type = ADD;
            Ast *body = query_ast->right;
            if (!body) {
                char *index_name = index_ast->val->name;
                Ast *attrs_list = index_ast->left;
                if (attrs_list) {
                    uint32_t cnt = attrs_list->val->cnt;
                    attr_type_t *attrs = malloc(cnt * sizeof(attr_type_t));
                    for (uint32_t i = 0; i < cnt; ++i) {
                        Ast *attr = attrs_list->left;
                        AttrType *curr_attr = attr->val->attr_type;
                        memcpy(attrs[i].type_name, curr_attr->name, strlen(curr_attr->name) + 1);
                        attrs[i].type = attr_types[curr_attr->val];
                        attrs_list = attrs_list->right;
                    }
                    q->index = create_index(index_name, attrs, cnt, I_NODE);
                } else {
                    q->index = create_index(index_name, 0, 0, I_LINK);
                }
                q->q_type = ADD;
                q->target = Q_INDEX;
                return q;
            }

            schema_t *schema = init_db(q->filename);
            q->index = get_first_index(schema, index_ast->val->name);
            close(schema->fd);
            free(schema);
            switch (query_ast->val->target) {
                case INSERT_TARGET__I_NODE: {
                    q->target = Q_NODE;
                    node_t *node = malloc(sizeof(node_t));
                    q->query_body.add.node = node;
                    Ast *val_list = body->left;
                    uint32_t cnt = val_list->val->cnt;
                    uint32_t id;
                    bool flag = false;
                    if (body->v_type == VALUE_TYPE__CNT) {
                        id = body->val->cnt;
                        flag = true;
                    }
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
                    q->query_body.add.node = create_node(values, cnt, id, flag);
                    break;
                }
                case INSERT_TARGET__I_LINK: {
                    q->target = Q_LINK;
                    Link *curr_link = body->val->link;
                    q->query_body.add.link = create_link(curr_link->node_from_id, curr_link->node_from_type_id,
                                                         curr_link->node_to_id, curr_link->node_to_type_id);
                    break;
                }
                default: {
                    fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
                }

            }
            return q;
        }
        case AST_NODE_TYPE__UPDATE_N: {
            q->q_type = SET;
            q->target = Q_NODE;

            schema_t *schema = init_db(q->filename);
            q->index = get_first_index(schema, index_ast->val->name);
            close(schema->fd);
            free(schema);

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
        case AST_NODE_TYPE__DELETE_N: {
            Ast *body = query_ast->right;
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
        default: {
            fprintf(stderr, "PROGRAMMAN TUPOY!!!!\n");
        }
    }
}
