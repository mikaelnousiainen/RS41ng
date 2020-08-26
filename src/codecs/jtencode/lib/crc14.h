/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Thu Dec  6 17:52:01 2018
 * by pycrc v0.9.1, https://pycrc.org
 * using the configuration:
 *  - Width         = 14
 *  - Poly          = 0x2757
 *  - XorIn         = Undefined
 *  - ReflectIn     = Undefined
 *  - XorOut        = Undefined
 *  - ReflectOut    = Undefined
 *  - Algorithm     = bit-by-bit
 *
 * This file defines the functions crc_init(), crc_update() and crc_finalize().
 *
 * The crc_init() function returns the inital \c crc value and must be called
 * before the first call to crc_update().
 * Similarly, the crc_finalize() function must be called after the last call
 * to crc_update(), before the \c crc is being used.
 * is being used.
 *
 * The crc_update() function can be called any number of times (including zero
 * times) in between the crc_init() and crc_finalize() calls.
 *
 * This pseudo-code shows an example usage of the API:
 * \code{.c}
 * crc_cfg_t cfg = {
 *     0,      // reflect_in
 *     0,      // xor_in
 *     0,      // reflect_out
 *     0,      // xor_out
 * };
 * crc_t crc;
 * unsigned char data[MAX_DATA_LEN];
 * size_t data_len;
 *
 * crc = crc_init(&cfg);
 * while ((data_len = read_data(data, MAX_DATA_LEN)) > 0) {
 *     crc = crc_update(&cfg, crc, data, data_len);
 * }
 * crc = crc_finalize(&cfg, crc);
 * \endcode
 */
#ifndef CRC14_H
#define CRC14_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * The definition of the used algorithm.
 *
 * This is not used anywhere in the generated code, but it may be used by the
 * application code to call algorithm-specific code, if desired.
 */
#define CRC_ALGO_BIT_BY_BIT 1


/**
 * The type of the CRC values.
 *
 * This type must be big enough to contain at least 14 bits.
 */
typedef uint_fast16_t crc_t;


/**
 * The configuration type of the CRC algorithm.
 */
typedef struct {
    bool reflect_in;            /*!< Whether the input shall be reflected or not */
    crc_t xor_in;               /*!< The initial value of the register */
    bool reflect_out;           /*!< Whether the output shall be reflected or not */
    crc_t xor_out;              /*!< The value which shall be XOR-ed to the final CRC value */
} crc_cfg_t;


/**
 * Calculate the initial crc value.
 *
 * \param[in] cfg  A pointer to an initialised crc_cfg_t structure.
 * \return     The initial crc value.
 */
crc_t crc_init(const crc_cfg_t *cfg);


/**
 * Update the crc value with new data.
 *
 * \param[in] crc      The current crc value.
 * \param[in] cfg      A pointer to an initialised crc_cfg_t structure.
 * \param[in] data     Pointer to a buffer of \a data_len bytes.
 * \param[in] data_len Number of bytes in the \a data buffer.
 * \return             The updated crc value.
 */
crc_t crc_update(const crc_cfg_t *cfg, crc_t crc, const void *data, size_t data_len);


/**
 * Calculate the final crc value.
 *
 * \param[in] cfg  A pointer to an initialised crc_cfg_t structure.
 * \param[in] crc  The current crc value.
 * \return     The final crc value.
 */
crc_t crc_finalize(const crc_cfg_t *cfg, crc_t crc);


#ifdef __cplusplus
}           /* closing brace for extern "C" */
#endif

#endif      /* CRC14_H */
