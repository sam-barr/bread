#include "common.h"
#include "vm.h"

/* http://www.cse.yorku.ca/~oz/hash.html djb2 hash algorithm */
static unsigned long
hash(char *str)
{
        unsigned long hash = 5381;
        int c;

        while ((c = *str++))
                hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
}

void
flt_value_map_init(struct flt_value_map *map)
{
        for (int i = 0; i < BUCKET_SIZE; i++) {
                map->bucket[i].key = "";
                map->bucket[i].next = NULL;
        }
}

void
flt_value_map_destroy(struct flt_value_map *map)
{
        /* At this point in time I'm not going to free the strings */

        for (int i = 0; i < BUCKET_SIZE; i++) {
                struct flt_value_map_list *list = map->bucket[i].next;
                
                while (list != NULL) {
                        struct flt_value_map_list *next = list->next;
                        free(list);
                        list = next;
                }
        }
}

void
flt_value_map_set(struct flt_value_map *map, char *key, struct flt_value *val)
{
        unsigned long h = hash(key) % BUCKET_SIZE;
        struct flt_value_map_list *list = &map->bucket[h];

        /*
         * Always at least 1 iteration since each bucket entry
         * has an initial dummy value
         */
        for (;;) {
                if (strcmp(list->key, key) == 0) {
                        list->val = *val;
                        return;
                } else if (list->next == NULL) {
                        break;
                } else {
                        list = list->next;
                }
        }

        /* key not in list */
        list->next = malloc(sizeof(struct flt_value_map_list));
        list->next->key = key;
        list->next->val = *val;
        list->next->next = NULL;
}

struct flt_value *
flt_value_map_get(struct flt_value_map *map, char *key)
{
        unsigned long h = hash(key) % BUCKET_SIZE;
        struct flt_value_map_list *list = &map->bucket[h];

        do {
                if (strcmp(list->key, key) == 0) {
                        return &list->val;
                }
        } while ((list = list->next) != NULL);

        return NULL;
}