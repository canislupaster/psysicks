#include <cglm/cglm.h>
#include <math.h>

#include "cglm/mat2.h"
#include "cglm/mat3.h"
#include "cglm/vec3.h"
#include "fpsutil.h"
#include "graphics.h"

//finds if point intersects triangle's projection from the origin
int projective_intersects(vec3 v1, vec3 v2, vec3 v3, vec3 point) {
}

object convex_obj(object* obj) {
	object convex = object_new();

	vector_cpy(&obj->vertices, &convex.vertices);
	vector_cpy(&obj->elements, &convex.elements);

	//find centroid of vertices
	vec3 center = GLM_VEC3_ZERO_INIT;

	vector_iterator iter = vector_iterate(&obj->vertices);
	while (vector_next(&iter)) {
		vertex* vert = iter.x;
		glm_vec3_muladds(center, 1.0/obj->vertices.length, vert->pos); //just be safe so it doesnt overflow?
	}

	//project each triangle to the center and remove inner vertices pairwise
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
		while (vector_next(&iter2)) {
			if (iter2.i < iter.i && iter2.i-1 >= iter.i-3)
				continue;
			
			vec3 tripos;
			glm_mat3_mulv(trimat, point, tripos);

			if (tripos[0] == 0) return 1; //at origin
			glm_vec3_divs(tripos, tripos[0], tripos);

			return tripos[1] > tripos[2] || tripos[1] < tripos[2] || tripos[2] > 1 || tripos[2] < 0;
		}
	}
}

//trace edges to nearest point
//char convex_ray
