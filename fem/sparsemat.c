#include "vector.h"
#include "hashtable.h"

typedef struct {
	vector_t elems;
	map_t rows;
} sparsemat_t;

//conjugate gradient / gmres, todo
