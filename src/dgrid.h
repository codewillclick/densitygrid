#ifndef dgrid_h
#define dgrid_h

#ifndef DEL_NONE
#define DEL_NONE 0x1
#define DEL_STRUCT 0x2
#define DEL_DATA 0x4
#define DEL_KEYS 0x8
#define DEL_VALS 0x16
#endif

typedef struct {
	union {
		int v[2];
		struct {
			int x;
			int y;
		};
	};
	void* val;
} valpair;

typedef struct {
	union {
		int v[4];
		struct {
			int x;
			int y;
			int w;
			int h;
		};
	};
} dgrid_frame;

dgrid_frame* init_dgrid_frame(dgrid_frame* f, int x, int y, int w, int h) {
	f->x = x;
	f->y = y;
	f->w = w;
	f->h = h;
	return f;
}

struct _dgrid;
struct _dgrid_config;

typedef void(*func_coord_translation)(int*,struct _dgrid*);
typedef void(*func_value_interact)(void*,void*);
typedef valpair*(*func_value_to_pairs)(void*,int*,struct _dgrid_config*);

typedef void*(*func_0)();
typedef void(*del_func)(void*,int);


// { dgrid
typedef struct _dgrid {
	dgrid_frame frame;
	func_0 new_cell;
	del_func del_cell;
	int del_cell_flags;
	void** arr;
	void*** rows;
} dgrid;

dgrid* init_dgrid(dgrid* dg, dgrid_frame* frame,
		func_0 new_cell, del_func del_cell, int del_cell_flags) {
	dg->frame = *frame;
	dg->new_cell = new_cell;
	dg->del_cell = del_cell;
	dg->del_cell_flags = del_cell_flags;
	
	// NOTE: Right now, this is a void**, which is inefficient given a plausibly
	//   sparse or concentrated set of input coordinates.  May amend at a later
	//   time.
	int size = frame->w * frame->h;
	dg->arr = (void**)malloc(sizeof(void*)*size);
	while (--size >= 0)
		dg->arr[size] = 0;
	
	dg->rows = (void***)malloc(sizeof(void**) * frame->h);
	int i;
	for (i=0; i < frame->h; ++i)
		dg->rows[i] = &dg->arr[i * frame->w];
	
	return dg;
}
dgrid* init_dgrid2(dgrid* dg, int x, int y, int w, int h,
		func_0 new_cell, del_func del_cell, int del_cell_flags) {
	dgrid_frame f;
	return init_dgrid(dg, init_dgrid_frame(&f,x,y,w,h),
		new_cell,del_cell,del_cell_flags);
}

void del_dgrid(dgrid* dg, int del_flags) {
	int size = dg->frame.w * dg->frame.h;
	if (dg->arr) {
		int i;
		for (i=0; i < size; ++i) {
			if (dg->arr[i])
				dg->del_cell(dg->arr[i],dg->del_cell_flags);
		}
		free(dg->arr);
	}
	if (dg->rows)
		free(dg->rows);
	if (del_flags & DEL_STRUCT)
		free(dg);
}
// }


// This exists as a stand-in variable for 'free'.
void DGRID_FREE(void* p, int del_flags) { free(p); }

typedef struct _dgrid_config {
	func_coord_translation translate;
	func_value_interact    vapply;
	func_value_to_pairs    tovpairs;
	del_func free_vpair_val;
	int free_vpair_val_flags;
	del_func free_values;
	int free_values_flags;
} dgrid_config;

dgrid_config* init_dgrid_config(
		dgrid_config*          conf,
		func_value_to_pairs    tovpairs,
		func_coord_translation translate,
		func_value_interact    vapply) {
	conf->tovpairs = tovpairs;
	conf->translate = translate;
	conf->vapply = vapply;
	conf->free_vpair_val = 0;
	conf->free_vpair_val_flags = 0;
	conf->free_values = 0;
	conf->free_values_flags = 0;
	return conf;
}
dgrid_config* dgrid_config_free(
		dgrid_config* conf,
		del_func free_vpair_val,
		int free_vpair_val_flags,
		del_func free_values,
		int free_values_flags) {
	conf->free_vpair_val = free_vpair_val;
	conf->free_vpair_val_flags = free_vpair_val_flags;
	conf->free_values = free_values;
	conf->free_values_flags = free_values_flags;
	return conf;
}

#include <stdio.h>

//#define DEBUG

void dgrid_process_values(dgrid* dg, dgrid_config* conf, void* values) {
	if (!values)
		return;
	
	valpair* pairs;
	void* todel_pairs;
	void* dg_cell;
	int paircount=0;
	// ASSUME: values is zero-ended.
	//while (*values) {
#ifdef DEBUG
		printf("value: %x\n",values); fflush(stdout);
#endif
		pairs = conf->tovpairs(values,&paircount,conf);
		todel_pairs = pairs;
		// ASSUME: another zero-ended array.
		while (paircount-- > 0) {
#ifdef DEBUG
			printf("point C.%i: %i %i\n",
				paircount,(*pairs).x,(*pairs).y); fflush(stdout);
#endif
			conf->translate((int*)(*pairs).v,dg); // .v is a v[2], so pointer or not?
			
			// Apply perhaps to specific dgrid grid object, already extracted with
			// coordinates.
			int x = (*pairs).x;
			int y = (*pairs).y;
			int i = y*dg->frame.w + x;
			if (!dg->arr[i])
				dg->arr[i] = dg->new_cell();
			dg_cell = dg->arr[i];
			if (!( // check if in bounds
				x < dg->frame.x || y < dg->frame.y ||
				x >= dg->frame.x+dg->frame.w || y >= dg->frame.y+dg->frame.h))
			conf->vapply(dg_cell,pairs->val);
			
			if (pairs->val && conf->free_vpair_val)
				conf->free_vpair_val(pairs->val, conf->free_vpair_val_flags);
#ifdef DEBUG
			printf("  end C\n");
#endif
			pairs++;
		}
#ifdef DEBUG
		printf("point D: %x %x\n",
			conf->free_values,
			conf->free_values_flags); fflush(stdout);
#endif
		if (values && conf->free_values)
			conf->free_values(values, conf->free_values_flags);
#ifdef DEBUG
		printf("point D.5\n"); fflush(stdout);
#endif
		if (todel_pairs)
			free(todel_pairs);
#ifdef DEBUG
		printf("point E\n"); fflush(stdout);
#endif
	//	values++;
	//}
#ifdef DEBUG
	printf("point F\n"); fflush(stdout);
#endif
	// ASSUME: values is to be freed.  Or don't.  Or do.
	//free(todel);
	//printf("point G\n"); fflush(stdout);
}


#endif

