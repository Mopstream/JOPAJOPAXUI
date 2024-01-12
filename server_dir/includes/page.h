#ifndef LLP_LAB1_PAGE_H_
#define LLP_LAB1_PAGE_H_

#include <stdint.h>

#include "schema.h"

#define PAGE_SIZE 4096

typedef enum {
    HEADER,
    INDEX,
    NODE,
    LINK,
    FREE
} page_type_t;

typedef struct {
    uint32_t this_page;
    uint32_t next_page;
    uint32_t prev_page;
    uint32_t free_space;
    uint32_t chunks_count;
    page_type_t page_type;
} page_header_t;


//typedef struct {
//    uint32_t size;
//    char *data;
//} chunk_t;

typedef struct page{
    page_header_t *header;
    chunk_t *chunks;
} page_t;

page_t *read_page(uint32_t fd, uint32_t page_num);
void write_page(uint32_t fd, page_t *page);
page_header_t *read_header(uint32_t fd, uint32_t page_num);
void write_header(uint32_t fd, page_header_t * header);
void free_page(schema_t *schema, uint32_t i);
page_t *get_free_page(schema_t *schema);
page_t * alloc_page(void);
page_header_t *alloc_header(void);
void add_chunk(schema_t *schema, page_t *p, chunk_t *new_chunk);
void destroy_page(page_t *page);
void delete_chunk(schema_t *schema, page_t *page, uint32_t i);
void shrink(schema_t *schema);
void save(schema_t *schema);
#endif
