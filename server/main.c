#include "includes/schema.h"
#include "includes/query.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

void add_del_test() {
    char *db_name = "../add_del.db";
    char *csv_name = "../add_del.csv";
    node_type_t empty_node_type = {.type_id = 1, .attr_count = 0, .attr_types = 0};
    element_type_t empty_node_el_type = {
            .type_name = "empty_node_type",
            .kind = I_NODE,
            .description.node = empty_node_type
    };

    empty_node_el_type.size = 4 + sizeof(empty_node_el_type.type_name) + sizeof(empty_node_el_type.kind) + 4 + 4;
    index_t empty_node_index = (index_t) {
            .type = empty_node_el_type,
            .first_page_num = 0,
            .count = 0
    };
    query_t add_index = {
            .filename = db_name,
            .index = &empty_node_index,
            .q_type = ADD,
            .target=Q_INDEX,
            .query_body = 0
    };
    exec(&add_index);

    node_t node = {
            .id = 1,
            .attrs = 0,
            .out_cnt = 0,
            .links_out = 0,
            .in_cnt = 0,
            .links_in = 0
    };

    query_t add_node = {
            .filename = db_name,
            .index = &empty_node_index,
            .q_type = ADD,
            .target=Q_NODE,
            .query_body.add.node = &node
    };
    query_t delete_node = {
            .filename = db_name,
            .index = &empty_node_index,
            .q_type = DELETE,
            .target=Q_NODE,

            .query_body.del.id = 1
    };

    FILE *add_del_file = fopen(csv_name, "w+");

    fprintf(add_del_file, "op_cnt,time\n");

    uint32_t del_size = 400;
    uint32_t add_size = 500;
    uint64_t cnt = 0;
    uint32_t *old = 0;
    uint32_t add[add_size];
    uint32_t delete[del_size];
    uint64_t clock_start;
    uint64_t duration;
    uint64_t x = 0;
    while (true) {
        to_add(add, add_size, 0);
        for (uint32_t i = 0; i < add_size / 100; ++i) {
            clock_start = clock();
            for (uint32_t j = 0; j < 100; ++j) {
                node.id = add[i * 100 + j];
                exec(&add_node);
            }
            duration = clock() - clock_start;
            fprintf(add_del_file, "%lu, %lu\n", x, duration);
            fflush(add_del_file);
            x += 100;
        }

        to_delete(delete, del_size, &old, cnt, add, add_size);
        for (uint32_t i = 0; i < del_size / 100; ++i) {
            clock_start = clock();
            for (uint32_t j = 0; j < 100; ++j) {
                delete_node.query_body.del.id = delete[i * 100 + j];
                exec(&delete_node);
            }
            duration = clock() - clock_start;
            fprintf(add_del_file, "%lu, %lu\n", x, duration);
            fflush(add_del_file);
            x += 100;
        }
        cnt += 100;
    }
}

void add_test() {
    char *db_name = "../add.db";
    char *csv_name = "../add.csv";
    char *storage_name = "../storage";
    char *storage_size_name = "../storage_size";

    node_type_t empty_node_type = {.type_id = 1, .attr_count = 0, .attr_types = 0};
    element_type_t empty_node_el_type = {
            .type_name = "empty_node_type",
            .kind = I_NODE,
            .description.node = empty_node_type
    };

    empty_node_el_type.size = 4 + sizeof(empty_node_el_type.type_name) + sizeof(empty_node_el_type.kind) + 4 + 4;
    index_t empty_node_index = (index_t) {
            .type = empty_node_el_type,
            .first_page_num = 0,
            .count = 0
    };
    query_t add_index = {
            .filename = db_name,
            .index = &empty_node_index,
            .q_type = ADD,
            .target=Q_INDEX,
            .query_body = 0
    };
    exec(&add_index);

    node_t node = {
            .id = 1,
            .attrs = 0,
            .out_cnt = 0,
            .links_out = 0,
            .in_cnt = 0,
            .links_in = 0
    };


    query_t add_node = {
            .filename = db_name,
            .index = &empty_node_index,
            .q_type = ADD,
            .target=Q_NODE,
            .query_body.add.node = &node
    };

    FILE *storage_file = fopen(storage_name, "w+");
    FILE *add_file = fopen(csv_name, "w+");

    fprintf(add_file, "op_cnt,time\n");


    uint32_t add_size = 500;
    uint32_t add[add_size];
    uint64_t clock_start;
    uint64_t duration;
    uint64_t x = 0;
    while (true) {
        to_add(add, add_size, storage_file);
        for (uint32_t i = 0; i < add_size / 100; ++i) {
            clock_start = clock();
            for (uint32_t j = 0; j < 100; ++j) {
                node.id = add[i * 100 + j];
                exec(&add_node);
            }
            duration = clock() - clock_start;
            fprintf(add_file, "%lu, %lu\n", x, duration);
            fflush(add_file);
            x += 100;
            FILE *storage_size_file = fopen(storage_size_name, "w+");
            fprintf(storage_size_file, "%lu", x);
            fclose(storage_size_file);

        }
    }
}


void del_test() {
    char *db_name = "../del.db";
    char *csv_name = "../del.csv";

    schema_t *schema = init_db(db_name);
    index_t empty_node_index;
    if (schema->first_index != 0) {
        index_t *i = get_first_index(schema, "empty_node_type");
        empty_node_index = *i;
        free_index(i);
        close(schema->fd);
        free(schema);
    }

    query_t delete_node = {
            .filename = db_name,
            .index = &empty_node_index,
            .q_type = DELETE,
            .target=Q_NODE,
            .query_body.del.id = 1
    };

    FILE *del_file = fopen(csv_name, "w+");

    fprintf(del_file, "op_cnt,time\n");

    uint32_t del_size = 100;
    uint64_t cnt;
    uint32_t *old = get_stored(&cnt);
    uint32_t delete[del_size];
    uint64_t clock_start;
    uint64_t duration;
    uint64_t x = 0;
    while (true && cnt > 0) {
        to_delete(delete, del_size, &old, cnt, 0, 0);
        for (uint32_t i = 0; i < del_size / 100; ++i) {
            clock_start = clock();
            for (uint32_t j = 0; j < 100; ++j) {
                delete_node.query_body.del.id = delete[i * 100 + j];
                exec(&delete_node);
            }
            duration = clock() - clock_start;
            fprintf(del_file, "%lu, %lu\n", x, duration);
            fflush(del_file);
            x += 100;
        }
        cnt -= 100;
    }


}

int main(){
    printf("jopa");
//    add_del_test();
//    add_test();
//    del_test();
}
