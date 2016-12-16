#include "table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Table *table_new(Table *parent) {
	Table *new = malloc(sizeof(Table));
	new->parent = parent;
	new->length = 0;
	return new;
}
void table_add(Table *tbl, char *name, int value) {
	tbl->names[tbl->length] = name;
	tbl->values[tbl->length] = value;
	tbl->length++;
	if(tbl->length > 1024) {
		fputs("Added too many entries to a single table.", stderr);
	}
}
int *table_get(Table *tbl, char *name) {
	for(size_t i = 0; i < tbl->length; i++) {
		if(strcmp(tbl->names[i], name) == 0) {
			return tbl->values + i;
		}
	}
	if(tbl->parent != NULL) {
		return table_get(tbl, name);
	} else {
		return NULL;
	}
}
