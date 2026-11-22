#ifndef DS_HASHINDEX_H
#define DS_HASHINDEX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IndexEntry {
    char* key;
    char** ids;
    int count;
    int cap;
    struct IndexEntry* next;
} IndexEntry;

typedef struct HashIndex {
    IndexEntry** buckets;
    int nbuckets;
    int nkeys;
} HashIndex;

HashIndex* hi_create(int nbuckets);
int hi_add(HashIndex* h, const char* key, const char* id);          
const char* const* hi_get(const HashIndex* h, const char* key, int* count);
int hi_key_count(const HashIndex* h);
void hi_foreach_key(const HashIndex* h,
                    void (*cb)(const char* key, int count, void* ctx), void* ctx);
void hi_destroy(HashIndex* h);

#ifdef __cplusplus
}
#endif
#endif