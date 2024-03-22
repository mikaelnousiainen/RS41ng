#ifndef __CATS_LDPC_MATRICES_H
#define __CATS_LDPC_MATRICES_H

typedef struct cats_ldpc_code {
    // Code length in bits (data+parity)
    int code_length_bits;
    // Data length in bits
    int data_length_bits;
    int punctured_bits;
    int bf_working_len;
    size_t circulant_size;
    size_t matrix_len;
    const uint64_t* matrix;
} cats_ldpc_code_t;

extern cats_ldpc_code_t tc128;
extern cats_ldpc_code_t tc256;
extern cats_ldpc_code_t tc512;
extern cats_ldpc_code_t tm2048;

extern const uint64_t tc128_matrix[];
extern const uint64_t tc256_matrix[];
extern const uint64_t tc512_matrix[];
extern const uint64_t tm2048_matrix[];

#endif
