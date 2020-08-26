/* Stuff specific to the general (integer) version of the Reed-Solomon codecs
 *
 * Copyright 2003, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */
#ifndef INT_H_
#define INT_H_

#include <cstdint>

typedef uint8_t data_t;
//typedef unsigned int data_t;

#define MODNN(x)		modnn(rs,x)
#define MM					(rs->mm)
#define NN					(rs->nn)
#define ALPHA_TO		(rs->alpha_to)
#define INDEX_OF		(rs->index_of)
#define GENPOLY			(rs->genpoly)
#define NROOTS			(rs->nroots)
#define FCR					(rs->fcr)
#define PRIM				(rs->prim)
#define IPRIM				(rs->iprim)
#define PAD					(rs->pad)
#define A_0					(NN)

#endif
