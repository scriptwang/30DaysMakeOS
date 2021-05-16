/* Simple bitmaps.
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "flags.h"
#include "hard-reg-set.h"
#include "basic-block.h"

/* Bitmap manipulation routines.  */

/* Allocate a simple bitmap of N_ELMS bits.  */

sbitmap
sbitmap_alloc (n_elms)
     unsigned int n_elms;
{
  unsigned int bytes, size, amt;
  sbitmap bmap;

  size = SBITMAP_SET_SIZE (n_elms);
  bytes = size * sizeof (SBITMAP_ELT_TYPE);
  amt = (sizeof (struct simple_bitmap_def)
	 + bytes - sizeof (SBITMAP_ELT_TYPE));
  bmap = (sbitmap) xmalloc (amt);
  bmap->n_bits = n_elms;
  bmap->size = size;
  bmap->bytes = bytes;
  return bmap;
}

/* Allocate a vector of N_VECS bitmaps of N_ELMS bits.  */

sbitmap *
sbitmap_vector_alloc (n_vecs, n_elms)
     unsigned int n_vecs, n_elms;
{
  unsigned int i, bytes, offset, elm_bytes, size, amt, vector_bytes;
  sbitmap *bitmap_vector;

  size = SBITMAP_SET_SIZE (n_elms);
  bytes = size * sizeof (SBITMAP_ELT_TYPE);
  elm_bytes = (sizeof (struct simple_bitmap_def)
	       + bytes - sizeof (SBITMAP_ELT_TYPE));
  vector_bytes = n_vecs * sizeof (sbitmap *);

  /* Round up `vector_bytes' to account for the alignment requirements
     of an sbitmap.  One could allocate the vector-table and set of sbitmaps
     separately, but that requires maintaining two pointers or creating
     a cover struct to hold both pointers (so our result is still just
     one pointer).  Neither is a bad idea, but this is simpler for now.  */
  {
    /* Based on DEFAULT_ALIGNMENT computation in obstack.c.  */
    struct { char x; SBITMAP_ELT_TYPE y; } align;
    int alignment = (char *) & align.y - & align.x;
    vector_bytes = (vector_bytes + alignment - 1) & ~ (alignment - 1);
  }

  amt = vector_bytes + (n_vecs * elm_bytes);
  bitmap_vector = (sbitmap *) xmalloc (amt);

  for (i = 0, offset = vector_bytes; i < n_vecs; i++, offset += elm_bytes)
    {
      sbitmap b = (sbitmap) ((char *) bitmap_vector + offset);

      bitmap_vector[i] = b;
      b->n_bits = n_elms;
      b->size = size;
      b->bytes = bytes;
    }

  return bitmap_vector;
}

/* Copy sbitmap SRC to DST.  */

void
sbitmap_copy (dst, src)
     sbitmap dst, src;
{
  memcpy (dst->elms, src->elms, sizeof (SBITMAP_ELT_TYPE) * dst->size);
}

/* Determine if a == b.  */
int
sbitmap_equal (a, b)
     sbitmap a, b;
{
  return !memcmp (a->elms, b->elms, sizeof (SBITMAP_ELT_TYPE) * a->size);
}
/* Zero all elements in a bitmap.  */

void
sbitmap_zero (bmap)
     sbitmap bmap;
{
  memset ((PTR) bmap->elms, 0, bmap->bytes);
}

/* Set all elements in a bitmap to ones.  */

void
sbitmap_ones (bmap)
     sbitmap bmap;
{
  unsigned int last_bit;

  memset ((PTR) bmap->elms, -1, bmap->bytes);

  last_bit = bmap->n_bits % SBITMAP_ELT_BITS;
  if (last_bit)
    bmap->elms[bmap->size - 1]
      = (SBITMAP_ELT_TYPE)-1 >> (SBITMAP_ELT_BITS - last_bit);
}

/* Zero a vector of N_VECS bitmaps.  */

void
sbitmap_vector_zero (bmap, n_vecs)
     sbitmap *bmap;
     unsigned int n_vecs;
{
  unsigned int i;

  for (i = 0; i < n_vecs; i++)
    sbitmap_zero (bmap[i]);
}

/* Set a vector of N_VECS bitmaps to ones.  */

void
sbitmap_vector_ones (bmap, n_vecs)
     sbitmap *bmap;
     unsigned int n_vecs;
{
  unsigned int i;

  for (i = 0; i < n_vecs; i++)
    sbitmap_ones (bmap[i]);
}

/* Set DST to be A union (B - C).
   DST = A | (B & ~C).
   Return non-zero if any change is made.  */

int
sbitmap_union_of_diff (dst, a, b, c)
     sbitmap dst, a, b, c;
{
  unsigned int i;
  sbitmap_ptr dstp, ap, bp, cp;
  int changed = 0;

  for (dstp = dst->elms, ap = a->elms, bp = b->elms, cp = c->elms, i = 0;
       i < dst->size; i++, dstp++)
    {
      SBITMAP_ELT_TYPE tmp = *ap++ | (*bp++ & ~*cp++);

      if (*dstp != tmp)
	{
	  changed = 1;
	  *dstp = tmp;
	}
    }

  return changed;
}

/* Set bitmap DST to the bitwise negation of the bitmap SRC.  */

void
sbitmap_not (dst, src)
     sbitmap dst, src;
{
  unsigned int i;
  sbitmap_ptr dstp, srcp;

  for (dstp = dst->elms, srcp = src->elms, i = 0; i < dst->size; i++)
    *dstp++ = ~(*srcp++);
}

/* Set the bits in DST to be the difference between the bits
   in A and the bits in B. i.e. dst = a & (~b).  */

void
sbitmap_difference (dst, a, b)
     sbitmap dst, a, b;
{
  unsigned int i;
  sbitmap_ptr dstp, ap, bp;
  
  for (dstp = dst->elms, ap = a->elms, bp = b->elms, i = 0; i < dst->size; i++)
    *dstp++ = *ap++ & (~*bp++);
}

/* Set DST to be (A and B).
   Return non-zero if any change is made.  */

int
sbitmap_a_and_b (dst, a, b)
     sbitmap dst, a, b;
{
  unsigned int i;
  sbitmap_ptr dstp, ap, bp;
  int changed = 0;

  for (dstp = dst->elms, ap = a->elms, bp = b->elms, i = 0; i < dst->size;
       i++, dstp++)
    {
      SBITMAP_ELT_TYPE tmp = *ap++ & *bp++;

      if (*dstp != tmp)
	{
	  changed = 1;
	  *dstp = tmp;
	}
    }

  return changed;
}

/* Set DST to be (A xor B)).
   Return non-zero if any change is made.  */

int
sbitmap_a_xor_b (dst, a, b)
     sbitmap dst, a, b;
{
  unsigned int i;
  sbitmap_ptr dstp, ap, bp;
  int changed = 0;
  
  for (dstp = dst->elms, ap = a->elms, bp = b->elms, i = 0; i < dst->size;
       i++, dstp++)
    {
      SBITMAP_ELT_TYPE tmp = *ap++ ^ *bp++;
      
      if (*dstp != tmp)
	{
	  changed = 1;
	  *dstp = tmp;
	}
    }
  return changed;
}

/* Set DST to be (A or B)).
   Return non-zero if any change is made.  */

int
sbitmap_a_or_b (dst, a, b)
     sbitmap dst, a, b;
{
  unsigned int i;
  sbitmap_ptr dstp, ap, bp;
  int changed = 0;

  for (dstp = dst->elms, ap = a->elms, bp = b->elms, i = 0; i < dst->size;
       i++, dstp++)
    {
      SBITMAP_ELT_TYPE tmp = *ap++ | *bp++;

      if (*dstp != tmp)
	{
	  changed = 1;
	  *dstp = tmp;
	}
    }

  return changed;
}

/* Return non-zero if A is a subset of B.  */

int
sbitmap_a_subset_b_p (a, b)
     sbitmap a, b;
{
  unsigned int i;
  sbitmap_ptr ap, bp;

  for (ap = a->elms, bp = b->elms, i = 0; i < a->size; i++, ap++, bp++)
    if ((*ap | *bp) != *bp)
      return 0;

  return 1;
}

/* Set DST to be (A or (B and C)).
   Return non-zero if any change is made.  */

int
sbitmap_a_or_b_and_c (dst, a, b, c)
     sbitmap dst, a, b, c;
{
  unsigned int i;
  sbitmap_ptr dstp, ap, bp, cp;
  int changed = 0;

  for (dstp = dst->elms, ap = a->elms, bp = b->elms, cp = c->elms, i = 0;
       i < dst->size; i++, dstp++)
    {
      SBITMAP_ELT_TYPE tmp = *ap++ | (*bp++ & *cp++);

      if (*dstp != tmp)
	{
	  changed = 1;
	  *dstp = tmp;
	}
    }

  return changed;
}

/* Set DST to be (A and (B or C)).
   Return non-zero if any change is made.  */

int
sbitmap_a_and_b_or_c (dst, a, b, c)
     sbitmap dst, a, b, c;
{
  unsigned int i;
  sbitmap_ptr dstp, ap, bp, cp;
  int changed = 0;

  for (dstp = dst->elms, ap = a->elms, bp = b->elms, cp = c->elms, i = 0;
       i < dst->size; i++, dstp++)
    {
      SBITMAP_ELT_TYPE tmp = *ap++ & (*bp++ | *cp++);

      if (*dstp != tmp)
	{
	  changed = 1;
	  *dstp = tmp;
	}
    }

  return changed;
}

#ifdef IN_GCC
/* Set the bitmap DST to the intersection of SRC of successors of
   block number BB, using the new flow graph structures.  */

void
sbitmap_intersection_of_succs (dst, src, bb)
     sbitmap dst;
     sbitmap *src;
     int bb;
{
  basic_block b = BASIC_BLOCK (bb);
  unsigned int set_size = dst->size;
  edge e;

  for (e = b->succ; e != 0; e = e->succ_next)
    {
      if (e->dest == EXIT_BLOCK_PTR)
        continue;

      sbitmap_copy (dst, src[e->dest->index]);
      break;
    }

  if (e == 0)
    sbitmap_ones (dst);
  else
    for (e = e->succ_next; e != 0; e = e->succ_next)
      {
	unsigned int i;
	sbitmap_ptr p, r;

	if (e->dest == EXIT_BLOCK_PTR)
	  continue;

	p = src[e->dest->index]->elms;
	r = dst->elms;
	for (i = 0; i < set_size; i++)
	  *r++ &= *p++;
      }
}

/* Set the bitmap DST to the intersection of SRC of predecessors of
   block number BB, using the new flow graph structures.  */

void
sbitmap_intersection_of_preds (dst, src, bb)
     sbitmap dst;
     sbitmap *src;
     int bb;
{
  basic_block b = BASIC_BLOCK (bb);
  unsigned int set_size = dst->size;
  edge e;

  for (e = b->pred; e != 0; e = e->pred_next)
    {
      if (e->src == ENTRY_BLOCK_PTR)
        continue;

      sbitmap_copy (dst, src[e->src->index]);
      break;
    }

  if (e == 0)
    sbitmap_ones (dst);
  else
    for (e = e->pred_next; e != 0; e = e->pred_next)
      {
	unsigned int i;
	sbitmap_ptr p, r;

	if (e->src == ENTRY_BLOCK_PTR)
	  continue;

	p = src[e->src->index]->elms;
	r = dst->elms;
	for (i = 0; i < set_size; i++)
	  *r++ &= *p++;
      }
}

/* Set the bitmap DST to the union of SRC of successors of
   block number BB, using the new flow graph structures.  */

void
sbitmap_union_of_succs (dst, src, bb)
     sbitmap dst;
     sbitmap *src;
     int bb;
{
  basic_block b = BASIC_BLOCK (bb);
  unsigned int set_size = dst->size;
  edge e;

  for (e = b->succ; e != 0; e = e->succ_next)
    {
      if (e->dest == EXIT_BLOCK_PTR)
        continue;

      sbitmap_copy (dst, src[e->dest->index]);
      break;
    }

  if (e == 0)
    sbitmap_zero (dst);
  else
    for (e = e->succ_next; e != 0; e = e->succ_next)
      {
	unsigned int i;
	sbitmap_ptr p, r;

	if (e->dest == EXIT_BLOCK_PTR)
	  continue;

	p = src[e->dest->index]->elms;
	r = dst->elms;
	for (i = 0; i < set_size; i++)
	  *r++ |= *p++;
      }
}

/* Set the bitmap DST to the union of SRC of predecessors of
   block number BB, using the new flow graph structures.  */

void
sbitmap_union_of_preds (dst, src, bb)
     sbitmap dst;
     sbitmap *src;
     int bb;
{
  basic_block b = BASIC_BLOCK (bb);
  unsigned int set_size = dst->size;
  edge e;

  for (e = b->pred; e != 0; e = e->pred_next)
    {
      if (e->src== ENTRY_BLOCK_PTR)
        continue;

      sbitmap_copy (dst, src[e->src->index]);
      break;
    }

  if (e == 0)
    sbitmap_zero (dst);
  else
    for (e = e->pred_next; e != 0; e = e->pred_next)
      {
	unsigned int i;
	sbitmap_ptr p, r;

	if (e->src == ENTRY_BLOCK_PTR)
	  continue;
	
	p = src[e->src->index]->elms;
	r = dst->elms;
	for (i = 0; i < set_size; i++)
	  *r++ |= *p++;
      }
}
#endif

/* Return number of first bit set in the bitmap, -1 if none.  */

int
sbitmap_first_set_bit (bmap)
     sbitmap bmap;
{
  unsigned int n;

  EXECUTE_IF_SET_IN_SBITMAP (bmap, 0, n, { return n; });
  return -1;
}

/* Return number of last bit set in the bitmap, -1 if none.  */

int
sbitmap_last_set_bit (bmap)
     sbitmap bmap;
{
  int i;
  SBITMAP_ELT_TYPE *ptr = bmap->elms;

  for (i = bmap->size - 1; i >= 0; i--)
    {
      SBITMAP_ELT_TYPE word = ptr[i];

      if (word != 0)
	{
	  unsigned int index = (i + 1) * SBITMAP_ELT_BITS - 1;
	  SBITMAP_ELT_TYPE mask
	    = (SBITMAP_ELT_TYPE) 1 << (SBITMAP_ELT_BITS - 1);

	  while (1)
	    {
	      if ((word & mask) != 0)
		return index;

	      mask >>= 1;
	      index--;
	    }
	}
    }

  return -1;
}

void
dump_sbitmap (file, bmap)
     FILE *file;
     sbitmap bmap;
{
  unsigned int i, n, j;
  unsigned int set_size = bmap->size;
  unsigned int total_bits = bmap->n_bits;

  fprintf (file, "  ");
  for (i = n = 0; i < set_size && n < total_bits; i++)
    for (j = 0; j < SBITMAP_ELT_BITS && n < total_bits; j++, n++)
      {
	if (n != 0 && n % 10 == 0)
	  fprintf (file, " ");

	fprintf (file, "%d",
		 (bmap->elms[i] & ((SBITMAP_ELT_TYPE) 1 << j)) != 0);
      }

  fprintf (file, "\n");
}

void
debug_sbitmap (bmap)
     sbitmap bmap;
{
  unsigned int i, pos;

  fprintf (stderr, "n_bits = %d, set = {", bmap->n_bits);

  for (pos = 30, i = 0; i < bmap->n_bits; i++)
    if (TEST_BIT (bmap, i))
      {
	if (pos > 70)
	  {
	    fprintf (stderr, "\n");
	    pos = 0;
	  }

	fprintf (stderr, "%d ", i);
	pos += 1 + (i >= 10) + (i >= 100);
      }

  fprintf (stderr, "}\n");
}

void
dump_sbitmap_vector (file, title, subtitle, bmaps, n_maps)
     FILE *file;
     const char *title, *subtitle;
     sbitmap *bmaps;
     int n_maps;
{
  int bb;

  fprintf (file, "%s\n", title);
  for (bb = 0; bb < n_maps; bb++)
    {
      fprintf (file, "%s %d\n", subtitle, bb);
      dump_sbitmap (file, bmaps[bb]);
    }

  fprintf (file, "\n");
}
