#ifndef DS_STACK_H
#define DS_STACK_H
/* Stack in plain C (struct + pointers + malloc/free).
   Used by the admin to keep a reviewable log of decisions (LIFO). */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Decision {
    char claimId[32];
    char itemId[32];
    char claimant[64];
    char admin[32];
    int approved;       /* 1 = approved, 0 = rejected */
    char note[128];
    long when;          /* time stamp (seconds) */
} Decision;

typedef struct StackNode {
    Decision data;
    struct StackNode* next;
} StackNode;

typedef struct Stack {
    StackNode* top;
    int size;
} Stack;

Stack* stack_create(void);
int    stack_push(Stack* s, const Decision* d);   /* 1 = ok, 0 = out of memory */
int    stack_pop(Stack* s, Decision* out);        /* 1 = ok, 0 = empty */
int    stack_peek(const Stack* s, Decision* out); /* 1 = ok, 0 = empty */
int    stack_size(const Stack* s);
/* visits entries from top (newest) to bottom (oldest) */
void   stack_foreach(const Stack* s, void (*cb)(const Decision*, void*), void* ctx);
void   stack_destroy(Stack* s);

#ifdef __cplusplus
}
#endif
#endif
