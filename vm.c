#include "common.h"
#include "ast.h"
#include "value.h"
#include "vm.h"

#include <math.h>

#define LIST_SIZE 32
#define GROW 1.5

struct brd_vm vm;

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

#ifdef DEBUG
static void
brd_bytecode_debug(enum brd_bytecode op)
{
        printf(" -------- ");
        switch (op) {
        case BRD_VM_NUM: printf("BRD_VM_NUM\n"); return;
        case BRD_VM_STR: printf("BRD_VM_STR\n"); return;
        case BRD_VM_GET_VAR: printf("BRD_VM_GET_VAR\n"); return;
        case BRD_VM_TRUE: printf("BRD_VM_TRUE\n"); return;
        case BRD_VM_FALSE: printf("BRD_VM_FALSE\n"); return;
        case BRD_VM_UNIT: printf("BRD_VM_UNIT\n"); return;
        case BRD_VM_PLUS: printf("BRD_VM_PLUS\n"); return;
        case BRD_VM_MINUS: printf("BRD_VM_MINUS\n"); return;
        case BRD_VM_MUL: printf("BRD_VM_MUL\n"); return;
        case BRD_VM_DIV: printf("BRD_VM_DIV\n"); return;
        case BRD_VM_IDIV: printf("BRD_VM_IDIV\n"); return;
        case BRD_VM_MOD: printf("BRD_VM_MOD\n"); return;
        case BRD_VM_POW: printf("BRD_VM_POW\n"); return;
        case BRD_VM_NEGATE: printf("BRD_VM_NEGATE\n"); return;
        case BRD_VM_LT: printf("BRD_VM_LT\n"); return;
        case BRD_VM_LEQ: printf("BRD_VM_LEQ\n"); return;
        case BRD_VM_GT: printf("BRD_VM_GT\n"); return;
        case BRD_VM_GEQ: printf("BRD_VM_GEQ\n"); return;
        case BRD_VM_EQ: printf("BRD_VM_EQ\n"); return;
        case BRD_VM_NOT: printf("BRD_VM_NOT\n"); return;
        case BRD_VM_TEST: printf("BRD_VM_TEST\n"); return;
        case BRD_VM_TESTN: printf("BRD_VM_TESTN\n"); return;
        case BRD_VM_TESTP: printf("BRD_VM_TESTP\n"); return;
        case BRD_VM_SET_VAR: printf("BRD_VM_SET_VAR\n"); return;
        case BRD_VM_JMP: printf("BRD_VM_JMP\n"); return;
        case BRD_VM_RETURN: printf("BRD_VM_RETURN\n"); return;
        case BRD_VM_POP: printf("BRD_VM_POP\n"); return;
        case BRD_VM_CONCAT: printf("BRD_VM_CONCAT\n"); return;
        case BRD_VM_CALL: printf("BRD_VM_CALL\n"); return;
        case BRD_VM_BUILTIN: printf("BRD_VM_BUILTIN\n"); return;
        case BRD_VM_LIST: printf("BRD_VM_LIST\n"); return;
        case BRD_VM_PUSH: printf("BRD_VM_PUSH\n"); return;
        }
        printf("oops\n");
}
#endif

void
brd_value_map_init(struct brd_value_map *map)
{
        for (int i = 0; i < BUCKET_SIZE; i++) {
                map->bucket[i].key = "";
                map->bucket[i].val.vtype = BRD_VAL_UNIT;
                map->bucket[i].next = NULL;
        }
}

void
brd_value_map_destroy(struct brd_value_map *map)
{
        /* At this point in time I'm not going to free the strings */

        for (int i = 0; i < BUCKET_SIZE; i++) {
                struct brd_value_map_list *list = map->bucket[i].next;
                
                while (list != NULL) {
                        struct brd_value_map_list *next = list->next;
                        free(list);
                        list = next;
                }
        }
}

void
brd_value_map_set(struct brd_value_map *map, char *key, struct brd_value *val)
{
        unsigned long h = hash(key) % BUCKET_SIZE;
        struct brd_value_map_list *list = &map->bucket[h];

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
        list->next = malloc(sizeof(struct brd_value_map_list));
        list->next->key = key;
        list->next->val = *val;
        list->next->next = NULL;
}

struct brd_value *
brd_value_map_get(struct brd_value_map *map, char *key)
{
        unsigned long h = hash(key) % BUCKET_SIZE;
        struct brd_value_map_list *list = &map->bucket[h];

        do {
                if (strcmp(list->key, key) == 0) {
                        return &list->val;
                }
        } while ((list = list->next) != NULL);

        return NULL;
}

void
brd_stack_push(struct brd_stack *stack, struct brd_value *value)
{
        *(stack->sp++) = *value;
}

struct brd_value *
brd_stack_pop(struct brd_stack *stack)
{
        return --stack->sp;
}

struct brd_value *
brd_stack_peek(struct brd_stack *stack)
{
        return stack->sp - 1;
}

void
brd_vm_allocate(struct brd_heap_entry *entry)
{
        entry->next = vm.heap->next;
        vm.heap->next = entry;
}

#define ADD_OP(x) do {\
        if(sizeof(enum brd_bytecode) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(enum brd_bytecode *)(*bytecode + *length) = (x);\
        *length += sizeof(enum brd_bytecode);\
} while (0)

#define ADD_SIZET(x) do {\
        if(sizeof(size_t) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(size_t *)(*bytecode + *length) = (x);\
        *length += sizeof(size_t);\
} while (0)

#define ADD_NUM(x) do {\
        if(sizeof(long double) + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        *(long double *)(*bytecode + *length) = (x);\
        *length += sizeof(long double);\
} while (0)

#define ADD_STR(x) do {\
        size_t len = strlen(x) + 1;\
        while (len + *length >= *capacity) {\
                *capacity *= GROW;\
                *bytecode = realloc(*bytecode, *capacity);\
        }\
        strcpy((*bytecode) + *length, (x));\
        *length += len;\
} while (0)

#define RECURSE_ON(x) (_brd_node_compile((x), bytecode, length, capacity))

#define AS(type, x) ((struct brd_node_ ## type *)(x))

static void
brd_node_compile_lvalue(
        struct brd_node *node,
        void **bytecode,
        size_t *length,
        size_t *capacity)
{
        switch (node->ntype) {
        case BRD_NODE_VAR:
                ADD_OP(BRD_VM_SET_VAR);
                ADD_STR(AS(var, node)->id);
                break;
        default:
                BARF("This shouldn't happen.");
        }
}

static void
_brd_node_compile(
        struct brd_node *node,
        void **bytecode,
        size_t *length,
        size_t *capacity)
{
        enum brd_bytecode op;
        size_t temp, jmp, *ifexpr_temps;

        switch (node->ntype) {
        case BRD_NODE_ASSIGN: 
                RECURSE_ON(AS(assign, node)->r);
                brd_node_compile_lvalue(AS(assign, node)->l, bytecode, length, capacity);
                break;
        case BRD_NODE_BINOP:
                /* yeah this code is ugly */
                switch (AS(binop, node)->btype) {
                case BRD_OR:
                case BRD_AND: 
                        RECURSE_ON(AS(binop, node)->l);
                        op = (AS(binop, node)->btype == BRD_AND)
                                ? BRD_VM_TEST : BRD_VM_TESTN;
                        ADD_OP(op);
                        ADD_OP(BRD_VM_JMP);
                        temp = *length;
                        ADD_SIZET(0);
                        RECURSE_ON(AS(binop, node)->r);
                        jmp = *length - temp;
                        *(size_t *)(*bytecode + temp) = jmp;
                        break;
                case BRD_PLUS: op = BRD_VM_PLUS; goto mkbinop;
                case BRD_MINUS: op = BRD_VM_MINUS; goto mkbinop;
                case BRD_MUL: op = BRD_VM_MUL; goto mkbinop;
                case BRD_DIV: op = BRD_VM_DIV; goto mkbinop;
                case BRD_IDIV: op = BRD_VM_IDIV; goto mkbinop;
                case BRD_MOD: op = BRD_VM_MOD; goto mkbinop;
                case BRD_POW: op = BRD_VM_POW; goto mkbinop;
                case BRD_CONCAT: op = BRD_VM_CONCAT; goto mkbinop;
                case BRD_LT: op = BRD_VM_LT; goto mkbinop;
                case BRD_LEQ: op = BRD_VM_LEQ; goto mkbinop;
                case BRD_GT: op = BRD_VM_GT; goto mkbinop;
                case BRD_GEQ: op = BRD_VM_GEQ; goto mkbinop;
                case BRD_EQ: op = BRD_VM_EQ; goto mkbinop;
mkbinop:
                        RECURSE_ON(AS(binop, node)->l);
                        RECURSE_ON(AS(binop, node)->r);
                        ADD_OP(op);
                }
                break;
        case BRD_NODE_UNARY:
                RECURSE_ON(AS(unary, node)->u);
                switch (AS(unary, node)->utype) {
                case BRD_NEGATE: ADD_OP(BRD_VM_NEGATE); break;
                case BRD_NOT: ADD_OP(BRD_VM_NOT); break;
                }
                break;
        case BRD_NODE_VAR:
                ADD_OP(BRD_VM_GET_VAR);
                ADD_STR(AS(var, node)->id);
                break;
        case BRD_NODE_NUM_LIT:
                ADD_OP(BRD_VM_NUM);
                ADD_NUM(AS(num_lit, node)->v);
                break;
        case BRD_NODE_STRING_LIT:
                ADD_OP(BRD_VM_STR);
                ADD_STR(AS(string_lit, node)->s);
                break;
        case BRD_NODE_BOOL_LIT:
                ADD_OP(AS(bool_lit, node)->b ? BRD_VM_TRUE : BRD_VM_FALSE);
                break;
        case BRD_NODE_UNIT_LIT:
                ADD_OP(BRD_VM_UNIT);
                break;
        case BRD_NODE_LIST_LIT:
                ADD_OP(BRD_VM_LIST);
                for (int i = 0; i < AS(list_lit, node)->items->num_args; i++) {
                        RECURSE_ON(AS(list_lit, node)->items->args[i]);
                        ADD_OP(BRD_VM_PUSH);
                }
                break;
        case BRD_NODE_FUNCALL:
                for (int i = 0; i < AS(funcall, node)->args->num_args; i++) {
                        RECURSE_ON(AS(funcall, node)->args->args[i]);
                }
                RECURSE_ON(AS(funcall, node)->fn);
                ADD_OP(BRD_VM_CALL);
                ADD_SIZET(AS(funcall, node)->args->num_args);
                break;
        case BRD_NODE_BUILTIN:
                ADD_OP(BRD_VM_BUILTIN);
                ADD_SIZET(brd_lookup_builtin(AS(builtin, node)->builtin));
                break;
        case BRD_NODE_BODY:
                for (int i = 0; i < AS(body, node)->num_stmts; i++) {
                        RECURSE_ON(AS(body, node)->stmts[i]);
                        if (i < AS(body, node)->num_stmts - 1) {
                                ADD_OP(BRD_VM_POP);
                        }
                }
                break;
        case BRD_NODE_IFEXPR:
                ifexpr_temps = malloc(sizeof(size_t)*(1+AS(ifexpr, node)->num_elifs));
                RECURSE_ON(AS(ifexpr, node)->cond);
                ADD_OP(BRD_VM_TESTP);
                ADD_OP(BRD_VM_JMP);
                temp = *length;
                ADD_SIZET(0);
                RECURSE_ON(AS(ifexpr, node)->body);
                ADD_OP(BRD_VM_JMP);
                ifexpr_temps[0] = *length;
                ADD_SIZET(0);
                jmp = *length - temp;
                *(size_t *)(*bytecode + temp) = jmp;

                for (int i = 0; i < AS(ifexpr, node)->num_elifs; i++) {
                        RECURSE_ON(AS(ifexpr, node)->elifs[i].cond);
                        ADD_OP(BRD_VM_TESTP);
                        ADD_OP(BRD_VM_JMP);
                        temp = *length;
                        ADD_SIZET(0);
                        RECURSE_ON(AS(ifexpr, node)->elifs[i].body);
                        ADD_OP(BRD_VM_JMP);
                        ifexpr_temps[i+1] = *length;
                        ADD_SIZET(0);
                        jmp = *length - temp;
                        *(size_t *)(*bytecode + temp) = jmp;
                }

                if (AS(ifexpr, node)->els != NULL) {
                        RECURSE_ON(AS(ifexpr, node)->els);
                } else {
                        ADD_OP(BRD_VM_UNIT);
                }

                for (int i = 0; i < AS(ifexpr, node)->num_elifs + 1; i++) {
                        jmp = *length - ifexpr_temps[i];
                        *(size_t *)(*bytecode + ifexpr_temps[i]) = jmp;
                }

                free(ifexpr_temps);
                break;
        case BRD_NODE_PROGRAM:
                for (int i = 0; i < AS(program, node)->num_stmts; i++) {
                        RECURSE_ON(AS(program, node)->stmts[i]);
                        ADD_OP(BRD_VM_POP);
                }
                ADD_OP(BRD_VM_RETURN);
                break;
        case BRD_NODE_MAX:
                BARF("This shouldn't happen.");
        }
}

#undef ADD_OP
#undef ADD_NUM
#undef ADD_STR
#undef RECURSE_ON
#undef AS

void *
brd_node_compile(struct brd_node *node)
{
        size_t length = 0, capacity = LIST_SIZE;
        void *bytecode = malloc(capacity);
        (void)brd_node_compile_lvalue;

        _brd_node_compile(node, &bytecode, &length, &capacity);
        return bytecode;
}

void
brd_vm_destroy()
{
        /* note to self: for container types, don't free the members! */
        struct brd_heap_entry *heap = vm.heap;
        while (heap != NULL) {
                struct brd_heap_entry *n = heap->next;
                switch(heap->htype) {
                case BRD_HEAP_STRING:
                        free(heap->as.string);
                        break;
                case BRD_HEAP_LIST:
                        free(heap->as.list->items);
                        free(heap->as.list);
                        break;
                }
                free(heap);
                heap = n;
        }
        brd_value_map_destroy(&vm.globals);
        free(vm.bytecode);
}

void
brd_vm_init(void *bytecode)
{
        vm.heap = malloc(sizeof(*vm.heap));
        vm.heap->next = NULL;
        vm.heap->htype = BRD_HEAP_STRING;
        vm.heap->as.string = malloc(1);
        brd_value_map_init(&vm.globals);
        vm.bytecode = bytecode;
        vm.pc = 0;
        vm.stack.sp = vm.stack.values;
}

void
brd_vm_run()
{
        enum brd_bytecode op;
        struct brd_value value1, value2;
        char *id;
        int go = true;
        size_t jmp, num_args;

        while (go) {
                op = *(enum brd_bytecode *)(vm.bytecode + vm.pc);
                vm.pc += sizeof(enum brd_bytecode);

#ifdef DEBUG
                brd_bytecode_debug(op);
#endif

                switch (op) {
                case BRD_VM_NUM:
                        value1.vtype = BRD_VAL_NUM;
                        value1.as.num = *(long double *)(vm.bytecode + vm.pc);
                        vm.pc += sizeof(long double);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_STR:
                        value1.vtype = BRD_VAL_STRING;
                        value1.as.string = (char *)(vm.bytecode + vm.pc);
                        while (*(char *)(vm.bytecode + vm.pc++));
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_GET_VAR:
                        id = (char *)(vm.bytecode + vm.pc);
                        while (*(char *)(vm.bytecode + vm.pc++));
                        value1.vtype = BRD_VAL_UNIT;
                        value1 = *(brd_value_map_get(&vm.globals, id) ?: &value1);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_TRUE:
                        value1.vtype = BRD_VAL_BOOL;
                        value1.as.boolean = true;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_FALSE:
                        value1.vtype = BRD_VAL_BOOL;
                        value1.as.boolean = false;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_UNIT:
                        value1.vtype = BRD_VAL_UNIT;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_PLUS:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num += value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_MINUS:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num -= value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_MUL:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num *= value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_DIV:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num /= value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_IDIV:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = floorl(
                                value2.as.num / value1.as.num
                        );
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_MOD:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = (long long int) value2.as.num
                                % (long long int) value1.as.num;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_POW:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        brd_value_coerce_num(&value2);
                        value2.as.num = powl(
                                value2.as.num,
                                value1.as.num
                        );
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_CONCAT:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        brd_value_concat(&value2, &value1);
                        brd_vm_allocate(value2.as.heap);
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_LT:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) < 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_LEQ:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) <= 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_GT:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) > 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_GEQ:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) >= 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_EQ:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_pop(&vm.stack);
                        value2.as.boolean = brd_value_compare(&value2, &value1) == 0;
                        value2.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_NEGATE:
                        value1 = *brd_stack_pop(&vm.stack);
                        brd_value_coerce_num(&value1);
                        value1.as.num *= -1;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_NOT:
                        value1 = *brd_stack_pop(&vm.stack);
                        value1.as.boolean = !brd_value_truthify(&value1);
                        value1.vtype = BRD_VAL_BOOL;
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_TEST:
                        if (brd_value_truthify(brd_stack_peek(&vm.stack))) {
                                brd_stack_pop(&vm.stack);
                                vm.pc += sizeof(enum brd_bytecode) + sizeof(size_t);
                        }
                        break;
                case BRD_VM_TESTN:
                        if (!brd_value_truthify(brd_stack_peek(&vm.stack))) {
                                brd_stack_pop(&vm.stack);
                                vm.pc += sizeof(enum brd_bytecode) + sizeof(size_t);
                        }
                        break;
                case BRD_VM_TESTP:
                        if (brd_value_truthify(brd_stack_pop(&vm.stack))) {
                                vm.pc += sizeof(enum brd_bytecode) + sizeof(size_t);
                        }
                        break;
                case BRD_VM_SET_VAR:
                        id = (char *)(vm.bytecode + vm.pc);
                        while (*(char *)(vm.bytecode + vm.pc++));
                        value1 = *brd_stack_pop(&vm.stack);
                        brd_value_map_set(&vm.globals, id, &value1);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_JMP:
                        jmp = *(size_t *)(vm.bytecode + vm.pc);
                        vm.pc += jmp;
                        break;
                case BRD_VM_BUILTIN:
                        value1.vtype = BRD_VAL_BUILTIN;
                        value1.as.builtin = *(size_t *)(vm.bytecode + vm.pc);
                        vm.pc += sizeof(size_t);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_CALL:
                        num_args = *(size_t *)(vm.bytecode + vm.pc);
                        vm.pc += sizeof(size_t);
                        value1 = *brd_stack_pop(&vm.stack);
                        vm.stack.sp -= num_args;
                        brd_value_call(&value1, vm.stack.sp, num_args, &value2);
                        brd_stack_push(&vm.stack, &value2);
                        break;
                case BRD_VM_LIST:
                        value1.vtype = BRD_VAL_HEAP;
                        value1.as.heap = malloc(sizeof(*value1.as.heap));
                        value1.as.heap->htype = BRD_HEAP_LIST;
                        value1.as.heap->as.list = malloc(
                                sizeof(struct brd_value_list)
                        );
                        brd_value_list_init(value1.as.heap->as.list);
                        brd_vm_allocate(value1.as.heap);
                        brd_stack_push(&vm.stack, &value1);
                        break;
                case BRD_VM_PUSH:
                        value1 = *brd_stack_pop(&vm.stack);
                        value2 = *brd_stack_peek(&vm.stack);
                        brd_value_list_push(
                                value2.as.heap->as.list,
                                &value1
                        );
                        break;
                case BRD_VM_RETURN:
                        go = false;
                        break;
                case BRD_VM_POP:
#ifdef DEBUG
                        value1 = *brd_stack_pop(&vm.stack);
                        brd_value_debug(&value1);
#else
                        brd_stack_pop(&vm.stack);
#endif
                        brd_vm_gc();
                        break;
                }
        }
}

void
brd_vm_gc()
{
        struct brd_heap_entry *heap, *prev;

        heap = vm.heap->next;
        while (heap != NULL) {
                heap->marked = false;
                heap = heap->next;
        }

        /* mark values in the stack */
        for (struct brd_value *p = vm.stack.values; p < vm.stack.sp; p++) {
                if (p->vtype == BRD_VAL_HEAP) {
                        p->as.heap->marked = true;
                }
        }

        /* mark values held by variables */
        for (int i = 0; i < BUCKET_SIZE; i++) {
                struct brd_value_map_list *p = &vm.globals.bucket[i];
                while (p != NULL) {
                        if (p->val.vtype == BRD_VAL_HEAP) {
                                brd_heap_mark(p->val.as.heap);
                        }
                        p = p->next;
                }
        }

        prev = vm.heap;
        heap = prev->next;
        while (heap != NULL) {
                struct brd_heap_entry *next = heap->next;
                if (heap->marked) {
                        prev = heap;
                        heap = next;
                        continue;
                }

                prev->next = next;
                switch(heap->htype) {
                case BRD_HEAP_STRING:
                        free(heap->as.string);
                        break;
                case BRD_HEAP_LIST:
                        free(heap->as.list->items);
                        free(heap->as.list);
                }
                free(heap);
                heap = next;
        }
}
