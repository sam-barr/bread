#include "common.h"
#include "ast.h"

/*
 * Destroys the base node
 * Every destroy function should call this
 */
static void
_flt_node_destroy(struct flt_node *n)
{
        free(n);
}

size_t
flt_node_type_sizeof(enum flt_node_type t)
{
        switch(t) {
        case FLT_NODE_BINOP:
                return sizeof(struct flt_node_binop);
        case FLT_NODE_NUM_LIT:
                return sizeof(struct flt_node_num_lit);
        case FLT_NODE_STRING_LIT:
                return sizeof(struct flt_node_string_lit);
        case FLT_NODE_BOOL_LIT:
                return sizeof(struct flt_node_bool_lit);
        case FLT_NODE_UNIT_LIT:
                return sizeof(struct flt_node_unit_lit);
        case FLT_NODE_MAX:
                BARF("Invalid node of type FLT_NODE_MAX");
        }
        return -1;
}

static void
flt_node_binop_destroy(struct flt_node *n)
{
        struct flt_node_binop *b = (struct flt_node_binop *)n;
        flt_node_destroy(b->l);
        flt_node_destroy(b->r);
        _flt_node_destroy(n);
}

void
flt_node_binop_init(struct flt_node **n, enum flt_binop btype, struct flt_node *l, struct flt_node *r)
{
        struct flt_node_binop *nn = malloc(sizeof(*nn));
        nn->_node.ntype = FLT_NODE_BINOP;
        nn->_node.destroy = flt_node_binop_destroy;
        nn->btype = btype;
        nn->l = l;
        nn->r = r;
        *n = (struct flt_node *)nn;
}

void
flt_node_num_lit_init(struct flt_node **n, long double v)
{
        struct flt_node_num_lit *nn = malloc(sizeof(*nn));
        nn->_node.ntype = FLT_NODE_NUM_LIT;
        nn->_node.destroy = _flt_node_destroy;
        nn->v = v;
        *n = (struct flt_node *)nn;
}

static void
flt_node_string_lit_destroy(struct flt_node *n)
{
        free(((struct flt_node_string_lit *)n)->s);
        _flt_node_destroy(n);
}

void
flt_node_string_lit_init(struct flt_node **n, char *s)
{
        struct flt_node_string_lit *nn = malloc(sizeof(*nn));
        nn->_node.ntype = FLT_NODE_STRING_LIT;
        nn->_node.destroy = flt_node_string_lit_destroy;
        nn->s = s;
        *n = (struct flt_node *)nn;
}

void
flt_node_bool_lit_init(struct flt_node **n, int b)
{
        struct flt_node_bool_lit *nn = malloc(sizeof(*nn));
        nn->_node.ntype = FLT_NODE_BOOL_LIT;
        nn->_node.destroy = _flt_node_destroy;
        nn->b = b;
        *n = (struct flt_node *)nn;
}

void
flt_node_unit_lit_init(struct flt_node **n)
{
        struct flt_node_unit_lit *nn = malloc(sizeof(*nn));
        nn->_node.ntype = FLT_NODE_UNIT_LIT;
        nn->_node.destroy = _flt_node_destroy;
        *n = (struct flt_node *)nn;
}
