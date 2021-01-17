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

        BRD_NODE_PROGRAM, /* the top level program */
        BRD_NODE_MAX,
};

struct brd_node;
typedef void (*brd_node_fn)(struct brd_node *);

struct brd_node {
        brd_node_fn destroy;
        enum brd_node_type ntype;
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
        BRD_PLUS,
        BRD_MINUS,
        BRD_MUL,
        BRD_DIV,

        /* comp */
        BRD_LT,
        BRD_LEQ,
        BRD_GT,
        BRD_GEQ,
        BRD_EQ,

        /* boolean */
        BRD_AND,
        BRD_OR,

        BRD_CONCAT,
};

struct brd_node_binop {
        struct brd_node _node;
        struct brd_node *l, *r;
        enum brd_binop btype;
};

enum brd_unary {
        BRD_NEGATE,
        BRD_NOT,
};

struct brd_node_unary {
        struct brd_node _node;
        struct brd_node *u;
        enum brd_unary utype;
};

struct brd_node_var {
        struct brd_node _node;
        char *id;
};

struct brd_node_num_lit {
        struct brd_node _node;
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

size_t brd_node_type_sizeof(enum brd_node_type t);
#define brd_node_sizeof(n) brd_node_type_sizeof(((struct brd_node *)n)->ntype)

struct brd_node *brd_node_program_new(struct brd_node **stmts, size_t num_stmts);
struct brd_node *brd_node_assign_new(struct brd_node *l, struct brd_node *r);
struct brd_node *brd_node_binop_new(enum brd_binop btype, struct brd_node *l, struct brd_node *r);
struct brd_node *brd_node_unary_new(enum brd_unary utype, struct brd_node *u);
struct brd_node *brd_node_var_new(char *id);
struct brd_node *brd_node_num_lit_new(long double v);
struct brd_node *brd_node_string_lit_new(char *s);
struct brd_node *brd_node_bool_lit_new(int b);
struct brd_node *brd_node_unit_lit_new();

#define brd_node_destroy(n) (((struct brd_node *)n)->destroy((struct brd_node *)n))

int brd_node_is_lvalue(struct brd_node *n);

#endif
