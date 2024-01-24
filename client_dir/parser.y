%{
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "ast.h"

#define MAX_NAME_LENGTH 16

extern int yylex();
extern int yylineno;

void yyerror(const char *msg) {
    fprintf(stderr, "Error on line %d: %s\n", yylineno, msg);
}
ast_node *ast = 0;
%}

%union {
    bool bool_val;
    int32_t int_val;
    double double_val;
    char* str_val;
    ast_node* node;
    attr_t* attr;
    insert_body_t in_body;
    set_t* set_e;
    cmp_t cmp;
    logical_op l_op;
    val_type_t type;
}

%token L_PR
%token R_PR
%token L_BR
%token R_BR
%token L_BRK
%token R_BRK
%token COMMA
%token COLON
%token SELECT_T
%token INSERT_T
%token UPDATE_T
%token DELETE_T
%token INDEX_T
%token INDEX_NAME
%token NAMES
%token NODE_T
%token LINK_T
%token ID
%token NODE_FROM
%token NODE_TO
%token VALUES
%token FILTER_T
%token SET_T
%token<str_val> FILENAME_T
%token<type> TYPE_T
%token<cmp> COMPARE_OP
%token<l_op> LOGICAL_BOP
%token<l_op> LOGICAL_UOP
%token<bool_val> BOOL_T
%token<int_val> INT_T
%token<double_val> DOUBLE_T
%token<str_val> STRING_T
%token<str_val> NAME_T

%type<node> file
%type<node> query
%type<node> select
%type<node> insert
%type<node> update
%type<node> delete
%type<node> select_body
%type<node> insert_body
%type<node> update_body
%type<node> delete_body
%type<node> index_from_name
%type<node> index_from_attrs
%type<node> attr_list
%type<node> attr
%type<node> name_node
%type<str_val> name
%type<in_body> insert_obj
%type<node> values
%type<node> element_list
%type<node> element
%type<attr> value
%type<node> link
%type<node> filter
%type<node> operation
%type<node> compare_op
%type<node> logical_op
%type<set_e> set

%start file
%%
file: FILENAME_T L_BR query R_BR {ast = new_filename_node($1, $3); return 0;}

query: select { $$ = $1; }
     | insert { $$ = $1; }
     | update { $$ = $1; }
     | delete { $$ = $1; }

select: SELECT_T L_BR select_body R_BR { $$ = $3; }

insert: INSERT_T L_BR insert_body R_BR { $$ = $3; }

update: UPDATE_T L_BR update_body R_BR { $$ = $3; }

delete: DELETE_T L_BR delete_body R_BR { $$ = $3; }

insert_body: index_from_name L_BR insert_obj R_BR { $$ = new_insert_node($1, $3); }
           | index_from_attrs { $$ = new_insert_node($1, (insert_body_t){.target = I_INDEX, .body = 0}); }

select_body: index_from_name L_BR filter R_BR { $$ = new_select_node($1, new_filter_node($3, 0)); }
           | index_from_name L_BR filter L_BR select_body R_BR R_BR { $$ = new_select_node($1, new_filter_node($3, $5)); }
           | index_from_name { $$ = new_select_node($1, 0); }
           | index_from_name L_BR select_body R_BR { $$ = new_select_node($1, new_filter_node(0, $3)); }
           | INDEX_T { $$ = new_select_node(0, 0); }

update_body: index_from_name L_BR set R_BR {$$ = new_update_node($1, $3); }

delete_body: index_from_name L_BR NODE_T L_PR INT_T R_PR R_BR { $$ = new_delete_node($1, new_val_node(new_int_attr($5))); }
           | index_from_name L_BR LINK_T L_PR INT_T R_PR R_BR { $$ = new_delete_node($1, new_val_node(new_int_attr($5))); }
           | index_from_name { $$ = new_delete_node($1, 0); }

index_from_name : INDEX_T L_PR INDEX_NAME COLON name R_PR { $$ = new_index_node_from_name($5); }

index_from_attrs : INDEX_T L_PR NODE_T L_PR INDEX_NAME COLON name COMMA NAMES L_BRK attr_list R_BRK R_PR R_PR { $$ = new_index_node_from_attrs($7, $11); }
                 | INDEX_T L_PR LINK_T L_PR INDEX_NAME COLON name R_PR R_PR { $$ = new_index_node_from_attrs($7, 0); }

attr_list : attr COMMA attr_list { $$ = add_to_list($3, $1); }
          | attr { $$ = list_init($1); }

attr: name COLON TYPE_T { $$ = new_attr_node($1, $3); }

name_node : name { $$ = new_name_node($1); }

name: NAME_T { if(strlen($1) > MAX_NAME_LENGTH) { yyerror("name is too long"); YYABORT; } $$ = $1; }

insert_obj: NODE_T L_PR values R_PR {$$ = (insert_body_t){.target = I_NODE, .body = new_node_node($3, 0, false)}; }
          | NODE_T L_PR ID COLON INT_T COMMA values R_PR {$$ = (insert_body_t){.target = I_NODE, .body = new_node_node($7, $5, true)};}
          | link {$$ = (insert_body_t){.target = I_LINK, .body = $1}; }

values: VALUES COLON L_BRK element_list R_BRK { $$ = $4; }

element_list: element COMMA element_list { $$ = add_to_list($3, $1); }
            | element { $$ = list_init($1); }

element: value { $$ = new_val_node($1); }

value: BOOL_T { $$ = new_bool_attr($1); }
     | INT_T { $$ = new_int_attr($1); }
     | DOUBLE_T { $$ = new_double_attr($1); }
     | STRING_T { $$ = new_str_attr($1, strlen($1)); }

link: LINK_T L_PR L_PR INT_T COMMA INT_T R_PR COMMA L_PR INT_T COMMA INT_T R_PR R_PR { $$ = new_link_node((link_t){.node_from_type_id = $4,
                                                                                                                   .node_from_id = $6,
                                                                                                                   .node_to_type_id = $10,
                                                                                                                   .node_to_id = $12}); }

filter: FILTER_T COLON operation { $$ = $3; }

operation: logical_op { $$ = $1; }
         | compare_op { $$ = $1; }

compare_op: COMPARE_OP L_PR name_node COMMA element R_PR { $$ = new_conditional_node($3, $1, $5); }

logical_op: LOGICAL_UOP L_PR operation R_PR { $$ = new_logical_node($3, 0, $1); }
          | LOGICAL_BOP L_PR operation COMMA operation R_PR { $$ = new_logical_node($3, $5, $1); }

set: SET_T L_PR INT_T COMMA name COMMA value R_PR { $$ = new_set_expr($3, $5, $7); }

%%