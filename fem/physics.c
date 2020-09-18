#include <math.h>

#include "field.c"
#include "vector.h"

typedef struct {
	unsigned i[3]; //aligned tris
	unsigned point[3]; //connected edge in ascending order
} adj_tri_t;

typedef struct {
	vector_t vertices; //deduped vertices
	map_t vertices_loc; //vec3 -> verticies
	vector_t adjacent; //adj_tri_t

	char volume_init; //volume has been computed
	float volume; //total without intersecting wedges

	vector_t vert_volume; //non-wedge projected vertex volume
	vector_t vert_volume_nonconvex; //vertex volume with intersecting wedges from adjacent tris
	vector_t opposite;

	object_t* obj;
} physics_obj_t;

physics_obj_t physics_obj(object_t* obj) {

}

//probably doesnt work lmao
object_t convex_obj(object_t* obj) {
	object_t convex = object_new();

	vector_cpy(&obj->vertices, &convex.vertices);
	vector_cpy(&obj->elements, &convex.elements);

	//find centroid of vertices
	vec3 center = GLM_VEC3_ZERO_INIT;

	vector_iterator iter = vector_iterate(&obj->vertices);
	while (vector_next(&iter)) {
		vertex* vert = iter.x;
		glm_vec3_muladds(center, 1.0/obj->vertices.length, vert->pos); //just be safe so it doesnt overflow?
	}

	//project each triangle to the center and remove inner triangles pairwise
	iter = vector_iterate(&obj->vertices);
	while (vector_skip(&iter, 3)) {
		vertex* verts = iter.x;

		float* v1 = verts[0].pos;
		float* v2 = verts[1].pos;
		float* v3 = verts[2].pos;

		//create inverse matrix
		mat3 trimat;
		glm_vec3_copy(v1, trimat[0]);
		glm_vec3_copy(v2, trimat[1]);
		glm_vec3_copy(v3, trimat[2]);

		glm_vec3_sub(trimat[1], v1, trimat[1]);
		glm_vec3_sub(trimat[2], v1, trimat[2]);

		glm_mat3_inv(trimat, trimat);

		vector_iterator iter2 = vector_iterate(&obj->vertices);
		while (vector_next(&iter)) {
			if (iter2.i == iter.i)
				continue;
			
			float* point = iter.x;

			vec3 tripos;
			glm_mat3_mulv(trimat, point, tripos);

			if (tripos[0] > 0) {
				glm_vec3_divs(tripos, tripos[0], tripos);
				if (tripos[1] > tripos[2] || tripos[1] < tripos[2] || tripos[2] > 1 || tripos[2] < 0) continue;
			}

			if (iter.i >= iter2.i) iter.i -= 1;
			iter2.i -= 1;

			vector_remove(&obj->vertices, iter2.i);
		}
	}

	return convex;
}
