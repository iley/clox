#include "table.h"

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

static entry_t* find_entry(entry_t* entries, int capacity, obj_string_t* key);
static void table_adjust_capacity(table_t* table, int new_capacity);

void table_init(table_t* table) {
  table->count = 0;
  table->capacity = -1;
  table->entries = NULL;
}

void table_free(table_t* table) {
  FREE_ARRAY(entry_t, table->entries, table->capacity + 1);
  table_init(table);
}

bool table_set(table_t* table, obj_string_t* key, value_t value) {
  if (table->count + 1 > (table->capacity + 1) * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity + 1) - 1;
    table_adjust_capacity(table, capacity);
  }

  entry_t* entry = find_entry(table->entries, table->capacity, key);

  bool is_new_key = entry->key == NULL;
  // only increment the count if inserting in a truly empty slot, not a tombstone
  if (is_new_key && IS_NIL(entry->value)) {
    table->count++;
  }

  entry->key = key;
  entry->value = value;

  return is_new_key;
}

bool table_get(table_t* table, obj_string_t* key, value_t* value) {
  if (table->count == 0) {
    return false;
  }

  entry_t* entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }

  *value = entry->value;
  return true;
}

bool table_delete(table_t* table, obj_string_t* key) {
  if (table->count == 0) {
    return false;
  }

  entry_t* entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) {
    return false;
  }

  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

void table_add_all(table_t* from, table_t* to) {
  for (int i = 0; i <= from->capacity; i++) {
    entry_t* entry = &from->entries[i];
    if (entry->key != NULL) {
      table_set(to, entry->key, entry->value);
    }
  }
}

obj_string_t* table_find_string(table_t* table, const char* chars, int length, uint32_t hash) {
  if (table->count == 0) {
    return NULL;
  }

  uint32_t index = hash & table->capacity;
  for (;;) {
    entry_t* entry = &table->entries[index];
    if (entry->key == NULL) {
      // stop if we find an empty non-tombstone entry
      if (IS_NIL(entry->value)) {
        return NULL;
      }
    } else if (entry->key->length == length &&
               entry->key->hash == hash &&
               memcmp(entry->key->chars, chars, length) == 0) {
      return entry->key;
    }

    index = (index + 1) & table->capacity;
  }
}

void mark_table(table_t* table) {
  for (int i = 0; i <= table->capacity; i++) {
    entry_t* entry = &table->entries[i];
    mark_object((obj_t*)entry->key);
    mark_value(entry->value);
  }
}

void table_remove_white(table_t* table) {
  for (int i = 0; i <= table->capacity; i++) {
    entry_t* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.is_marked) {
      table_delete(table, entry->key);
    }
  }
}

static entry_t* find_entry(entry_t* entries, int capacity, obj_string_t* key) {
  uint32_t index = key->hash & capacity;
  entry_t* tombstone = NULL;
  for (;;) {
    entry_t* entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // empty entry
        return tombstone != NULL ? tombstone : entry;
      } else {
        // found a tombstone
        if (tombstone == NULL) {
          tombstone = entry;
        }
      }
    } else if (entry->key == key) {
      // found the entry
      return entry;
    }
    index = (index + 1) & capacity;
  }
}

static void table_adjust_capacity(table_t* table, int new_capacity) {
  entry_t* new_entries = ALLOCATE(entry_t, new_capacity + 1);
  for (int i = 0; i <= new_capacity; i++) {
    new_entries[i].key = NULL;
    new_entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i <= table->capacity; i++) {
    entry_t* entry = &table->entries[i];
    if (entry->key == NULL) {
      continue;
    }
    entry_t* dest = find_entry(new_entries, new_capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(entry_t, table->entries, table->capacity + 1);
  table->entries = new_entries;
  table->capacity = new_capacity;
}
