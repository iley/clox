#ifndef _CLOX_TABLE_H
#define _CLOX_TABLE_H

#include "value.h"

typedef struct {
  obj_string_t* key;
  value_t value;
} entry_t;

typedef struct {
  int count;
  int capacity;
  entry_t* entries;
} table_t;

void table_init(table_t* table);
void table_free(table_t* table);
bool table_set(table_t* table, obj_string_t* key, value_t value);
bool table_get(table_t* table, obj_string_t* key, value_t* value);
bool table_delete(table_t* table, obj_string_t* key);
void table_add_all(table_t* from, table_t* to);

#endif // _CLOX_TABLE_H
