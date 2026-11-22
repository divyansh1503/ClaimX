#include <stdlib.h>
#include "stack.h"

Stack* stack_create(void) {
    Stack* s = (Stack*)malloc(sizeof(Stack));
    if (!s) return NULL;
    s->top = NULL;
    s->size = 0;
    return s;
}

int stack_push(Stack* s, const Decision* d) {
    StackNode* n;
    if (!s || !d) return 0;
    n = (StackNode*)malloc(sizeof(StackNode));
    if (!n) return 0;
    n->data = *d;
    n->next = s->top;
    s->top = n;
    s->size++;
    return 1;
}

int stack_pop(Stack* s, Decision* out) {
    StackNode* n;
    if (!s || !s->top) return 0;
    n = s->top;
    if (out) *out = n->data;
    s->top = n->next;
    free(n);
    s->size--;
    return 1;
}

int stack_peek(const Stack* s, Decision* out) {
    if (!s || !s->top) return 0;
    if (out) *out = s->top->data;
    return 1;
}

int stack_size(const Stack* s) { return s ? s->size : 0; }

void stack_foreach(const Stack* s, void (*cb)(const Decision*, void*), void* ctx) {
    const StackNode* n;
    if (!s || !cb) return;
    for (n = s->top; n; n = n->next) cb(&n->data, ctx);
}

void stack_destroy(Stack* s) {
    if (!s) return;
    while (s->top) stack_pop(s, NULL);
    free(s);
}
