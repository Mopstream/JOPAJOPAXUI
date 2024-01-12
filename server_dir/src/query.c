#include <unistd.h>
#include "../includes/query.h"
#include "stdlib.h"


void exec(query_t *q) {
    schema_t * schema = init_db(q->filename);
    switch (q->q_type) {
        case ADD: {
            switch (q->target) {
                case Q_INDEX: {
                    add_index(q->index, schema);
                    break;
                }
                case Q_NODE: {
                    add_node(schema, q->index, q->query_body.add.node);
                    break;
                }
                case Q_LINK: {
                    add_link(schema, q->index, q->query_body.add.link);
                    break;
                }
            }
            break;
        }
        case SELECT: {
            switch (q->target)  {
                case Q_INDEX: {
                    index_enumerate(schema);
                    break;
                }
                case Q_NODE: {
                    node_enumerate(schema, q->index, q->query_body.sel);
                    break;
                }
                case Q_LINK: {
                    link_enumerate(schema, q->index);
                    break;
                }
            }
            break;
        }
        case DELETE: {
            switch (q->target) {
                case Q_INDEX: {
                    delete_index_by_name(schema, q->index->type.type_name);
                    break;
                }
                case Q_NODE: {
                    delete_node_by_id(schema, q->index, q->query_body.del.id);
                    break;
                }
                case Q_LINK: {
                    delete_link_by_id(schema, q->index, q->query_body.del.id);
                    break;
                }
            }
            break;
        }
        case SET: {
            if (q->target == Q_NODE) {
                set_node_attribute(schema, q->index, q->query_body.set.node_id, q->query_body.set.attr_name,
                                   q->query_body.set.new_value);
            }
            break;
        }
    }
    close(schema->fd);
    free(schema);
}