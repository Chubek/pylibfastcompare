#include "fastcompare.h"



/*
given a pointer to arrays of unsigned chars by Python, gets lazy hamming label. 1 for dup 0 for non-dup.
*/
void find_hammings_and_mark(char *in[], int outs_labels[], int len_rows, int maxlen) {  
    uint8_t (*nested_array)[len_rows][maxlen] = (uint8_t (*)[len_rows][maxlen])in; 
    
    int i = -1;
    int j = -1;
    int m = -1;
    int cmp;
    char *s1;
    char *s2;

    while (i++ < len_rows) {      
        set_j:
        j = i + 1;

        while (j++ < len_rows) {
            if (outs_labels[j] != -1) continue;

            s1 = (char *)(*nested_array)[i];
            s2 = (char *)(*nested_array)[j];

            m = -1;
            cmp = 0;
            while (m++ < maxlen) {
                if (s1[m] != s2[m]) ++cmp;

                if (cmp > 1) goto set_out;
            };
            
            set_out:
            if (cmp < 2) {
                outs_labels[j] = i;
                goto set_j;
            }
        }
    }
}

TSTNode *new_node(char data) {
    TSTNode* temp = (TSTNode*) malloc(sizeof(TSTNode));
    temp->data = data;
    temp->is_end = 0;
    temp->left = temp->eq = temp->right = NULL;
    return temp;
}

void insert_tst(TSTNode **root, char *word) {
    if (!(*root)) *root = new_node(*word);
    if ((*word) < (*root)->data) insert_tst(&((*root)->left), word);
    else if ((*word) > (*root)->data) insert_tst(&((*root)->right), word);
    else {
        if (*(word + 1)) insert_tst(&(*root)->eq, word + 1);
        else (*root)->is_end = 1;
    }
}
int search_tst(TSTNode *root, char *word) {
    if (!root) return 0;
    if (*word < (root)->data) return search_tst(root->left, word);
    else if (*word > (root)->data) return search_tst(root->right, word);

    else {
        if (*(word + 1) == '\0') return root->is_end;

        return search_tst(root->eq, word + 1);
    }
};
void _traverse_tst(TSTNode *root, char *buffer, int depth) {
    if (root) {
        _traverse_tst(root->left, buffer, depth);

        buffer[depth] = root->data;
        if (root->is_end) {
            buffer[depth + 1] = '\0';
            return;
        }

        _traverse_tst(root->eq, buffer, depth + 1);
        _traverse_tst(root->right, buffer, depth);
    }
}
char *traverse_tst(TSTNode *root) {
    char *buffer = malloc(BUFFER_MAX);
    _traverse_tst(root, buffer, 0);

    return buffer;
}

int find_common_prefix(TSTNode *root, char *word, int *charindex) {
    if (!(*word) || root->is_end) return 1;
    int res = 0;
    char c = word[*charindex];
    char data = root->data;
    char ldata = root->left ? root->left->data : 0;
    char rdata = root->right ? root->right->data  : 0;
    char edata = root->eq ? root->eq->data : 0;
    
    if (c == root->data) {
        ++(*charindex);
        res = find_common_prefix(root->eq, word, charindex);
        if (!res) return 1;    
    }
    else if (c < root->data) {
        if (!root->left) {
            return 0;
        } else {
            res = find_common_prefix(root->left, word, charindex);
            if (!res) return 1;
        }
    } 
    else if (c > root->data) {
        if (!root->right) {
            return 0;
        } else {
            res = find_common_prefix(root->right, word, charindex);
            if (!res) return 1;
        }
    }

    return 0;
}

TrieNode *get_node_trie() {
    TrieNode *node = malloc(sizeof(TrieNode));
    if (!node) {
        printf("Error allocating trie node\n");
        exit(1);
    }

    node->is_end = 0;
    for (int i = 0; i < ALPHA_SIZE; ++i) {
        node->children[i] = NULL;
    }

    return node;
}

void insert_trie(TrieNode *root, char *key) {
    size_t len = strlen(key);
    int index = 0;

    TrieNode *crawler = root;
    for (int l = 0; l < len; ++l) {
        index = CHAR_TO_IND(key[l]);
        
        if (!crawler->children[index]) crawler->children[index] = get_node_trie();


        crawler = crawler->children[index];
    }

    crawler->is_end = 1;
}
int count_children_trie(TrieNode *root, int *index) {
    int cnt = 0;
    for (int i = 0; i < ALPHA_SIZE; ++i) {
        if (root->children[i]) {
            cnt++;
            *index = i;
        }
    }

    return cnt;
}
int walk_trie(TrieNode *root) {
    TrieNode *crawler = root;
    int index = 0;
    int size_common = 0;

    while (count_children_trie(crawler, &index) == 1 && !crawler->is_end) {
        crawler = crawler->children[index];
        size_common++;
    }

    return size_common;
    
}
void construct_tries(char *strings[], int num, TrieNode *root) {
    for (int i = 0; i < num; ++i) {
        insert_trie(root, strings[i]);
    }
}
int len_lcp(char *strings[], int num) {
    TrieNode *root = get_node_trie();
    construct_tries(strings, num, root);

    int res = walk_trie(root);
    free(root);

    return res;    
}

