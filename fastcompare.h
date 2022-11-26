#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define BUFFER_MAX 6000

typedef struct TSTNode {
    char data;
    struct TSTNode *left, *right, *eq;
    unsigned char is_end;
} TSTNode;

TSTNode *new_node(char data);

int search_tst(TSTNode *root, char *word);
void insert_tst(TSTNode **root, char *word);
void _traverse_tst(TSTNode *root, char *buffer, int depth);
char *traverse_tst(TSTNode *root);

#define ALPHA_SIZE 4
#define CHAR_TO_IND(c) (((c == 'A') ? 0 : (c == 'C' ? 1 : (c == 'G' ? 2 : (c == 'T' || c == 'U' ? 3 : -1)))));

typedef struct TrieNode {
    struct TrieNode *children[ALPHA_SIZE];
    int is_end;
} TrieNode;

TrieNode *get_node_trie();
void insert_trie(TrieNode *root, char *key);
int count_children_trie(TrieNode *root, int *index);
int walk_trie(TrieNode *root);
void construct_tries(char **strings, int num, TrieNode *root);
int len_lcp(char **strings, int num);


int find_common_prefix(TSTNode *root, char *str, int *charindex);

void find_hammings_and_mark(char *in[], int outs_labels[], int len_rows, int maxlen);
