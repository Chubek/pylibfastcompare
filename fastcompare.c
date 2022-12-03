#include "fastcompare.h"

const uint8_t lookup_num_diffs[257] = {
    0, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 
    1, 2, 2, 2, 1, 2, 2, 2, 2, 3, 3, 3, 
    2, 3, 3, 3, 2, 3, 3, 3, 1, 2, 2, 2, 
    2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 
    1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 
    2, 3, 3, 3, 1, 2, 2, 2, 2, 3, 3, 3, 
    2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 
    3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 
    2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 
    3, 4, 4, 4, 2, 3, 3, 3, 3, 4, 4, 4, 
    3, 4, 4, 4, 3, 4, 4, 4, 1, 2, 2, 2, 
    2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 
    2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4, 
    3, 4, 4, 4, 2, 3, 3, 3, 3, 4, 4, 4, 
    3, 4, 4, 4, 3, 4, 4, 4, 2, 3, 3, 3, 
    3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 
    1, 2, 2, 2, 2, 3, 3, 3, 2, 3, 3, 3, 
    2, 3, 3, 3, 2, 3, 3, 3, 3, 4, 4, 4, 
    3, 4, 4, 4, 3, 4, 4, 4, 2, 3, 3, 3, 
    3, 4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 
    2, 3, 3, 3, 3, 4, 4, 4, 3, 4, 4, 4,
    3, 4, 4, 4, 1
};


TSTNode *new_node(uint8_t data) {
    TSTNode* temp = (TSTNode*) malloc(sizeof(TSTNode));
    temp->data = data;
    temp->is_end = 0;
    temp->left = temp->eq = temp->right = NULL;
    return temp;
}

void insert_tst(TSTNode **root, uint8_t *word) {
    if (!(*root)) *root = new_node(*word);
    if ((*word) < (*root)->data) insert_tst(&((*root)->left), word);
    else if ((*word) > (*root)->data) insert_tst(&((*root)->right), word);
    else {
        if (*(word + 1)) insert_tst(&(*root)->eq, word + 1);
        else (*root)->is_end = 1;
    }
}
int search_tst(TSTNode *root, uint8_t *word) {
    if (!root) return 0;
    if (*word < (root)->data) return search_tst(root->left, word);
    else if (*word > (root)->data) return search_tst(root->right, word);

    else {
        if (*(word + 1) == '\0') return root->is_end;

        return search_tst(root->eq, word + 1);
    }
};
void _traverse_tst(TSTNode *root, uint8_t *buffer, int depth) {
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
uint8_t *traverse_tst(TSTNode *root) {
    uint8_t *buffer = malloc(BUFFER_MAX);
    _traverse_tst(root, buffer, 0);

    return buffer;
}

int find_common_prefix(TSTNode *root, uint8_t *word, int *uint8_tindex) {
    if (!(*word) || root->is_end) return 1;
    int res = 0;
    uint8_t c = word[*uint8_tindex];
    
    if (c == root->data) {
        ++(*uint8_tindex);
        res = find_common_prefix(root->eq, word, uint8_tindex);
        if (!res) return 1;    
    }
    else if (c < root->data) {
        if (!root->left) {
            return 0;
        } else {
            res = find_common_prefix(root->left, word, uint8_tindex);
            if (!res) return 1;
        }
    } 
    else if (c > root->data) {
        if (!root->right) {
            return 0;
        } else {
            res = find_common_prefix(root->right, word, uint8_tindex);
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

void insert_trie(TrieNode *root, uint8_t *key, int stlen) {
    int len = stlen;
    int index = 0;
    uint8_t c;
    TrieNode *crawler = root;
    for (int l = 0; l < len; ++l) {
        c = key[l];
        if (c == '\0') {
            crawler->is_end = 1;
            break;
        }
        index = CHAR_TO_IND(c);      
        if (!crawler->children[index]) crawler->children[index] = get_node_trie();
        crawler = crawler->children[index];
    }    
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
    int size_common = 0;
    int index;


    while (count_children_trie(crawler, &index) == 1 && !crawler->is_end) {
        crawler = crawler->children[index];
        size_common++;
    }
    return size_common;
    
}
void construct_tries(uint8_t strings[2][4000], int num, TrieNode *root, int stlen) {
    uint8_t *s;

    for (int i = 0; i < num; ++i) {
        s = strings[i];
        insert_trie(root, s, stlen);
    }
}
int len_lcp(uint8_t strings[2][4000], int num, int out[], int stlen) {
    TrieNode *root = get_node_trie();
    construct_tries(strings, num, root, stlen);

    int res = walk_trie(root);
    free(root);

    out[0] = res;
    return res;    
}


void encode_gatacca(chartype_t in[SIZE_CHARS], outtype_t out[SIZE_OUT]) {
    __m256i all_threes = _mm256_set1_epi8(3);
    __m256i sl_nums = _mm256_setr_epi64x(0, 2, 4, 6);
    __m256i loaded_array = _mm256_loadu_si256((__m256i *)&in[0]);

    __m256i loaded_array_shifted_right =  _mm256_srai_epi16(loaded_array, 1);
;   __m256i loaded_array_anded = _mm256_and_si256(loaded_array_shifted_right, all_threes);
    __m256i loaded_array_shifted_left = _mm256_sllv_epi64(loaded_array_anded, sl_nums);

  
    outtype_t v[SIZE_OUT];

    _mm256_storeu_si256((__m256i*)&v[0], loaded_array_shifted_left);

    for (int i = 0; i < SIZE_OUT; i++) {
        out[i] = v[i];
    }

}

void reduce_integer_or_op(outtype_t in, outtype_t *reducer) {
    *reducer = in | *reducer;
}

void pack_32_bytes_in_64_bits(chartype_t in[SIZE_CHARS], outtype_t result[1]) {
    outtype_t out[SIZE_OUT];
    memset(out, 0, SIZE_OUT * sizeof(chartype_t));

    encode_gatacca(in, out);

    result[0] = out[0] | out[1] | out[2] | out[3];;
}

 

/*
given a pointer to arrays of unsigned uint8_ts by Python, gets lazy hamming label. 1 for dup 0 for non-dup.
*/
int get_hamming_integers(hamtype_t a[SIZE_HAM], hamtype_t b[SIZE_HAM]) {  
    __m256i a_loadedu = _mm256_loadu_si256((__m256i*)&a[0]);
    __m256i b_loadedu = _mm256_loadu_si256((__m256i*)&b[0]);

    __m256i xord = _mm256_xor_si256(a_loadedu, b_loadedu);

    uint8_t v[32];
    int diff = 0;
    uint8_t c;

    _mm256_storeu_epi8((__m256i*)&v[0], xord);

    for (size_t i = 0; i < 32; i++) {
        c = v[i];
        diff += lookup_num_diffs[c];

        if (diff >= 2) break;
    }

    return diff;

}


void get_hamming_cluster(hamtype_t in_cluster[][SIZE_HAM], size_t len_rows, int *out) {
  //  hamtype_t (*in_2d)[len_rows][len_cols] = (hamtype_t (*)[len_rows][len_cols])in;

    hamtype_t a[len_cols];
    hamtype_t b[len_cols];
    int64_t i = -1;
    int64_t j = 0;
    int out_i = 0;
    int out_j = 0;
    int diff = 0;

    while (i++ < (int64_t)len_rows) {
        set_j:
        diff = 0;
        out_i = out[i];
        if (out_i != -1) continue;
        j = i + 1;

        while (j < (int64_t)len_rows) {
            out_j = out[j];
            if (out_j != -1) { j++; continue; }
  
            memcpy(a, in_2d[i], len_cols * sizeof(hamtype_t));
            memcpy(b, in_2d[j], len_cols * sizeof(hamtype_t));

            diff = get_hamming_integers(a, b, len_cols); 
            if (diff < 2) {
                out[j] = i;
                goto set_j;                
            }
            j++;
        }
    }

}


uint32_t hash32(uint64_t x) {
  uint32_t low = (uint32_t)x;
  uint32_t high = (uint32_t)(x >> 32);
  return (uint32_t)((A * low + B * high + C) >> 32);
}

size_t hash_tuple_to_index(uint32_t x, size_t len) {
    uint32_t hash_x = hash32(x);
    uint32_t hash_len = hash32(len);

    return (size_t)((((hash_x ^ PYHASH_X) * PYHASH_MULT) ^ PYHASH_X) * (PYHASH_MULT + 1))
}

hm_s *new_hashmap() {
    hm_s *hm = malloc(sizeof(hm_s));

    if (!hm) {
        printf("Error allocating hashmap on heap\n");
        exit(139);
    }

    hm->vec_vec = NULL;
    hm->n = 0;
    hm->next_round = 0;

    return hm;

}
hmvalue_s *new_hmv(size_t len_seq) {
    hmvalue_s *hmv = malloc(sizeof(hm_s));

    if (!hmv) {
        printf("Error allocating hashmap on heap\n");
        exit(139);
    }

    hmv->arr = NULL;
    hmv->len_seq = len_seq;
    hmv->n = 0;
    hmv->next_round = 0;

    return hmv;
}

void resize_hashmap(hm_s *self) {
    self->next_round = ROUNDUP_32(self->n);

    if (self->next_round > self->n) {
        self->vec_vec = (hmvalue_s**)realloc(self->vec_vec, self->next_round * sizeof(hmvalue_s*));

        if (!self->vec_vec) {
            printf("Error reallocating hashmap value vector on heap to %lu\n", self->next_round);
            exit(139);
        }
    }
}

void resize_hashmap_vec(hmvalue_s *self) {
    self->next_round = ROUNDUP_32(self->n);

    if (self->next_round > self->n) {
        self->arr = (uint64_t*)realloc(self->arr, self->next_round * sizeof(uint64_t) * self->len_seq);

        if (!self->arr) {
            printf("Error reallocating sequence holder array on heap to %lu\n", self->next_round);
            exit(139);
        }
    }
}

void free_hashmap_vec(hmvalue_s *self) {
    free(self);
}

void free_hashmap(hm_s *self) {
    for (size_t i = 0; i < self->n; ++i) free_hashmap_vec(self->vec_vec[i]);

    free(self);
}

void insert_into_hashmap(hm_s *self, uint64_t key, uint64_t *value, size_t len_seq) {
    size_t vec_index = hash_tuple_to_index(key, len_seq);

    if (self->n < vec_index) {
        self->n = vec_index;
        resize_hashmap(self);

        self->vec_vec[vec_index - 1] = new_hmv();
    }

    self->vec_vec[vec_index - 1]->n += 1;
    resize_hashmap_vec(self->vec_vec[vec_index - 1]);
    for (int i = 0; i < len_seq; i++) self->vec_vec[vec_index - 1]->arr[i + self->vec_vec[vec_index - 1]->n - 1] = value[i];
}

hmvalue_s *get_hashmap_value(hm_s *self, uint64_t key, size_t len_seq) {
    size_t vec_index = hash_tuple_to_index(key, len_seq);

    return self->vec_vec[vec_index - 1];
}