/* Initialize a RS codec
 *
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 *
 * Slightly modified by Jason Milldrum NT7S, 2015 to fit into the Arduino framework
 */

#include <string>
#include <cstdlib>
#include "JTEncode.h"
#include "rs_common.h"

void JTEncode::free_rs_int(void * p)
{
  struct rs *rs = (struct rs *)p;

  free(rs->alpha_to);
  free(rs->index_of);
  free(rs->genpoly);
  free(rs);
}

void * JTEncode::init_rs_int(int symsize, int gfpoly, int fcr, int prim,
	int nroots, int pad)
{
  struct rs *rs;

  int i, j, sr,root,iprim;

  rs = ((struct rs *)0);
  /* Check parameter ranges */
  if(symsize < 0 || symsize > 8*sizeof(data_t)){
    goto done;
  }

  if(fcr < 0 || fcr >= (1<<symsize))
    goto done;
  if(prim <= 0 || prim >= (1<<symsize))
    goto done;
  if(nroots < 0 || nroots >= (1<<symsize))
    goto done; /* Can't have more roots than symbol values! */
  if(pad < 0 || pad >= ((1<<symsize) -1 - nroots))
    goto done; /* Too much padding */

  rs = (struct rs *)calloc(1,sizeof(struct rs));
  if(rs == ((struct rs *)0))
    goto done;

  rs->mm = symsize;
  rs->nn = (1<<symsize)-1;
  rs->pad = pad;

  rs->alpha_to = (data_t *)malloc(sizeof(data_t)*(rs->nn+1));
  if(rs->alpha_to == NULL){
    free(rs);
    rs = ((struct rs *)0);
    goto done;
  }
  rs->index_of = (data_t *)malloc(sizeof(data_t)*(rs->nn+1));
  if(rs->index_of == NULL){
    free(rs->alpha_to);
    free(rs);
    rs = ((struct rs *)0);
    goto done;
  }

  /* Generate Galois field lookup tables */
  rs->index_of[0] = A_0; /* log(zero) = -inf */
  rs->alpha_to[A_0] = 0; /* alpha**-inf = 0 */
  sr = 1;
  for(i=0;i<rs->nn;i++){
    rs->index_of[sr] = i;
    rs->alpha_to[i] = sr;
    sr <<= 1;
    if(sr & (1<<symsize))
      sr ^= gfpoly;
    sr &= rs->nn;
  }
  if(sr != 1){
    /* field generator polynomial is not primitive! */
    free(rs->alpha_to);
    free(rs->index_of);
    free(rs);
    rs = ((struct rs *)0);
    goto done;
  }

  /* Form RS code generator polynomial from its roots */
  rs->genpoly = (data_t *)malloc(sizeof(data_t)*(nroots+1));
  if(rs->genpoly == NULL){
    free(rs->alpha_to);
    free(rs->index_of);
    free(rs);
    rs = ((struct rs *)0);
    goto done;
  }
  rs->fcr = fcr;
  rs->prim = prim;
  rs->nroots = nroots;

  /* Find prim-th root of 1, used in decoding */
  for(iprim=1;(iprim % prim) != 0;iprim += rs->nn)
    ;
  rs->iprim = iprim / prim;

  rs->genpoly[0] = 1;
  for (i = 0,root=fcr*prim; i < nroots; i++,root += prim) {
    rs->genpoly[i+1] = 1;

    /* Multiply rs->genpoly[] by  @**(root + x) */
    for (j = i; j > 0; j--){
      if (rs->genpoly[j] != 0)
	rs->genpoly[j] = rs->genpoly[j-1] ^ rs->alpha_to[modnn(rs,rs->index_of[rs->genpoly[j]] + root)];
      else
	rs->genpoly[j] = rs->genpoly[j-1];
    }
    /* rs->genpoly[0] can never be zero */
    rs->genpoly[0] = rs->alpha_to[modnn(rs,rs->index_of[rs->genpoly[0]] + root)];
  }
  /* convert rs->genpoly[] to index form for quicker encoding */
  for (i = 0; i <= nroots; i++)
    rs->genpoly[i] = rs->index_of[rs->genpoly[i]];
 done:;

  return rs;
}
