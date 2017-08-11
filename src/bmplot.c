
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "dgrid.h"
#include "bmper.h"

// { Junk
char* _tokenize_pbuf[1024]; // single-thread only, I guess
char** tokenize(char* s, char delim) {
	// TODO: End by finding zero, rather than with len.
	int len = strlen(s);
	char* tr = (char*)malloc(sizeof(char)*(len+1));
	tr[len] = 0;
	char** pr = _tokenize_pbuf;
	int prior = 0;
	int tcount = 0;
	int i;
	for (i=0; i < len; ++i) {
		if (s[i] == delim) {
			tr[i] = 0;
			pr[tcount++] = &tr[prior];
			prior = i+1;
		}
		else
			tr[i] = s[i];
	}
	if (prior < len)
		pr[tcount++] = &tr[prior];
	pr = (char**)malloc(sizeof(char*)*(tcount+1));
	pr[tcount] = 0;
	for (i=0; i < tcount; ++i)
		pr[i] = _tokenize_pbuf[i];
	return pr;
}
void free_tokenized(char** tokens, int free_source) {
	if (!tokens || !tokens[0])
		return;
	if (free_source)
		free(tokens[0]);
	free(tokens);
}

typedef struct {
	union {
		struct {
			int x;
			int y;
			int z;
			int q;
		};
		struct {
			int a[2];
			int b[2];
		};
		int v[4];
	};
} intv4;

intv4 def_intv4_1(int* r) {
	intv4 v;
	v.x=r[0]; v.y=r[1]; v.z=r[2]; v.q=r[3];
	return v;
}
intv4 def_intv4_2(int* a, int* b) {
	intv4 v;
	v.x=a[0]; v.y=a[1]; v.z=b[0]; v.q=b[1];
	return v;
}
intv4 def_intv4_4(int x, int y, int z, int q) {
	intv4 v;
	v.x=x; v.y=y; v.z=z; v.q=q;
	return v;
}

// NOTE: minitable handles no memory deallocation.
typedef uint32_t(*func_minihash)(void*);
typedef struct {
	uint32_t key;
	void* val;
} keyval;
typedef struct {
	int size;
	keyval* r;
	func_minihash hashfunc;
} minitable;
minitable* init_minitable(minitable* t, func_minihash f, int size) {
	t->size = size;
	t->hashfunc = f;
	t->r = (keyval*)malloc(sizeof(keyval)*size);
	int i;
	for (i=0; i < size; ++i) {
		t->r[i].key = 0;
		t->r[i].val = 0;
	}
}
void del_minitable(minitable* t, int del_flags) {
	free(t->r);
	if (del_flags & DEL_STRUCT)
		free(t);
}
void del_minitable_vals(minitable* t, del_func func, int del_flags) {
	int i;
	for (i=0; i < t->size; ++i)
		if (t->r[i].val)
			func(&t->r[i].val,del_flags);
}
	keyval* _minitable_get_rec(keyval* p, uint32_t h, keyval* first, keyval* last) {
		// NOTE: Hmm... this should not use recursion.  Too much copied into the stack.
		//printf("_rec: %x, %i, %x, %x\n",p,h,first,last); fflush(stdout);
		if (!p->val || p->key == h)
			return p;
		if (p == last)
			return _minitable_get_rec(first,h,first,last);
		return _minitable_get_rec(++p,h,first,last);
	}
void* minitable_get(minitable* t, void* key) {
	uint32_t h = t->hashfunc(key);
	keyval* p = _minitable_get_rec(&t->r[h % t->size],h,t->r,&t->r[t->size-1]);
	return p->val;
}
void minitable_add(minitable* t, void* key, void* val) {
	uint32_t h = t->hashfunc(key);
	int x = h % t->size;
	keyval* p = _minitable_get_rec(&t->r[x],h,t->r,&t->r[t->size-1]);
	p->key = h;
	p->val = val;
}
void minitable_add_many(minitable* t, void** pairs) {
	// ASSUME: pairs is zero-ended.
	void* k;
	void* v;
	int i=0;
	while (*pairs) {
		k = pairs[0];
		v = pairs[1];
		//if (i < 20) { printf("%i: %x, %x\n",i,k,v); fflush(stdout); }
		minitable_add(t,k,v);
		pairs++; pairs++;
		++i;
	}
}
uint32_t minihash_int(void* v) { return *(int32_t*)v; }
uint32_t minihash_float(void* v) { return *(int32_t*)v; }
uint32_t minihash_str(void* v) {
	struct { union { uint64_t q; struct { uint32_t hi; uint32_t lo; }; }; } h;
	char* s = (char*)v;
	h.q = 0;
	while (*s) {
		h.q <<= 7;
		h.q += (uint64_t)(*s & 0x7f);
		if (h.hi)
			h.lo += h.hi;
		s++;
	}
	return h.lo;
}
// }




// { dgrid callbacks.
void* new_cell_int() {
	int* p = (int*)malloc(sizeof(int*));
	*p = 0;
	return (void*)p;
}
void* new_cell_float() {
	float* p = (float*)malloc(sizeof(float*));
	*p = 0;
	return (void*)p;
}
void del_cell_free(void* cell, int del_flags) {
	free(cell);
}

valpair* pairs_int(void* vals, int* count, dgrid_config* conf) {
	int* r = (int*)vals;
	valpair* p = (valpair*)malloc(sizeof(valpair));
	(*p).x = (int)r[0];
	(*p).y = (int)r[1];
	(*p).val = malloc(sizeof(int*));
	*(int*)(*p).val = 1;
	*count = 1;
	return p;
}

void trans_offset_xy(int* c, dgrid* dg) {
	c[0] -= dg->frame.x;
	c[1] -= dg->frame.y;
}

void apply_int_int_add(void* cell, void* val) {
	int* p = (int*)cell;
	*p += *(int*)val;
}
void apply_int_float_add(void* cell, void* val) {
	int* p = (int*)cell;
	*p += (int)*(float*)val;
}
void apply_float_float_add(void* cell, void* val) {
	float* p = (float*)cell;
	*p += *(float*)val;
}
void apply_float_int_add(void* cell, void* val) {
	float* p = (float*)cell;
	*p += (float)*(int*)val;
}
// }

// { Declare a few things
// }

// { Bitmap functions
typedef struct addendum_i {
	int max;
	int min;
} addendum_i;
typedef void(*func_render)(dgrid*,void*,void*);

void render_bmp_black(dgrid* dg, void* bstate, void* params);

#define params1(a) ((void**)a)[0]
#define params2(a,i) (((void**)&(((void**)a)[1]))[i])
void iter_callback_bmp_white(int x, int y, void* val, void* params) {
	//printf("iter_callback_bmp_white: %i %i %x %x\n",x,y,val,params);
	BMP24File* bmp = (BMP24File*)((void**)params)[0];
	bmp->pr[y][x] = pix24_int(0xffffff);
}
void iter_callback_bmp_gray256i(int x, int y, void* val, void* params) {
	//printf("iter_callback_bmp_white: %i %i %x %x\n",x,y,val,params);
	int v = *(int*)val;
	v &= 0xff;
	BMP24File* bmp = (BMP24File*)params1(params);
	bmp->pr[y][x] = pix24_rgb(v,v,v);
}
void iter_callback_bmp_gray256i_norm(int x, int y, void* val, void* params) {
	//printf("iter_callback_bmp_white: %i %i %x %x\n",x,y,val,params);
	addendum_i* ad = (addendum_i*)params2(params,0);
	int v = *(int*)val;
	v = (v-ad->min) * (ad->max) / 255 + 1;
	v &= 0xff;
	BMP24File* bmp = (BMP24File*)params1(params);
	bmp->pr[y][x] = pix24_rgb(v,v,v);
}

// Not bitmap really, but putting this here for now.
void iter_callback_addendum_i(int x, int y, void* val, void* params) {
	addendum_i* p = (addendum_i*)params;
	int v = *(int*)val;
	p->min = (p->min < v ? p->min : v);
	p->max = (p->max < v ? p->max : v);
}
// }


static minitable ftable;
void initialize_ftable() {
	init_minitable(&ftable,minihash_str,27);
	int NOID;
	void* p[] = {
		"n.int",   new_cell_int,
		"n.float", new_cell_float,
		
		"del", del_cell_free,
		
		"p.int", pairs_int,
		
		"t.xy", trans_offset_xy,
		
		"ii", apply_int_int_add,
		"if", apply_int_float_add,
		"fi", apply_float_int_add,
		"ff", apply_float_float_add,
		
		"bmp.black", render_bmp_black,
		"c.white",   iter_callback_bmp_white,
		"c.gray256i", iter_callback_bmp_gray256i,
		"c.gray256i.n", iter_callback_bmp_gray256i_norm,
		0
	};
	minitable_add_many(&ftable,p);
}
void delete_ftable() {
	del_minitable(&ftable,0);
}


// { Plot state
typedef struct bmplot_state {
	int skip_lines;
	intv4 frame;
	int cols[2];
	char cell_key[16];
	char del_key[16];
	char pairs_key[16];
	char trans_key[16];
	char apply_key[16];
	char render_key[16];
	char color_key[16];
	char render_name[257];
} bmplot_state;

bmplot_state* init_bmplot_state(bmplot_state* s) {
	s->skip_lines = 0;
	return s;
}
char* str_bmplot_state(bmplot_state* state, char* buf) {
	sprintf(buf,"{skip_lines:%i, cols:[%i,%i], frame:[%i,%i,%i,%i]}",
		state->skip_lines,
		state->cols[0],state->cols[1],
		state->frame.x, state->frame.y, state->frame.z, state->frame.q);
	return buf;
}
// }


// { Render functions

static int _render_index = 0;
void render_bmp_black(dgrid* dg, void* bstate, void* param) {
	bmplot_state* state = (bmplot_state*)bstate;
	
	printf("render_bmp_black: %x, %x\n",dg,bstate); fflush(stdout);
	
	BMP24File bmp;
	init_BMP24File(&bmp,dg->frame.w,dg->frame.h);
	BMP24File_fill(&bmp,pix24_int(0x0));
	
	// Draw that durn thang!!!
	func_dgrid_iterator_callback color = minitable_get(&ftable,state->color_key);
	void** args[2] = {(void*)&bmp,(void*)param};
	dgrid_iterate_values(dg, color, args);
	
	FILE* f = fopen(state->render_name,"wb");
	BMP24File_write(&bmp,f);
	fclose(f);
	
	del_BMP24File(&bmp,0);
}
// }


// { Evaluation methods
void configure_dgrid(dgrid* dg, dgrid_config* conf, bmplot_state* state) {
	func_0              new_cell=0;
	del_func            del_cell=0;
	func_value_to_pairs    pairs=0;
	func_coord_translation trans=0;
	func_value_interact    apply=0;
	
	// Logic with ftable, setting funcs.
	new_cell = minitable_get(&ftable,state->cell_key);
	del_cell = minitable_get(&ftable,state->del_key);
	pairs    = minitable_get(&ftable,state->pairs_key);
	trans    = minitable_get(&ftable,state->trans_key);
	apply    = minitable_get(&ftable,state->apply_key);
	
	intv4 f = state->frame;
	init_dgrid2(dg, f.x,f.y,f.z,f.q, new_cell,del_cell,0);
	
	init_dgrid_config(conf, pairs,trans,apply);
	dgrid_config_free(conf, DGRID_FREE,0, 0,0);
}

//	char _1s[] = {' ',0};
//	char* str1(char* s) { _1s[0] = s[0]; return _1s; }
void evaluate_file(char* path, void* bstate) {
	bmplot_state* state = (bmplot_state*)bstate;
	printf("evaluate_file: %s, %x\n",path,bstate); fflush(stdout);
	char cbuf[512];
	printf("state: %s\n",str_bmplot_state(state,cbuf));
	FILE* f = fopen(path,"r");
	
	dgrid dg;
	dgrid_config conf;
	configure_dgrid(&dg,&conf,state);
	
	int skip = state->skip_lines;
	int count;
	char** tokens;
	int vals[2];
	char line[512000];
	//char blarg[12888];
	while (count=fscanf(f,"%[^\n]\n",line) == 1) {
		if (skip-- > 0)
			continue;
		//strcpy(blarg,line);
		//int len = strlen(blarg);
		tokens = tokenize(line,',');
		int j;
		//printf("pretok: %s\n",blarg);
		//printf("tokens: ");
		//for (j=0; j < len; ++j)
		//	printf("%i:%s,",j,str1(&tokens[0][j]));
		//printf("\n");
		fflush(stdout);
		// TODO: Do something about this warning.
		// WARNING: This assumes integers, rather than offering options
		//   parallel to pipeline selections.
		//printf("deese: %s, %s",tokens[state->cols[0]],tokens[state->cols[1]]);
		vals[0] = atoi(tokens[state->cols[0]]);
		vals[1] = atoi(tokens[state->cols[1]]);
		// And the processing, at last...
		//printf("Processing columns %i,%i: %i,%i\n",
		//	state->cols[0],state->cols[1],vals[0],vals[1]); fflush(stdout);
		dgrid_process_values(&dg,&conf,vals);
		free_tokenized(tokens,0);
	}
	
	// Special processing...
	addendum_i ad;
	ad.max = 0; ad.min = 0;
	dgrid_iterate_values(&dg,iter_callback_addendum_i,&ad);
	
	// Now, finally, fiiinally, we render.
	func_render render = minitable_get(&ftable,state->render_key);
	if (render) {
		void* vp[1] = {(void*)&ad};
		render(&dg,state,vp);
	}
	del_dgrid(&dg,0);
	
	fclose(f);
}
// }


typedef void(*func_command)(void*,void*);


// { Commands
void command_plot(void* a, void* bstate) {
	bmplot_state* state = (bmplot_state*)bstate;
	char* line = (char*)a;
	
	printf("command_plot\n"); fflush(stdout);
	
	char rangebuf[48]; rangebuf[0]=0;
	char filebuf[512]; filebuf[0]=0;
	char colbuf[128]; colbuf[0]=0;
	char types[16]; types[0]=0;
	
	sscanf(line,"%*s%*[ \t]%s%*[ \t]%s%*[ \t]%s",rangebuf,filebuf,colbuf);
	printf("these:\n  %s\n  %s\n  %s\n",rangebuf,filebuf,colbuf); fflush(stdout);
	
	int x,y,w,h;
	sscanf(rangebuf,"[%i,%i,%i,%i]",&x,&y,&w,&h);
	state->frame = def_intv4_4(x,y,w,h);
	
	int c1,c2;
	sscanf(colbuf,"%i,%i",&c1,&c2);
	state->cols[0]=c1; state->cols[1]=c2;
	printf("state cols: %i %i\n",state->cols[0],state->cols[1]); fflush(stdout);
	
	// Only a file, for now, I guess.
	evaluate_file(filebuf,state);
	
}

void command_skip(void* a, void* bstate) {
	bmplot_state* state = (bmplot_state*)bstate;
	char* s = (char*)a;
	int skip = 1;
	int res = sscanf(s,"%*s%*[ \t]%i",&skip);
	state->skip_lines += skip;
}

void command_pipeline(void* a, void* bstate) {
	bmplot_state* state = (bmplot_state*)bstate;
	char* line = (char*)a;
	
	printf("command_pipeline\n"); fflush(stdout);
	
	char newbuf[16]; newbuf[0]=0;
	char delbuf[16]; delbuf[0]=0;
	char pairsbuf[16]; pairsbuf[0]=0;
	char transbuf[16]; transbuf[0]=0;
	char applybuf[16]; applybuf[0]=0;
	
	sscanf(line,"%*s%*[ \t]%s%*[ \t]%s%*[ \t]%s%*[ \t]%s%*[ \t]%s",
		newbuf,delbuf,pairsbuf,transbuf,applybuf);
	printf("these:\n  %s\n  %s\n  %s\n  %s\n  %s\n",
		newbuf,delbuf,pairsbuf,transbuf,applybuf); fflush(stdout);
	
	strcpy(state->cell_key,newbuf);
	strcpy(state->del_key,delbuf);
	strcpy(state->pairs_key,pairsbuf);
	strcpy(state->trans_key,transbuf);
	strcpy(state->apply_key,applybuf);
}

void command_render(void* a, void* bstate) {
	bmplot_state* state = (bmplot_state*)bstate;
	char* line = (char*)a;
	
	printf("command_render\n"); fflush(stdout);
	
	char rendernamebuf[257]; rendernamebuf[0]=0;
	char renderbuf[16]; renderbuf[0]=0;
	char colorbuf[16]; colorbuf[0]=0;
	
	sscanf(line,"%*s%*[ \t]%s%*[ \t]%s%*[ \t]%s%*[ \t]%s",
		rendernamebuf,renderbuf,colorbuf);
	printf("these:\n  %s\n  %s\n  %s\n",rendernamebuf,renderbuf,colorbuf);
	
	strcpy(state->render_name,rendernamebuf);
	strcpy(state->render_key,renderbuf);
	strcpy(state->color_key,colorbuf);
}
// }


// { Command Behavior
typedef struct {
	char name[16];
	func_command command;
} command_pair;

command_pair COMMAND_PAIRS[] = {
	{"plot",command_plot},
	{"skip",command_skip},
	{"pipeline",command_pipeline},
	{"render",command_render}
};

void run_command(char* name, char* full_string, void* state) {
	printf("run_command, sizes: %i, %i\n",sizeof(COMMAND_PAIRS),sizeof(command_pair)); fflush(stdout);
	int count = sizeof(COMMAND_PAIRS) / sizeof(command_pair);
	int i;
	for (i=0; i < count; ++i) {
		printf("run_command[%i]: %s vs %s\n",i,name,COMMAND_PAIRS[i].name); fflush(stdout);
		if (strcmp(name,COMMAND_PAIRS[i].name) == 0) {
			COMMAND_PAIRS[i].command(full_string,state);
			break;
		}
	}
}
// }


int main(int argc, char** argv) {
	
	// Local initializations.
	initialize_ftable();
	
	FILE* f;
	
	// Read in from stdin or from arg[1].
	if (argc > 1) {
		f = fopen(argv[1],"r");
	}
	else f = stdin;
	
	printf("point A\n"); fflush(stdout);
	//*
	bmplot_state state;
	init_bmplot_state(&state);
	int i=0;
	char buf[2048]; buf[0]=0;
	char buf2[256]; buf2[0]=0;
	while (fscanf(f,"%[^\n]\n",buf) == 1) {
		printf("%i: %s\n",i,buf); fflush(stdout);
		if (strlen(buf) == 0)
			continue;
	printf("point Y.1\n"); fflush(stdout);
		sscanf(buf,"%s",buf2);
	printf("point Y.2: (%s) (%s)\n",buf2,buf); fflush(stdout);
		// Loop through commands until... death by falling bananas?
		run_command(buf2,buf,&state);
		i++;
	printf("point Y.3\n"); fflush(stdout);
	}
	
	printf("point Z\n"); fflush(stdout);
	fclose(f);
	// */
	// Local deallocations.
	delete_ftable();
	
	return 0;
}

