#include <stdint.h>

#include "vector.h"
#include "cfg.h"
#include "util.h"
#include "endian.h"

typedef enum {
	json_dbl = 0x01,
	json_str = 0x02,
	json_doc = 0x03,
	json_array_begin = 0x04,
	json_barray = 0x05,
	json_null = 0x0A,
	json_array_end,
	json_eof,
	json_done
} json_ty;

const char* json_ty_names[] = {
		[json_dbl] = "double",
		[json_str] = "str",
		[json_doc] = "object",
		[json_array_begin] = "array",
		[json_array_end] = "end array",
		[json_barray] = "byte array",
		[json_null] = "null",
		[json_eof] = "eof",
		[json_done] = "done"
};

typedef struct {
	FILE* src;

	vector_cap_t stack;

	char* cur;
	char* err;
} json_parser;

typedef struct {
	json_ty ty;
	char* name;

	union {
		double dbl;
		void* data;
	};
} json_obj;

json_parser json_new(FILE* src) {
	json_parser jparse = {.stack=vector_alloc(vector_new(sizeof(json_ty)), 0), .cur="\0", .src=src, .err=NULL};
	return jparse;
}

char* json_skip_string(json_parser* jparse) {
	char* start = jparse->cur;

	while (*jparse->cur != '"') {
		if (*jparse->cur == 0) {
			jparse->err = "expected end quote";
			return start;
		} else if (*jparse->cur == '\\') {
			memcpy(start+1, start, jparse->cur - start);
			start++;

			jparse->cur++;

			//escape chars
			if (*jparse->cur == 'n') {
				*jparse->cur = '\n';
			} else if (*jparse->cur == 't') {
				*jparse->cur = '\t';
			}
		}

		jparse->cur++;
	}

	*jparse->cur = 0;
	jparse->cur++;

	return start;
}

//sensible bullshit
json_obj json_next(json_parser* jparse) {
	skip_ws(&jparse->cur);

	json_ty* ty = vector_get(&jparse->stack.vec, jparse->stack.vec.length-1);

	if (skip_char(&jparse->cur, '}')) {
		if (*ty != json_doc) jparse->err = "no matching brace";
		if (jparse->stack.vec.length == 1) return (json_obj){.ty = json_done};
		vector_pop(&jparse->stack.vec);

	} else if (skip_char(&jparse->cur, ']')) {
		if (*ty != json_array_begin) jparse->err = "no matching bracket";
		if (jparse->stack.vec.length <= 1) jparse->err = "expected eof, not bracket";

		vector_pop(&jparse->stack.vec);

		return (json_obj){.ty=json_array_end};
	}

	if (!*jparse->cur) {
		size_t sz = 0;
		jparse->cur = NULL;
		if (getline(&jparse->cur, &sz, jparse->src)==-1) {
			jparse->err = "unexpected eof";
			return (json_obj){.ty=json_eof};
		}

		return json_next(jparse);
	}

	skip_ws(&jparse->cur);

	json_obj obj;

	if (ty && *ty == json_doc) {
		if (!skip_char(&jparse->cur, '"')) jparse->err = "unnamed object";

		obj.name = json_skip_string(jparse);
		obj.name = heapcpy(jparse->cur - obj.name, obj.name);

		skip_ws(&jparse->cur);
		if (!skip_char(&jparse->cur, ':')) jparse->err = "expected colon";
	} else if (ty && *ty == json_array_begin) {
		skip_char(&jparse->cur, ',');
		obj.name = NULL;
	}

	skip_ws(&jparse->cur);

	if (skip_char(&jparse->cur, '{')) {
		obj.ty = json_doc;
		*(char*)vector_push(&jparse->stack.vec) = json_doc;

	} else if (skip_char(&jparse->cur, '"')) {
		obj.ty = json_str;
		obj.data = json_skip_string(jparse);
		obj.name = heapcpy(jparse->cur - obj.name, obj.name);

	} else if (skip_name(&jparse->cur, "null")) {
		obj.ty = json_null;

	} else if (skip_char(&jparse->cur, '[')) {
		obj.ty = json_array_begin;
		*(char*)vector_push(&jparse->stack.vec) = json_array_begin;

	} else {
		obj.ty = json_dbl;
		obj.dbl = strtod(jparse->cur, &jparse->cur);
	}

	return obj;
}

int main(int argc, char** argv) {
	json_parser jparse = json_new(stdin);

	json_obj obj;
	do {
		obj = json_next(&jparse);
		if (jparse.err) {
			printf("%s", jparse.err);
			return 1;
		}


	} while (obj.ty != json_done);
}