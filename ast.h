#ifndef BRD_AST_H
#define BRD_AST_H

/*
 * Nodes are ALWAYS allocated on the heap!
 */

enum brd_node_type {
        BRD_NODE_ASSIGN,
        BRD_NODE_BINOP,
        BRD_NODE_UNARY,
        BRD_NODE_VAR,
        BRD_NODE_NUM_LIT,
        BRD_NODE_STRING_LIT,
        BRD_NODE_BOOL_LIT,
        BRD_NODE_UNIT_LIT,
        BRD_NODE_LIST_LIT,
        BRD_NODE_FUNCALL,
        BRD_NODE_CLOSURE,
        BRD_NODE_BUILTIN,
        BRD_NODE_BODY,
        BRD_NODE_IFEXPR,
        BRD_NODE_INDEX,
        BRD_NODE_WHILE,
        BRD_NODE_FIELD,
        BRD_NODE_ACC_OBJ,
        BRD_NODE_SUBCLASS,
        BRD_NODE_DICT,

        BRD_NODE_PROGRAM, /* the top level program */
};

struct brd_node;
typedef void (*brd_node_fn)(struct brd_node *);
typedef struct brd_node * (*brd_node_fn2)(struct brd_node *);

struct brd_node {
        int line_number;
        enum brd_node_type ntype;
        char _p[3];
};

struct brd_node_program {
        struct brd_node _node;
        struct brd_node **stmts;
        size_t num_stmts;
};

struct brd_node_assign {
        struct brd_node _node;
        struct brd_node *l, *r;
};

enum brd_binop {
        /* arith */
        // NOTE: do not put anything between plus and concat
        // must match with token.h here
        BRD_PLUS = 0,
        BRD_MINUS,
        BRD_MUL,
        BRD_DIV,
        BRD_IDIV,
        BRD_MOD,
        BRD_POW,
        BRD_CONCAT,

        /* comp */
        BRD_LT,
        BRD_LEQ,
        BRD_GT,
        BRD_GEQ,
        BRD_EQ,

        /* boolean */
        BRD_AND,
        BRD_OR,

};

struct brd_node_binop {
        struct brd_node _node;
        struct brd_node *l, *r;
        enum brd_binop btype;
        char _p[7];
};

enum brd_unary {
        BRD_NEGATE,
        BRD_NOT,
};

struct brd_node_unary {
        struct brd_node _node;
        struct brd_node *u;
        enum brd_unary utype;
        char _p[7];
};

struct brd_node_var {
        struct brd_node _node;
        char *id;
};

struct brd_node_num_lit {
        struct brd_node _node;
        char _p[8];
        long double v;
};

struct brd_node_string_lit {
        struct brd_node _node;
        char *s;
};

struct brd_node_bool_lit {
        struct brd_node _node;
        int b;
};

struct brd_node_unit_lit {
        struct brd_node _node;
};


struct brd_node_arglist {
        size_t num_args;
        struct brd_node **args;
};

struct brd_node_list_lit {
        struct brd_node _node;
        struct brd_node_arglist *items;
};

struct brd_node_funcall {
        struct brd_node _node;
        struct brd_node *fn;
        struct brd_node_arglist *args;
};

struct brd_node_closure {
        struct brd_node _node;
        char **args;
        size_t num_args;
        struct brd_node *body;
};

struct brd_node_builtin {
        struct brd_node _node;
        char *builtin;
};

struct brd_node_body {
        struct brd_node _node;
        struct brd_node **stmts;
        size_t num_stmts;
};

struct brd_node_elif {
        struct brd_node *cond;
        struct brd_node *body;
};

struct brd_node_ifexpr {
        struct brd_node _node;
        struct brd_node *cond;
        struct brd_node *body;
        struct brd_node_elif *elifs;
        size_t num_elifs;
        struct brd_node *els;
        // els can be null, in which case unit is the value of the expression
};

struct brd_node_index {
        struct brd_node _node;
        struct brd_node *list;
        struct brd_node *idx;
};

struct brd_node_while {
        struct brd_node _node;
        struct brd_node *cond;
        struct brd_node *body;
        struct brd_node *inc;
        int no_list;
        char _p[4];
        // inc can be null
};

struct brd_node_field {
        struct brd_node _node;
        struct brd_node *object;
        char *field;
};

struct brd_node_acc_obj {
        struct brd_node _node;
        struct brd_node *object;
        char *id;
};

struct brd_node_subclass_set {
        char *id;
        struct brd_node *expression;
};

struct brd_node_subclass {
        struct brd_node _node;
        struct brd_node *super;
        struct brd_node *constructor;
        struct brd_node_subclass_set *decs;
        size_t num_decs;
};

struct brd_node_dict_pair {
        char *key;
        struct brd_node *value;
};

struct brd_node_dict {
        struct brd_node _node;
        struct brd_node_dict_pair *pairs;
        size_t num_pairs;
};

struct brd_node *brd_node_program_new(struct brd_node **stmts, size_t num_stmts);
struct brd_node *brd_node_assign_new(struct brd_node *l, struct brd_node *r);
struct brd_node *brd_node_binop_new(enum brd_binop btype, struct brd_node *l, struct brd_node *r);
struct brd_node *brd_node_unary_new(enum brd_unary utype, struct brd_node *u);
struct brd_node *brd_node_var_new(char *id);
struct brd_node *brd_node_num_lit_new(long double v);
struct brd_node *brd_node_string_lit_new(char *s);
struct brd_node *brd_node_bool_lit_new(int b);
struct brd_node *brd_node_unit_lit_new(void);
void brd_node_arglist_destroy(struct brd_node_arglist *args);
struct brd_node_arglist *brd_node_arglist_new(struct brd_node **args, size_t num_args);
struct brd_node *brd_node_list_lit_new(struct brd_node_arglist *items);
struct brd_node *brd_node_funcall_new(struct brd_node *fn, struct brd_node_arglist *args);
struct brd_node *brd_node_closure_new(char **args, size_t num_args, struct brd_node *body);
struct brd_node *brd_node_builtin_new(char *builtin);
struct brd_node *brd_node_body_new(struct brd_node **stmts, size_t num_stmts);
struct brd_node *brd_node_ifexpr_new(struct brd_node *cond, struct brd_node *body, struct brd_node_elif *elifs, size_t num_elifs, struct brd_node *els);
struct brd_node *brd_node_index_new(struct brd_node *list, struct brd_node *idx);
struct brd_node *brd_node_while_new(int no_list, struct brd_node *cond, struct brd_node *body, struct brd_node *inc);
struct brd_node *brd_node_field_new(struct brd_node *object, char *field);
struct brd_node *brd_node_acc_obj_new(struct brd_node *object, char *id);
struct brd_node *brd_node_subclass_new(struct brd_node *super, struct brd_node *constructor, struct brd_node_subclass_set *decs, size_t num_decs);
struct brd_node *brd_node_dict_new(struct brd_node_dict_pair *pairs, size_t num_pairs);

void brd_node_destroy(struct brd_node *node);
struct brd_node *brd_node_copy(struct brd_node *node);

#endif
