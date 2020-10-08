#include <math.h>

#include "util.h"
#include "field.c"
#include "mat.h"
#include "vector.h"

#define PHYSICS_NORMAL_ANGLES 10

typedef struct {
	unsigned* edges; //ends with zero, +1
} physics_adj_t;

typedef struct {
	vector_t vertices; //deduped vertices
	map_t vertices_loc; //vec3 -> vertices
	vector_t adjacent; //adjacent edges
	vector_t tris;

	float volume; //total vol

	vector_t vert_volume; //vertex contribution to volume

	vector_t normals[PHYSICS_NORMAL_ANGLES*PHYSICS_NORMAL_ANGLES]; //normal of hyperplane -> all triangles with normal within 180 degrees for raycasting
	vector_t opposite; //intersections from normal
} physics_obj_t;

//rudimentary analysis
physics_obj_t physics_obj(vector_t vertices, vector_t elements, unsigned pos_stride) {
	physics_obj_t pobj;
	pobj.vertices = vector_new(sizeof(vec3));
	pobj.tris = vector_new(sizeof(unsigned)*3);

	pobj.vertices_loc = map_new();
	map_configure_uint96_key(&pobj.vertices_loc, sizeof(unsigned));

	pobj.adjacent = vector_new(sizeof(physics_adj_t));

	for (unsigned i=0; i<PHYSICS_NORMAL_ANGLES*PHYSICS_NORMAL_ANGLES; i++) {
		pobj.normals[i] = vector_new(sizeof(unsigned));
	}

	//tag vertices
	for (unsigned i=2; i<elements.length; i+=3) {
		unsigned v[3];
		float* verts[3];

		//deref
		for (char i2=0; i2<3; i2++) {
			unsigned* elem = (unsigned*)vector_get(&elements, i-i2);
			verts[i2] = (float*)((char*)vector_get(&vertices, *elem) + pos_stride);
			map_insert_result res = map_insertcpy_noexist(&pobj.vertices_loc, verts[i2], &pobj.vertices.length);
			if (!res.exists) {
				vector_pushcpy(&pobj.vertices, verts[i2]);
			}

			v[i2] = *(unsigned*)res.val;
		}

		unsigned tri = pobj.tris.length;
		vector_pushcpy(&pobj.tris, v);

		//insert adjacent
		unsigned v_adj[3] = {0};

		for (char i2=0; i2<3; i2++) {
			char exists;
			physics_adj_t* adj = vector_setget(&pobj.adjacent, v[i2], &exists);

			//hopefully unrolled lmao
			for (char i3 = 0; i3 < 3; i3++) {
				if (i3 == i2) continue;
				v_adj[i3 > i2 ? i3 - 1 : i3] = v[i3] + 1;
			}

			if (!exists) {
				adj->edges = heapcpy(sizeof(unsigned) * 3, v_adj);
			} else {
				unsigned* x = adj->edges;
				do {
					x++;

					if (*x == v_adj[0]) {
						v_adj[0] = 0;
						if (v_adj[1] == 0) break;
					}

					if (*x == v_adj[1]) {
						v_adj[1] = 0;
						if (v_adj[0] == 0) break;
					}
				} while (*x != 0);

				if (v_adj[0] == 0 && v_adj[1] == 0) continue;
				unsigned count = x - adj->edges;
				if (v_adj[0]) count++;
				if (v_adj[1]) count++;

				adj->edges = resize(adj->edges, sizeof(unsigned) * count);
				adj->edges[count - 1] = 0;

				if (v_adj[0]) {
					adj->edges[count - 2] = v_adj[0];
					count--;
				}

				if (v_adj[1]) adj->edges[count - 2] = v_adj[1];
			}
		}

		//what the fuck did i just write
		//anyways
		vec3 v12, v13;
		vec3sub(verts[1], verts[0], v12);
		vec3sub(verts[2], verts[0], v13);

		vec3 normal;
		vec3cross(v12, v13, normal);
		vec3normalize(normal);

		float y_scale = sqrtf(1-normal[1]*normal[1]);
		normal[0] /= y_scale;
		normal[2] /= y_scale;

		float y = asinf(normal[1]) / M_PI_2;
		float x = asinf(normal[2]) / M_PI_2;

		//insert into hemisphere
		for (int off=-PHYSICS_NORMAL_ANGLES/2 - 1; off<=PHYSICS_NORMAL_ANGLES/2 + 1; off++) {
			vector_pushcpy(&pobj.normals[((int)(y*PHYSICS_NORMAL_ANGLES + x) + off) % PHYSICS_NORMAL_ANGLES], &tri);
		}


	}

	return pobj;
}

//vector_t convex_obj(object_t* obj) {
//	vector_t convex = vector_new(sizeof(vec3));
//
//	//find centroid of vertices
//	vec3 center = GLM_VEC3_ZERO_INIT;
//
//	vector_iterator iter = vector_iterate(&obj->vertices);
//	while (vector_next(&iter)) {
//		vertex* vert = iter.x;
//		glm_vec3_muladds(center, 1.0/obj->vertices.length, vert->pos); //just be safe so it doesnt overflow?
//	}
//
//	//project each triangle to the center and remove inner triangles pairwise
//	iter = vector_iterate(&obj->vertices);
//	while (vector_skip(&iter, 3)) { before you uncomment this needs to be done for every three points
//		vertex* verts = iter.x;
//
//		float* v1 = verts[0].pos;
//		float* v2 = verts[1].pos;
//		float* v3 = verts[2].pos;
//
//		//create inverse matrix
//		mat3 trimat;
//		glm_vec3_copy(v1, trimat[0]);
//		glm_vec3_copy(v2, trimat[1]);
//		glm_vec3_copy(v3, trimat[2]);
//
//		glm_vec3_sub(trimat[1], v1, trimat[1]);
//		glm_vec3_sub(trimat[2], v1, trimat[2]);
//
//		glm_mat3_inv(trimat, trimat);
//
//		vector_iterator iter2 = vector_iterate(&obj->vertices);
//		while (vector_next(&iter)) {
//			if (iter2.i == iter.i)
//				continue;
//
//			float* point = iter.x;
//
//			vec3 tripos;
//			glm_mat3_mulv(trimat, point, tripos);
//
//			if (tripos[0] > 0) {
//				glm_vec3_divs(tripos, tripos[0], tripos);
//				if (tripos[1] > tripos[2] || tripos[1] < tripos[2] || tripos[2] > 1 || tripos[2] < 0) continue;
//			}
//
//			if (iter.i >= iter2.i) iter.i -= 1;
//			iter2.i -= 1;
//
//			vector_remove(&obj->vertices, iter2.i);
//		}
//	}
//
//	return convex;
//}