#include "ast.h"
#include "types.h"
#include "../spec.pb-c.h"
#include <stdlib.h>

enum _AstNodeType node_types[] = {
        [FILENAME_N] = AST_NODE_TYPE__FILENAME_N,
        [INSERT_N] = AST_NODE_TYPE__INSERT_N,
        [SELECT_N] = AST_NODE_TYPE__SELECT_N,
        [UPDATE_N] = AST_NODE_TYPE__UPDATE_N,
        [DELETE_N] = AST_NODE_TYPE__DELETE_N,
        [INDEX_N] = AST_NODE_TYPE__INDEX_N,
        [NODE_N] = AST_NODE_TYPE__NODE_N,
        [LINK_N] = AST_NODE_TYPE__LINK_N,
        [CONDITIONAL_N] = AST_NODE_TYPE__CONDITIONAL_N,
        [FILTER_N] = AST_NODE_TYPE__FILTER_N,
        [SET_N] = AST_NODE_TYPE__SET_N,
        [VAL_N] = AST_NODE_TYPE__VAL_N,
        [NAME_N] = AST_NODE_TYPE__NAME_N,
        [LIST_N] = AST_NODE_TYPE__LIST_N,
};

enum _InsertTarget insert_targets[] = {
        [I_NODE] = INSERT_TARGET__I_NODE,
        [I_LINK] = INSERT_TARGET__I_LINK,
        [I_INDEX] = INSERT_TARGET__I_INDEX,
};


//NONE = 0,
//SET = 3,
//ATTR = 4,
//LOGICAL_OP = 5,
//CMP = 6,
//LINK = 7,

Ast *convert(ast_node *node) {
    Ast *ast = malloc(sizeof(Ast));
    ast__init(ast);
    if (node->left) ast->left = convert(node->left);
    if (node->right) ast->right = convert(node->right);
    ast->type = node_types[node->type];
    switch (node->v_type) {
        case NAME: {
            ast->v_type = VALUE_TYPE__NAME;
            ast->val->name = node->value;
            break;
        }
        case INSERT_TARGET: {
            ast->v_type = VALUE_TYPE__INSERT_TARGET;
            ast->val->target = insert_targets[*(insert_target_t *) node->value];
            break;
        }
        case SET: {
            ast->v_type = VALUE_TYPE__SET;
            ast->val->set = malloc(sizeof(Set));
            set__init(ast->val->set);
            ast->val->set->node_id = ((set_t *) node->value)->node_id;
            ast->val->set->attr_name = ((set_t *) node->value)->attr_name;
            ast->val->set->new_value = malloc(sizeof(Attr));
            attr__init(ast->val->set->new_value);
            ast->val->set->new_value->type;

        }

            // in process...
    }
}