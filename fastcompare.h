#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include <threads.h>
#include <stdatomic.h>

#define ALPHA_SIZE 4
#define CHAR_TO_IND(c) (((c == 'A') ? 0 : (c == 'C' ? 1 : (c == 'G' ? 2 : (c == 'T' || c == 'U' ? 3 : -1)))));
#define BUFFER_MAX 6000
#define SIZE_CHARS 32
#define SIZE_OUT 4
#define SIZE_HAM 4

#define PYHASH_X 0x345678
#define PYHASH_MULT 1000003

#define A 0x02502ef84dbecade
#define B 0x27112ee84dfecafa
#define C 0x191022f84ddecadb



#define ROUNDUP_32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))

#define kvec_t(type) struct { size_t n, m; type *a; }
#define kv_init(v) ((v).n = (v).m = 0, (v).a = 0)
#define kv_destroy(v) free((v).a)
#define kv_A(v, i) ((v).a[(i)])
#define kv_pop(v) ((v).a[--(v).n])
#define kv_size(v) ((v).n)
#define kv_max(v) ((v).m)

#define kv_resize(type, v, s) do { \
		if ((v).m < (s)) { \
			(v).m = (s); \
			kv_roundup32((v).m); \
			(v).a = (type*)realloc((v).a, sizeof(type) * (v).m); \
		} \
	} while (0)

#define kv_copy(type, v1, v0) do {							\
		if ((v1).m < (v0).n) kv_resize(type, v1, (v0).n);	\
		(v1).n = (v0).n;									\
		memcpy((v1).a, (v0).a, sizeof(type) * (v0).n);		\
	} while (0)												\

#define kv_push(type, v, x) do {									\
		if ((v).n == (v).m) {										\
			(v).m = (v).m? (v).m<<1 : 2;							\
			(v).a = (type*)realloc((v).a, sizeof(type) * (v).m);	\
		}															\
		(v).a[(v).n++] = (x);										\
	} while (0)

#define kv_a(type, v, i) ((v).m <= (size_t)(i)?						\
						  ((v).m = (v).n = (i) + 1, kv_roundup32((v).m), \
						   (v).a = (type*)realloc((v).a, sizeof(type) * (v).m), 0) \
						  : (v).n <= (size_t)(i)? (v).n = (i)			\
						  : 0), (v).a[(i)]

#define kv_reverse(type, v, start) do { \
		if ((v).m > 0 && (v).n > (start)) { \
			size_t __i, __end = (v).n - (start); \
			type *__a = (v).a + (start); \
			for (__i = 0; __i < __end>>1; ++__i) { \
				type __t = __a[__end - 1 - __i]; \
				__a[__end - 1 - __i] = __a[__i]; __a[__i] = __t; \
			} \
		} \
	} while (0)



typedef uint8_t chartype_t;
typedef uint64_t outtype_t;
typedef uint64_t hamtype_t;

typedef struct TSTNode {
    uint8_t data;
    struct TSTNode *left, *right, *eq;
    int is_end;
} TSTNode;
TSTNode *new_node(uint8_t data);
int search_tst(TSTNode *root, uint8_t *word);
void insert_tst(TSTNode **root, uint8_t *word);
void _traverse_tst(TSTNode *root, uint8_t *buffer, int depth);
uint8_t *traverse_tst(TSTNode *root);
typedef struct TrieNode {
    struct TrieNode *children[ALPHA_SIZE];
    int is_end;
} TrieNode;

TrieNode *get_node_trie();
void insert_trie(TrieNode *root, uint8_t *key, int stn);
int count_children_trie(TrieNode *root, int *index);
int walk_trie(TrieNode *root);
void construct_tries(uint8_t strings[2][4000], int num, TrieNode *root, int stn);
int len_lcp(uint8_t strings[2][4000], int num, int out[], int stn);
int find_common_prefix(TSTNode *root, uint8_t *str, int *uint8_tindex);

int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM]);
void get_hamming_cluster(hamtype_t in_cluster[][SIZE_HAM], size_t len_rows, int *out);
void reduce_integer_or_op(outtype_t in, outtype_t *reducer);
void encode_gatacca(chartype_t in[SIZE_CHARS], outtype_t out[SIZE_OUT]);
void pack_32_bytes_in_64_bits(chartype_t in[SIZE_CHARS], outtype_t result[1]);

uint32_t hash32(uint64_t x);
size_t hash_tuple_to_index(uint32_t x, size_t len);
int next_round_32(int *in);

#define SIZE_MAX 256

typedef struct HashMapValue {
    uint64_t *arr;
    size_t len_seq;
    size_t n;
    size_t next_round;
} hmvalue_s;

typedef struct HashMapSt {
    hmvalue_s** vec_vec;
    size_t n;
    size_t next_round;
} hm_s;

hm_s *new_hashmap();
hmvalue_s *new_hmv();
void resize_hashmap(hm_s *self);
void resize_hashmap_vec(hmvalue_s *self);
void free_hashmap_vec(hmvalue_s *self);
void free_hashmap(hm_s *self);
void insert_into_hashmap(hm_s *self, uint64_t key, uint64_t *value, size_t len_seq);
hmvalue_s *get_hashmap_value(hm_s *self, uint64_t key, size_t len_seq);