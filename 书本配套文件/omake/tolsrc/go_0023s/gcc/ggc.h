/* Garbage collection for the GNU compiler.
   Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include "varray.h"

/* Symbols are marked with `ggc' for `gcc gc' so as not to interfere with
   an external gc library that might be linked in.  */

/* These structures are defined in various headers throughout the
   compiler.  However, rather than force everyone who includes this
   header to include all the headers in which they are declared, we
   just forward-declare them here.  */
struct eh_status;
struct emit_status;
struct expr_status;
struct hash_table;
struct label_node;
struct rtx_def;
struct rtvec_def;
struct stmt_status;
union  tree_node;
struct varasm_status;

/* Constants for general use.  */
extern const char empty_string[];	/* empty string */
extern const char digit_vector[];	/* "0" .. "9" */
#define digit_string(d) (digit_vector + ((d) * 2))

/* Trees that have been marked, but whose children still need marking.  */
extern varray_type ggc_pending_trees;

/* Manipulate global roots that are needed between calls to gc.  */
extern void ggc_add_root		PARAMS ((void *base, int nelt,
						 int size, void (*)(void *)));
extern void ggc_add_rtx_root		PARAMS ((struct rtx_def **, int nelt));
extern void ggc_add_tree_root		PARAMS ((union tree_node **,
						 int nelt));
extern void ggc_add_rtx_varray_root	PARAMS ((struct varray_head_tag **,
						 int nelt));
extern void ggc_add_tree_varray_root	PARAMS ((struct varray_head_tag **,
						 int nelt));
extern void ggc_add_tree_hash_table_root PARAMS ((struct hash_table **,
						  int nelt));
extern void ggc_del_root		PARAMS ((void *base));

/* Types used for mark test and marking functions, if specified, in call
   below.  */
typedef int (*ggc_htab_marked_p) PARAMS ((const void *));
typedef void (*ggc_htab_mark) PARAMS ((const void *));

/* Add a hash table to be scanned when all roots have been processed.  We
   delete any entry in the table that has not been marked.  The argument is
   really htab_t.  */
extern void ggc_add_deletable_htab	PARAMS ((PTR, ggc_htab_marked_p,
						 ggc_htab_mark));

/* Mark nodes from the gc_add_root callback.  These functions follow
   pointers to mark other objects too.  */
extern void ggc_mark_rtx_varray		PARAMS ((struct varray_head_tag *));
extern void ggc_mark_tree_varray	PARAMS ((struct varray_head_tag *));
extern void ggc_mark_tree_hash_table	PARAMS ((struct hash_table *));
extern void ggc_mark_roots		PARAMS ((void));

extern void ggc_mark_rtx_children	PARAMS ((struct rtx_def *));
extern void ggc_mark_rtvec_children	PARAMS ((struct rtvec_def *));

/* If EXPR is not NULL and previously unmarked, mark it and evaluate
   to true.  Otherwise evaluate to false.  */
#define ggc_test_and_set_mark(EXPR) \
  ((EXPR) != NULL && ! ggc_set_mark (EXPR))

#define ggc_mark_rtx(EXPR)                      \
  do {                                          \
    rtx r__ = (EXPR);                           \
    if (ggc_test_and_set_mark (r__))            \
      ggc_mark_rtx_children (r__);              \
  } while (0)

#define ggc_mark_tree(EXPR)				\
  do {							\
    tree t__ = (EXPR);					\
    if (ggc_test_and_set_mark (t__))			\
      VARRAY_PUSH_TREE (ggc_pending_trees, t__);	\
  } while (0)

#define ggc_mark_nonnull_tree(EXPR)			\
  do {							\
    tree t__ = (EXPR);					\
    if (! ggc_set_mark (t__))				\
      VARRAY_PUSH_TREE (ggc_pending_trees, t__);	\
  } while (0)

#define ggc_mark_rtvec(EXPR)                    \
  do {                                          \
    rtvec v__ = (EXPR);                         \
    if (ggc_test_and_set_mark (v__))            \
      ggc_mark_rtvec_children (v__);            \
  } while (0)

#define ggc_mark(EXPR)				\
  do {						\
    const void *a__ = (EXPR);			\
    if (a__ != NULL)				\
      ggc_set_mark (a__);			\
  } while (0)

/* A GC implementation must provide these functions.  */

/* Initialize the garbage collector.  */
extern void init_ggc		PARAMS ((void));
extern void init_stringpool	PARAMS ((void));

/* Start a new GGC context.  Memory allocated in previous contexts
   will not be collected while the new context is active.  */
extern void ggc_push_context	PARAMS ((void));

/* Finish a GC context.  Any uncollected memory in the new context
   will be merged with the old context.  */
extern void ggc_pop_context 	PARAMS ((void));

/* Allocation.  */

/* The internal primitive.  */
extern void *ggc_alloc		PARAMS ((size_t));
/* Like ggc_alloc, but allocates cleared memory.  */
extern void *ggc_alloc_cleared	PARAMS ((size_t));

#define ggc_alloc_rtx(NSLOTS)						  \
  ((struct rtx_def *) ggc_alloc (sizeof (struct rtx_def)		  \
				 + ((NSLOTS) - 1) * sizeof (rtunion)))

#define ggc_alloc_rtvec(NELT)						  \
  ((struct rtvec_def *) ggc_alloc (sizeof (struct rtvec_def)		  \
				   + ((NELT) - 1) * sizeof (rtx)))

#define ggc_alloc_tree(LENGTH) ((union tree_node *) ggc_alloc (LENGTH))

/* Allocate a gc-able string, and fill it with LENGTH bytes from CONTENTS.
   If LENGTH is -1, then CONTENTS is assumed to be a
   null-terminated string and the memory sized accordingly.  */
extern const char *ggc_alloc_string	PARAMS ((const char *contents,
						 int length));

/* Make a copy of S, in GC-able memory.  */
#define ggc_strdup(S) ggc_alloc_string((S), -1)

/* Invoke the collector.  Garbage collection occurs only when this
   function is called, not during allocations.  */
extern void ggc_collect			PARAMS ((void));

/* Actually set the mark on a particular region of memory, but don't
   follow pointers.  This function is called by ggc_mark_*.  It
   returns zero if the object was not previously marked; non-zero if
   the object was already marked, or if, for any other reason,
   pointers in this data structure should not be traversed.  */
extern int ggc_set_mark			PARAMS ((const void *));

/* Return 1 if P has been marked, zero otherwise. 
   P must have been allocated by the GC allocator; it mustn't point to
   static objects, stack variables, or memory allocated with malloc.  */
extern int ggc_marked_p			PARAMS ((const void *));

/* Callbacks to the languages.  */

/* This is the language's opportunity to mark nodes held through
   the lang_specific hooks in the tree.  */
extern void lang_mark_tree		PARAMS ((union tree_node *));

/* The FALSE_LABEL_STACK, declared in except.h, has language-dependent
   semantics.  If a front-end needs to mark the false label stack, it
   should set this pointer to a non-NULL value.  Otherwise, no marking
   will be done.  */
extern void (*lang_mark_false_label_stack) PARAMS ((struct label_node *));

/* Mark functions for various structs scattered about.  */

void mark_eh_status			PARAMS ((struct eh_status *));
void mark_emit_status			PARAMS ((struct emit_status *));
void mark_expr_status			PARAMS ((struct expr_status *));
void mark_stmt_status			PARAMS ((struct stmt_status *));
void mark_varasm_status			PARAMS ((struct varasm_status *));
void mark_optab				PARAMS ((void *));

/* Statistics.  */

/* This structure contains the statistics common to all collectors.
   Particular collectors can extend this structure.  */
typedef struct ggc_statistics 
{
  /* The Ith element is the number of nodes allocated with code I.  */
  unsigned num_trees[256];
  /* The Ith element is the number of bytes allocated by nodes with 
     code I.  */
  size_t size_trees[256];
  /* The Ith element is the number of nodes allocated with code I.  */
  unsigned num_rtxs[256];
  /* The Ith element is the number of bytes allocated by nodes with 
     code I.  */
  size_t size_rtxs[256];
  /* The total size of the tree nodes allocated.  */
  size_t total_size_trees;
  /* The total size of the RTL nodes allocated.  */
  size_t total_size_rtxs;
  /* The total number of tree nodes allocated.  */
  unsigned total_num_trees;
  /* The total number of RTL nodes allocated.  */
  unsigned total_num_rtxs;
} ggc_statistics;

/* Return the number of bytes allocated at the indicated address.  */
extern size_t ggc_get_size		PARAMS ((const void *));

/* Used by the various collectors to gather and print statistics that
   do not depend on the collector in use.  */
extern void ggc_print_common_statistics PARAMS ((FILE *, ggc_statistics *));

/* Print allocation statistics.  */
extern void ggc_print_statistics	PARAMS ((void));
extern void stringpool_statistics	PARAMS ((void));
