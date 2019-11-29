// #include <stdio.h>
// #include <stdlib.h>

/*******************************
 * Define structure data types *
 *******************************/

/*
 * This is the data that every node carries.
 * You can define anything here.
 * This data structure is the one that will be
 * used by comparison function to determine
 * the order of the elements inside the
 * Fibonacci Heap (which mean that any posible
 * key should be hear).
 *
 * You have to allocate this on your own but
 * some of the functions here will free it
 * for you (ie. when deleting a node).
 */
struct fib_heap_data {
	int key;
};

/* this is the data type for every node */
struct fib_heap_node {
	struct fib_heap_data *data;
	unsigned int degree;
	char marked;
	struct fib_heap_node *parent;
	struct fib_heap_node *child;
	struct fib_heap_node *left;
	struct fib_heap_node *right;
};

/* the main heap data type */
struct fib_heap {
	struct fib_heap_node *the_one;
	struct fib_heap_node **cons_array;
	int (*compr)(struct fib_heap_data *, struct fib_heap_data *);
	unsigned int total_nodes;
	unsigned int max_nodes;
};

/* intel assembly */
#define LOG2(X) ({ \
		unsigned int _i = 0; \
		__asm__("bsr %1, %0" : "=r" (_i) : "r" ((X))); \
		_i; })

#define CHECK_MALLOC(X) ({ \
		if((X)==0) { \
			exit(); \
		}; })

#define CHECK_INPUT(X, M) ({ \
		if(!(X)) {\
			exit(); \
		}; })

static void
dblink_list_concat(struct fib_heap_node *a, struct fib_heap_node *b)
{
	struct fib_heap_node *temp;
	CHECK_INPUT(a != 0, "dblink_list_concat: a==0");
	CHECK_INPUT(b != 0, "dblink_list_concat: b==0");

	temp = a->right;
	a->right = b->right;
	b->right->left = a;
	b->right = temp;
	temp->left = b;
}

struct fib_heap *
fibheap_init(unsigned int max,
	int (*compr)(struct fib_heap_data *, struct fib_heap_data *))
{
	struct fib_heap *H = (struct fib_heap *)malloc(sizeof(struct fib_heap));
	CHECK_MALLOC(H);
	CHECK_INPUT(max > 0, "fibheap_init: max has to be positive");

	H->the_one = 0;
	H->cons_array = (struct fib_heap_node **)
		malloc((LOG2(max)+2) *	sizeof(struct fib_heap_node *));
	CHECK_MALLOC(H->cons_array);
	H->total_nodes = 0U;
	H->max_nodes = max;

	H->compr = compr;

	return H;
}

struct fib_heap_node *
fibheap_insert(struct fib_heap *H, struct fib_heap_data *d)
{
	struct fib_heap_node *node = (struct fib_heap_node *)
		malloc(sizeof(struct fib_heap_node));
	CHECK_MALLOC(node);
	CHECK_INPUT(H != 0, "fibheap_insert: H==0");
	CHECK_INPUT(H->total_nodes<H->max_nodes, "fibheap_insert: Fibonacci Heap Overflow");

	node->data = d;
	node->degree = 0U;
	node->marked = 0;
	node->parent = 0;
	node->child = 0;

	if((H->the_one) == 0) {
		node->left = node;
		node->right = node;
		H->the_one = node;
	} else {
		node->left = H->the_one->left;
		node->right = H->the_one;
		H->the_one->left->right = node;
		H->the_one->left = node;
		if(H->compr(H->the_one->data, node->data) > 0)
			H->the_one = node;
	}
	H->total_nodes++;

	return node;
}

struct fib_heap_data *
fibheap_read(struct fib_heap *H)
{
	CHECK_INPUT(H != 0, "fibheap_read: H==0");
	return (H->the_one->data);
}

struct fib_heap *
fibheap_union(struct fib_heap *a, struct fib_heap *b)
{
	struct fib_heap *temp;
	CHECK_INPUT(a != 0, "fibheap_union: a==0");
	CHECK_INPUT(b != 0, "fibheap_union: b==0");
	CHECK_INPUT(a != b, "fibheap_union: a==b");
	CHECK_INPUT(a->compr == b->compr,
		"fibheap_union: a and b have different compare functions");

	if(b->max_nodes > a->max_nodes) {
		temp = a;
		a = b;
		b = temp;
	}

	if((a->the_one) == 0) {
		a->the_one = b->the_one;
		a->total_nodes = b->total_nodes;
		free(b->cons_array);
		free(b);
	} else if((b->the_one) == 0) {
		free(b->cons_array);
		free(b);
	} else {
		dblink_list_concat(a->the_one, b->the_one);
		if((a->compr)(a->the_one->data, b->the_one->data) > 0)
			a->the_one = b->the_one;
		a->total_nodes += b->total_nodes;
		free(b->cons_array);
		free(b);
	}
	return a;
}

static void
fibheap_link(struct fib_heap_node *y, struct fib_heap_node *x)
{
	struct fib_heap_node *temp;

	y->left->right = y->right;
	y->right->left = y->left;

	y->parent = x;
	if(x->child == 0) {
		x->child = y;
		y->left = y;
		y->right = y;
	} else {
		temp = x->child->right;
		x->child->right = y;
		y->left = x->child;
		temp->left = y;
		y->right = temp;
	}

	(x->degree)++;
	y->marked = 0;
}

static void
fibheap_consolidate(struct fib_heap *H)
{
	unsigned int D;
	unsigned int i, d;
	struct fib_heap_node *x, *w, *y, *temp, *new_one;

	if(H->total_nodes == 0U) {
		free(H->the_one);
		H->the_one = 0;
		return;
	}

	D = LOG2(H->total_nodes);
	for(i=0U;i<=D;i++)
		H->cons_array[i] = 0;

	w = H->the_one->right;
	new_one = w;
	while(w != H->the_one) {
		x = w;
		d = x->degree;
		while((d<=D) && (H->cons_array[d]!=0)) {
			y = H->cons_array[d];
			if((H->compr)(x->data, y->data) > 0) {
				temp = x;
				x = y;
				y = temp;
			}
			if(w == y)
				w = w->left;
			fibheap_link(y, x);
			H->cons_array[d] = 0;
			d++;
		}
		H->cons_array[d] = x;
		if((H->compr)(x->data, new_one->data) <= 0)
			new_one = x;
		w = w->right;
	}

	H->the_one->left->right = H->the_one->right;
	H->the_one->right->left = H->the_one->left;
	free(H->the_one);
	H->the_one = new_one;
}

struct fib_heap_data *
fibheap_extract(struct fib_heap *H)
{
	struct fib_heap_data *ret;
	unsigned int i;
	CHECK_INPUT(H != 0, "fibheap_extract: H==0");

	if(H->the_one == 0)
		return 0;

	ret = H->the_one->data;

	for(i=0U;i<H->the_one->degree;i++) {
		H->the_one->child->parent = 0;
		H->the_one->child = H->the_one->child->right;
	}

	if(H->the_one->child != 0)
		dblink_list_concat(H->the_one, H->the_one->child);
	(H->total_nodes)--;
	fibheap_consolidate(H);

	return ret;
}

static void
fibheap_cut(struct fib_heap *H,	struct fib_heap_node *x,struct fib_heap_node *y)
{
	(y->degree)--;
	if(y->degree == 0U) {
		y->child = 0;
	} else {
		y->child = x->right;
		x->left->right = x->right;
		x->right->left = x->left;
	}

	x->right = x;
	x->left = x;
	x->parent = 0;
	x->marked = 0;
	dblink_list_concat(H->the_one, x);
}

/*
 * Yes,  I know this function can easily be re-written
 * as non-recursive. But I think gcc with optimizations
 * can handle a simple tail-recursive function like this.
 * This way is easier and cleaner!
 */
static void
fibheap_casc_cut(struct fib_heap *H, struct fib_heap_node *y)
{
	struct fib_heap_node *z = y->parent;

	if(z == 0)
		return;

	if(!(y->marked)) {
		y->marked = 1;
		return;
	}

	fibheap_cut(H, y, z);
	return fibheap_casc_cut(H, z);
}

void
fibheap_increase(struct fib_heap *H, struct fib_heap_node *node)
{
	struct fib_heap_node *y;
	CHECK_INPUT(H != 0, "fibheap_increase: H==0");
	CHECK_INPUT(node != 0, "fibheap_increase: node==0");

	y = node->parent;
	if(y!=0 && ((H->compr)(y->data, node->data) > 0)) {
		fibheap_cut(H, node, y);
		fibheap_casc_cut(H, y);
	}
	if((H->compr)(H->the_one->data, node->data) > 0)
		H->the_one = node;
}

void
fibheap_decrease(struct fib_heap *H, struct fib_heap_node *node)
{
	unsigned int i;
	struct fib_heap_node *y;
	CHECK_INPUT(H != 0, "fibheap_decrease: H==0");
	CHECK_INPUT(node != 0, "fibheap_decrease: node==0");

	/* concat node's childs with the root list */
	for(i=0U;i<node->degree;i++) {
		node->child->parent = 0;
		node->child = node->child->right;
	}
	if(node->child != 0)
		dblink_list_concat(H->the_one, node->child);
	node->child = 0;
	node->degree = 0U;

	/* cut the node */
	y = node->parent;
	if(y != 0) {
		fibheap_cut(H, node, y);
		fibheap_casc_cut(H, y);
	} else if(H->the_one == node) {	/* find then new the_one */
		y = node->right;
		while(y != node) {
			if((H->compr)(node->data, y->data) > 0)
				H->the_one = y;
			y = y->right;
		}
	}
}

void
fibheap_delete(struct fib_heap *H, struct fib_heap_node *node)
{
	struct fib_heap_node *y;
	struct fib_heap_data *d;
	CHECK_INPUT(H != 0, "fibheap_delete: H==0");
	CHECK_INPUT(node != 0, "fibheap_delete: node==0");

	/* increase priority / cut the node */
	y = node->parent;
	if(y != 0) {
		fibheap_cut(H, node, y);
		fibheap_casc_cut(H, y);
	}

	/* set this node as the highest priority node and extract it */
	H->the_one = node;
	d = fibheap_extract(H);
	free(d);
}

static void
fibheap_destroy_rec(struct fib_heap_node *node)
{
	struct fib_heap_node *start = node;

	if(node == 0)
		return;

	do {
		fibheap_destroy_rec(node->child);
		node = node->right;
		free(node->left->data);
		free(node->left);
	} while(node != start);
}

void
fibheap_destroy(struct fib_heap *H)
{
	CHECK_INPUT(H != 0, "fibheap_destroy: H==0");

	fibheap_destroy_rec(H->the_one);
	free(H->cons_array);
	free(H);
}
