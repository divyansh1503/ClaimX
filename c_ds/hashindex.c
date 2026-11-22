#include <stdlib.h>
#include <string.h>
#include "hashindex.h"

static char* dup_str(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static unsigned long hash_str(const char* s) {  /* djb2 */
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*s++) != 0) h = ((h << 5) + h) + (unsigned long)c;
    return h;
}

HashIndex* hi_create(int nbuckets) {
    HashIndex* h;
    if (nbuckets < 1) nbuckets = 31;
    h = (HashIndex*)malloc(sizeof(HashIndex));
    if (!h) return NULL;
    h->buckets = (IndexEntry**)calloc((size_t)nbuckets, sizeof(IndexEntry*));
    if (!h->buckets) { free(h); return NULL; }
    h->nbuckets = nbuckets;
    h->nkeys = 0;
    return h;
}

static IndexEntry* find_entry(const HashIndex* h, const char* key) {
    IndexEntry* e = h->buckets[hash_str(key) % (unsigned long)h->nbuckets];
    while (e) {
        if (strcmp(e->key, key) == 0) return e;
        e = e->next;
    }
    return NULL;
}

int hi_add(HashIndex* h, const char* key, const char* id) {
    IndexEntry* e;
    int i;
    if (!h || !key || !id) return 0;
    e = find_entry(h, key);
    if (!e) {
        unsigned long b = hash_str(key) % (unsigned long)h->nbuckets;
        e = (IndexEntry*)malloc(sizeof(IndexEntry));
        if (!e) return 0;
        e->key = dup_str(key);
        e->cap = 4;
        e->count = 0;
        e->ids = (char**)malloc((size_t)e->cap * sizeof(char*));
        if (!e->key || !e->ids) { free(e->key); free(e->ids); free(e); return 0; }
        e->next = h->buckets[b];
        h->buckets[b] = e;
        h->nkeys++;
    }
    for (i = 0; i < e->count; i++)
        if (strcmp(e->ids[i], id) == 0) return 1;  /* already there */
    if (e->count == e->cap) {
        char** bigger = (char**)realloc(e->ids, (size_t)(e->cap * 2) * sizeof(char*));
        if (!bigger) return 0;
        e->ids = bigger;
        e->cap *= 2;
    }
    e->ids[e->count] = dup_str(id);
    if (!e->ids[e->count]) return 0;
    e->count++;
    return 1;
}

const char* const* hi_get(const HashIndex* h, const char* key, int* count) {
    IndexEntry* e;
    if (count) *count = 0;
    if (!h || !key) return NULL;
    e = find_entry(h, key);
    if (!e) return NULL;
    if (count) *count = e->count;
    return (const char* const*)e->ids;
}

int hi_key_count(const HashIndex* h) { return h ? h->nkeys : 0; }

void hi_foreach_key(const HashIndex* h,
                    void (*cb)(const char* key, int count, void* ctx), void* ctx) {
    int b;
    IndexEntry* e;
    if (!h || !cb) return;
    for (b = 0; b < h->nbuckets; b++)
        for (e = h->buckets[b]; e; e = e->next) cb(e->key, e->count, ctx);
}

void hi_destroy(HashIndex* h) {
    int b, i;
    if (!h) return;
    for (b = 0; b < h->nbuckets; b++) {
        IndexEntry* e = h->buckets[b];
        while (e) {
            IndexEntry* next = e->next;
            for (i = 0; i < e->count; i++) free(e->ids[i]);
            free(e->ids);
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(h->buckets);
    free(h);
}
