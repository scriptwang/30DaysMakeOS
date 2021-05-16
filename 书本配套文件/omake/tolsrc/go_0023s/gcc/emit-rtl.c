/* Emit RTL for the GNU C-Compiler expander.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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


/* Middle-to-low level generation of rtx code and insns.

   This file contains the functions `gen_rtx', `gen_reg_rtx'
   and `gen_label_rtx' that are the usual ways of creating rtl
   expressions for most purposes.

   It also has the functions for creating insns and linking
   them in the doubly-linked chain.

   The patterns of the insns are created by machine-dependent
   routines in insn-emit.c, which is generated automatically from
   the machine description.  These routines use `gen_rtx' to make
   the individual rtx's of the pattern; what is machine dependent
   is the kind of rtx's they make and what arguments they use.  */

/* !kawai! */
#include "config.h"
#include "system.h"
#include "toplev.h"
#include "rtl.h"
#include "tree.h"
#include "tm_p.h"
#include "flags.h"
#include "function.h"
#include "expr.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "../include/hashtab.h"
#include "insn-config.h"
#include "recog.h"
#include "real.h"
#include "../include/obstack.h"
#include "bitmap.h"
#include "basic-block.h"
#include "ggc.h"
#include "debug.h"
#include "langhooks.h"
/* end of !kawai! */

/* Commonly used modes.  */

enum machine_mode byte_mode;	/* Mode whose width is BITS_PER_UNIT.  */
enum machine_mode word_mode;	/* Mode whose width is BITS_PER_WORD.  */
enum machine_mode double_mode;	/* Mode whose width is DOUBLE_TYPE_SIZE.  */
enum machine_mode ptr_mode;	/* Mode whose width is POINTER_SIZE.  */


/* This is *not* reset after each function.  It gives each CODE_LABEL
   in the entire compilation a unique label number.  */

static int label_num = 1;

/* Highest label number in current function.
   Zero means use the value of label_num instead.
   This is nonzero only when belatedly compiling an inline function.  */

static int last_label_num;

/* Value label_num had when set_new_first_and_last_label_number was called.
   If label_num has not changed since then, last_label_num is valid.  */

static int base_label_num;

/* Nonzero means do not generate NOTEs for source line numbers.  */

static int no_line_numbers;

/* Commonly used rtx's, so that we only need space for one copy.
   These are initialized once for the entire compilation.
   All of these except perhaps the floating-point CONST_DOUBLEs
   are unique; no other rtx-object will be equal to any of these.  */

rtx global_rtl[GR_MAX];

/* We record floating-point CONST_DOUBLEs in each floating-point mode for
   the values of 0, 1, and 2.  For the integer entries and VOIDmode, we
   record a copy of const[012]_rtx.  */

rtx const_tiny_rtx[3][(int) MAX_MACHINE_MODE];

rtx const_true_rtx;

REAL_VALUE_TYPE dconst0;
REAL_VALUE_TYPE dconst1;
REAL_VALUE_TYPE dconst2;
REAL_VALUE_TYPE dconstm1;

/* All references to the following fixed hard registers go through
   these unique rtl objects.  On machines where the frame-pointer and
   arg-pointer are the same register, they use the same unique object.

   After register allocation, other rtl objects which used to be pseudo-regs
   may be clobbered to refer to the frame-pointer register.
   But references that were originally to the frame-pointer can be
   distinguished from the others because they contain frame_pointer_rtx.

   When to use frame_pointer_rtx and hard_frame_pointer_rtx is a little
   tricky: until register elimination has taken place hard_frame_pointer_rtx
   should be used if it is being set, and frame_pointer_rtx otherwise.  After
   register elimination hard_frame_pointer_rtx should always be used.
   On machines where the two registers are same (most) then these are the
   same.

   In an inline procedure, the stack and frame pointer rtxs may not be
   used for anything else.  */
rtx struct_value_rtx;		/* (REG:Pmode STRUCT_VALUE_REGNUM) */
rtx struct_value_incoming_rtx;	/* (REG:Pmode STRUCT_VALUE_INCOMING_REGNUM) */
rtx static_chain_rtx;		/* (REG:Pmode STATIC_CHAIN_REGNUM) */
rtx static_chain_incoming_rtx;	/* (REG:Pmode STATIC_CHAIN_INCOMING_REGNUM) */
rtx pic_offset_table_rtx;	/* (REG:Pmode PIC_OFFSET_TABLE_REGNUM) */

/* This is used to implement __builtin_return_address for some machines.
   See for instance the MIPS port.  */
rtx return_address_pointer_rtx;	/* (REG:Pmode RETURN_ADDRESS_POINTER_REGNUM) */

/* We make one copy of (const_int C) where C is in
   [- MAX_SAVED_CONST_INT, MAX_SAVED_CONST_INT]
   to save space during the compilation and simplify comparisons of
   integers.  */

rtx const_int_rtx[MAX_SAVED_CONST_INT * 2 + 1];

/* A hash table storing CONST_INTs whose absolute value is greater
   than MAX_SAVED_CONST_INT.  */

static htab_t const_int_htab;

/* A hash table storing memory attribute structures.  */
static htab_t mem_attrs_htab;

/* start_sequence and gen_sequence can make a lot of rtx expressions which are
   shortly thrown away.  We use two mechanisms to prevent this waste:

   For sizes up to 5 elements, we keep a SEQUENCE and its associated
   rtvec for use by gen_sequence.  One entry for each size is
   sufficient because most cases are calls to gen_sequence followed by
   immediately emitting the SEQUENCE.  Reuse is safe since emitting a
   sequence is destructive on the insn in it anyway and hence can't be
   redone.

   We do not bother to save this cached data over nested function calls.
   Instead, we just reinitialize them.  */

#define SEQUENCE_RESULT_SIZE 5

static rtx sequence_result[SEQUENCE_RESULT_SIZE];

/* During RTL generation, we also keep a list of free INSN rtl codes.  */
static rtx free_insn;

#define first_insn (cfun->emit->x_first_insn)
#define last_insn (cfun->emit->x_last_insn)
#define cur_insn_uid (cfun->emit->x_cur_insn_uid)
#define last_linenum (cfun->emit->x_last_linenum)
#define last_filename (cfun->emit->x_last_filename)
#define first_label_num (cfun->emit->x_first_label_num)

static rtx make_jump_insn_raw		PARAMS ((rtx));
static rtx make_call_insn_raw		PARAMS ((rtx));
static rtx find_line_note		PARAMS ((rtx));
static void mark_sequence_stack         PARAMS ((struct sequence_stack *));
static rtx change_address_1		PARAMS ((rtx, enum machine_mode, rtx,
						 int));
static void unshare_all_rtl_1		PARAMS ((rtx));
static void unshare_all_decls		PARAMS ((tree));
static void reset_used_decls		PARAMS ((tree));
static void mark_label_nuses		PARAMS ((rtx));
static hashval_t const_int_htab_hash    PARAMS ((const void *));
static int const_int_htab_eq            PARAMS ((const void *,
						 const void *));
static hashval_t mem_attrs_htab_hash    PARAMS ((const void *));
static int mem_attrs_htab_eq            PARAMS ((const void *,
						 const void *));
static void mem_attrs_mark		PARAMS ((const void *));
static mem_attrs *get_mem_attrs		PARAMS ((HOST_WIDE_INT, tree, rtx,
						 rtx, unsigned int,
						 enum machine_mode));
static tree component_ref_for_mem_expr	PARAMS ((tree));
static rtx gen_const_vector_0		PARAMS ((enum machine_mode));

/* Probability of the conditional branch currently proceeded by try_split.
   Set to -1 otherwise.  */
int split_branch_probability = -1;

/* Returns a hash code for X (which is a really a CONST_INT).  */

static hashval_t
const_int_htab_hash (x)
     const void *x;
{
  return (hashval_t) INTVAL ((const struct rtx_def *) x);
}

/* Returns non-zero if the value represented by X (which is really a
   CONST_INT) is the same as that given by Y (which is really a
   HOST_WIDE_INT *).  */

static int
const_int_htab_eq (x, y)
     const void *x;
     const void *y;
{
  return (INTVAL ((const struct rtx_def *) x) == *((const HOST_WIDE_INT *) y));
}

/* Returns a hash code for X (which is a really a mem_attrs *).  */

static hashval_t
mem_attrs_htab_hash (x)
     const void *x;
{
  mem_attrs *p = (mem_attrs *) x;

  return (p->alias ^ (p->align * 1000)
	  ^ ((p->offset ? INTVAL (p->offset) : 0) * 50000)
	  ^ ((p->size ? INTVAL (p->size) : 0) * 2500000)
	  ^ (size_t) p->expr);
}

/* Returns non-zero if the value represented by X (which is really a
   mem_attrs *) is the same as that given by Y (which is also really a
   mem_attrs *).  */

static int
mem_attrs_htab_eq (x, y)
     const void *x;
     const void *y;
{
  mem_attrs *p = (mem_attrs *) x;
  mem_attrs *q = (mem_attrs *) y;

  return (p->alias == q->alias && p->expr == q->expr && p->offset == q->offset
	  && p->size == q->size && p->align == q->align);
}

/* This routine is called when we determine that we need a mem_attrs entry.
   It marks the associated decl and RTL as being used, if present.  */

static void
mem_attrs_mark (x)
     const void *x;
{
  mem_attrs *p = (mem_attrs *) x;

  if (p->expr)
    ggc_mark_tree (p->expr);

  if (p->offset)
    ggc_mark_rtx (p->offset);

  if (p->size)
    ggc_mark_rtx (p->size);
}

/* Allocate a new mem_attrs structure and insert it into the hash table if
   one identical to it is not already in the table.  We are doing this for
   MEM of mode MODE.  */

static mem_attrs *
get_mem_attrs (alias, expr, offset, size, align, mode)
     HOST_WIDE_INT alias;
     tree expr;
     rtx offset;
     rtx size;
     unsigned int align;
     enum machine_mode mode;
{
  mem_attrs attrs;
  void **slot;

  /* If everything is the default, we can just return zero.  */
  if (alias == 0 && expr == 0 && offset == 0
      && (size == 0
	  || (mode != BLKmode && GET_MODE_SIZE (mode) == INTVAL (size)))
      && (align == BITS_PER_UNIT
	  || (STRICT_ALIGNMENT
	      && mode != BLKmode && align == GET_MODE_ALIGNMENT (mode))))
    return 0;

  attrs.alias = alias;
  attrs.expr = expr;
  attrs.offset = offset;
  attrs.size = size;
  attrs.align = align;

  slot = htab_find_slot (mem_attrs_htab, &attrs, INSERT);
  if (*slot == 0)
    {
      *slot = ggc_alloc (sizeof (mem_attrs));
      memcpy (*slot, &attrs, sizeof (mem_attrs));
    }

  return *slot;
}

/* Generate a new REG rtx.  Make sure ORIGINAL_REGNO is set properly, and
   don't attempt to share with the various global pieces of rtl (such as
   frame_pointer_rtx).  */

rtx
gen_raw_REG (mode, regno)
     enum machine_mode mode;
     int regno;
{
  rtx x = gen_rtx_raw_REG (mode, regno);
  ORIGINAL_REGNO (x) = regno;
  return x;
}

/* There are some RTL codes that require special attention; the generation
   functions do the raw handling.  If you add to this list, modify
   special_rtx in gengenrtl.c as well.  */

rtx
gen_rtx_CONST_INT (mode, arg)
     enum machine_mode mode ATTRIBUTE_UNUSED;
     HOST_WIDE_INT arg;
{
  void **slot;

  if (arg >= - MAX_SAVED_CONST_INT && arg <= MAX_SAVED_CONST_INT)
    return const_int_rtx[arg + MAX_SAVED_CONST_INT];

#if STORE_FLAG_VALUE != 1 && STORE_FLAG_VALUE != -1
  if (const_true_rtx && arg == STORE_FLAG_VALUE)
    return const_true_rtx;
#endif

  /* Look up the CONST_INT in the hash table.  */
  slot = htab_find_slot_with_hash (const_int_htab, &arg,
				   (hashval_t) arg, INSERT);
  if (*slot == 0)
    *slot = gen_rtx_raw_CONST_INT (VOIDmode, arg);

  return (rtx) *slot;
}

rtx
gen_int_mode (c, mode)
     HOST_WIDE_INT c;
     enum machine_mode mode;
{
  return GEN_INT (trunc_int_for_mode (c, mode));
}

/* CONST_DOUBLEs needs special handling because their length is known
   only at run-time.  */

rtx
gen_rtx_CONST_DOUBLE (mode, arg0, arg1)
     enum machine_mode mode;
     HOST_WIDE_INT arg0, arg1;
{
  rtx r = rtx_alloc (CONST_DOUBLE);
  int i;

  PUT_MODE (r, mode);
  X0EXP (r, 0) = NULL_RTX;
  XWINT (r, 1) = arg0;
  XWINT (r, 2) = arg1;

  for (i = GET_RTX_LENGTH (CONST_DOUBLE) - 1; i > 2; --i)
    XWINT (r, i) = 0;

  return r;
}

rtx
gen_rtx_REG (mode, regno)
     enum machine_mode mode;
     int regno;
{
  /* In case the MD file explicitly references the frame pointer, have
     all such references point to the same frame pointer.  This is
     used during frame pointer elimination to distinguish the explicit
     references to these registers from pseudos that happened to be
     assigned to them.

     If we have eliminated the frame pointer or arg pointer, we will
     be using it as a normal register, for example as a spill
     register.  In such cases, we might be accessing it in a mode that
     is not Pmode and therefore cannot use the pre-allocated rtx.

     Also don't do this when we are making new REGs in reload, since
     we don't want to get confused with the real pointers.  */

  if (mode == Pmode && !reload_in_progress)
    {
      if (regno == FRAME_POINTER_REGNUM)
	return frame_pointer_rtx;
#if FRAME_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
      if (regno == HARD_FRAME_POINTER_REGNUM)
	return hard_frame_pointer_rtx;
#endif
#if FRAME_POINTER_REGNUM != ARG_POINTER_REGNUM && HARD_FRAME_POINTER_REGNUM != ARG_POINTER_REGNUM
      if (regno == ARG_POINTER_REGNUM)
	return arg_pointer_rtx;
#endif
#ifdef RETURN_ADDRESS_POINTER_REGNUM
      if (regno == RETURN_ADDRESS_POINTER_REGNUM)
	return return_address_pointer_rtx;
#endif
      if (regno == PIC_OFFSET_TABLE_REGNUM
	  && fixed_regs[PIC_OFFSET_TABLE_REGNUM])
        return pic_offset_table_rtx;
      if (regno == STACK_POINTER_REGNUM)
	return stack_pointer_rtx;
    }

  return gen_raw_REG (mode, regno);
}

rtx
gen_rtx_MEM (mode, addr)
     enum machine_mode mode;
     rtx addr;
{
  rtx rt = gen_rtx_raw_MEM (mode, addr);

  /* This field is not cleared by the mere allocation of the rtx, so
     we clear it here.  */
  MEM_ATTRS (rt) = 0;

  return rt;
}

rtx
gen_rtx_SUBREG (mode, reg, offset)
     enum machine_mode mode;
     rtx reg;
     int offset;
{
  /* This is the most common failure type.
     Catch it early so we can see who does it.  */
  if ((offset % GET_MODE_SIZE (mode)) != 0)
    abort ();

  /* This check isn't usable right now because combine will
     throw arbitrary crap like a CALL into a SUBREG in
     gen_lowpart_for_combine so we must just eat it.  */
#if 0
  /* Check for this too.  */
  if (offset >= GET_MODE_SIZE (GET_MODE (reg)))
    abort ();
#endif
  return gen_rtx_fmt_ei (SUBREG, mode, reg, offset);
}

/* Generate a SUBREG representing the least-significant part of REG if MODE
   is smaller than mode of REG, otherwise paradoxical SUBREG.  */

rtx
gen_lowpart_SUBREG (mode, reg)
     enum machine_mode mode;
     rtx reg;
{
  enum machine_mode inmode;

  inmode = GET_MODE (reg);
  if (inmode == VOIDmode)
    inmode = mode;
  return gen_rtx_SUBREG (mode, reg,
			 subreg_lowpart_offset (mode, inmode));
}

/* rtx gen_rtx (code, mode, [element1, ..., elementn])
**
**	    This routine generates an RTX of the size specified by
**	<code>, which is an RTX code.   The RTX structure is initialized
**	from the arguments <element1> through <elementn>, which are
**	interpreted according to the specific RTX type's format.   The
**	special machine mode associated with the rtx (if any) is specified
**	in <mode>.
**
**	    gen_rtx can be invoked in a way which resembles the lisp-like
**	rtx it will generate.   For example, the following rtx structure:
**
**	      (plus:QI (mem:QI (reg:SI 1))
**		       (mem:QI (plusw:SI (reg:SI 2) (reg:SI 3))))
**
**		...would be generated by the following C code:
**
**		gen_rtx (PLUS, QImode,
**		    gen_rtx (MEM, QImode,
**			gen_rtx (REG, SImode, 1)),
**		    gen_rtx (MEM, QImode,
**			gen_rtx (PLUS, SImode,
**			    gen_rtx (REG, SImode, 2),
**			    gen_rtx (REG, SImode, 3)))),
*/

/*VARARGS2*/
rtx
gen_rtx VPARAMS ((enum rtx_code code, enum machine_mode mode, ...))
{
  int i;		/* Array indices...			*/
  const char *fmt;	/* Current rtx's format...		*/
  rtx rt_val;		/* RTX to return to caller...		*/

  VA_OPEN (p, mode);
  VA_FIXEDARG (p, enum rtx_code, code);
  VA_FIXEDARG (p, enum machine_mode, mode);

  switch (code)
    {
    case CONST_INT:
      rt_val = gen_rtx_CONST_INT (mode, va_arg (p, HOST_WIDE_INT));
      break;

    case CONST_DOUBLE:
      {
	HOST_WIDE_INT arg0 = va_arg (p, HOST_WIDE_INT);
	HOST_WIDE_INT arg1 = va_arg (p, HOST_WIDE_INT);

        rt_val = gen_rtx_CONST_DOUBLE (mode, arg0, arg1);
      }
      break;

    case REG:
      rt_val = gen_rtx_REG (mode, va_arg (p, int));
      break;

    case MEM:
      rt_val = gen_rtx_MEM (mode, va_arg (p, rtx));
      break;

    default:
      rt_val = rtx_alloc (code);	/* Allocate the storage space.  */
      rt_val->mode = mode;		/* Store the machine mode...  */

      fmt = GET_RTX_FORMAT (code);	/* Find the right format...  */
      for (i = 0; i < GET_RTX_LENGTH (code); i++)
	{
	  switch (*fmt++)
	    {
	    case '0':		/* Unused field.  */
	      break;

	    case 'i':		/* An integer?  */
	      XINT (rt_val, i) = va_arg (p, int);
	      break;

	    case 'w':		/* A wide integer? */
	      XWINT (rt_val, i) = va_arg (p, HOST_WIDE_INT);
	      break;

	    case 's':		/* A string?  */
	      XSTR (rt_val, i) = va_arg (p, char *);
	      break;

	    case 'e':		/* An expression?  */
	    case 'u':		/* An insn?  Same except when printing.  */
	      XEXP (rt_val, i) = va_arg (p, rtx);
	      break;

	    case 'E':		/* An RTX vector?  */
	      XVEC (rt_val, i) = va_arg (p, rtvec);
	      break;

	    case 'b':           /* A bitmap? */
	      XBITMAP (rt_val, i) = va_arg (p, bitmap);
	      break;

	    case 't':           /* A tree? */
	      XTREE (rt_val, i) = va_arg (p, tree);
	      break;

	    default:
	      abort ();
	    }
	}
      break;
    }

  VA_CLOSE (p);
  return rt_val;
}

/* gen_rtvec (n, [rt1, ..., rtn])
**
**	    This routine creates an rtvec and stores within it the
**	pointers to rtx's which are its arguments.
*/

/*VARARGS1*/
rtvec
gen_rtvec VPARAMS ((int n, ...))
{
  int i, save_n;
  rtx *vector;

  VA_OPEN (p, n);
  VA_FIXEDARG (p, int, n);

  if (n == 0)
    return NULL_RTVEC;		/* Don't allocate an empty rtvec...	*/

  vector = (rtx *) alloca (n * sizeof (rtx));

  for (i = 0; i < n; i++)
    vector[i] = va_arg (p, rtx);

  /* The definition of VA_* in K&R C causes `n' to go out of scope.  */
  save_n = n;
  VA_CLOSE (p);

  return gen_rtvec_v (save_n, vector);
}

rtvec
gen_rtvec_v (n, argp)
     int n;
     rtx *argp;
{
  int i;
  rtvec rt_val;

  if (n == 0)
    return NULL_RTVEC;		/* Don't allocate an empty rtvec...	*/

  rt_val = rtvec_alloc (n);	/* Allocate an rtvec...			*/

  for (i = 0; i < n; i++)
    rt_val->elem[i] = *argp++;

  return rt_val;
}

/* Generate a REG rtx for a new pseudo register of mode MODE.
   This pseudo is assigned the next sequential register number.  */

rtx
gen_reg_rtx (mode)
     enum machine_mode mode;
{
  struct function *f = cfun;
  rtx val;

  /* Don't let anything called after initial flow analysis create new
     registers.  */
  if (no_new_pseudos)
    abort ();

  if (generating_concat_p
      && (GET_MODE_CLASS (mode) == MODE_COMPLEX_FLOAT
	  || GET_MODE_CLASS (mode) == MODE_COMPLEX_INT))
    {
      /* For complex modes, don't make a single pseudo.
	 Instead, make a CONCAT of two pseudos.
	 This allows noncontiguous allocation of the real and imaginary parts,
	 which makes much better code.  Besides, allocating DCmode
	 pseudos overstrains reload on some machines like the 386.  */
      rtx realpart, imagpart;
      int size = GET_MODE_UNIT_SIZE (mode);
      enum machine_mode partmode
	= mode_for_size (size * BITS_PER_UNIT,
			 (GET_MODE_CLASS (mode) == MODE_COMPLEX_FLOAT
			  ? MODE_FLOAT : MODE_INT),
			 0);

      realpart = gen_reg_rtx (partmode);
      imagpart = gen_reg_rtx (partmode);
      return gen_rtx_CONCAT (mode, realpart, imagpart);
    }

  /* Make sure regno_pointer_align, regno_decl, and regno_reg_rtx are large
     enough to have an element for this pseudo reg number.  */

  if (reg_rtx_no == f->emit->regno_pointer_align_length)
    {
      int old_size = f->emit->regno_pointer_align_length;
      char *new;
      rtx *new1;
      tree *new2;

      new = xrealloc (f->emit->regno_pointer_align, old_size * 2);
      memset (new + old_size, 0, old_size);
      f->emit->regno_pointer_align = (unsigned char *) new;

      new1 = (rtx *) xrealloc (f->emit->x_regno_reg_rtx,
			       old_size * 2 * sizeof (rtx));
      memset (new1 + old_size, 0, old_size * sizeof (rtx));
      regno_reg_rtx = new1;

      new2 = (tree *) xrealloc (f->emit->regno_decl,
				old_size * 2 * sizeof (tree));
      memset (new2 + old_size, 0, old_size * sizeof (tree));
      f->emit->regno_decl = new2;

      f->emit->regno_pointer_align_length = old_size * 2;
    }

  val = gen_raw_REG (mode, reg_rtx_no);
  regno_reg_rtx[reg_rtx_no++] = val;
  return val;
}

/* Identify REG (which may be a CONCAT) as a user register.  */

void
mark_user_reg (reg)
     rtx reg;
{
  if (GET_CODE (reg) == CONCAT)
    {
      REG_USERVAR_P (XEXP (reg, 0)) = 1;
      REG_USERVAR_P (XEXP (reg, 1)) = 1;
    }
  else if (GET_CODE (reg) == REG)
    REG_USERVAR_P (reg) = 1;
  else
    abort ();
}

/* Identify REG as a probable pointer register and show its alignment
   as ALIGN, if nonzero.  */

void
mark_reg_pointer (reg, align)
     rtx reg;
     int align;
{
  if (! REG_POINTER (reg))
    {
      REG_POINTER (reg) = 1;

      if (align)
	REGNO_POINTER_ALIGN (REGNO (reg)) = align;
    }
  else if (align && align < REGNO_POINTER_ALIGN (REGNO (reg)))
    /* We can no-longer be sure just how aligned this pointer is */
    REGNO_POINTER_ALIGN (REGNO (reg)) = align;
}

/* Return 1 plus largest pseudo reg number used in the current function.  */

int
max_reg_num ()
{
  return reg_rtx_no;
}

/* Return 1 + the largest label number used so far in the current function.  */

int
max_label_num ()
{
  if (last_label_num && label_num == base_label_num)
    return last_label_num;
  return label_num;
}

/* Return first label number used in this function (if any were used).  */

int
get_first_label_num ()
{
  return first_label_num;
}

/* Return the final regno of X, which is a SUBREG of a hard
   register.  */
int
subreg_hard_regno (x, check_mode)
     rtx x;
     int check_mode;
{
  enum machine_mode mode = GET_MODE (x);
  unsigned int byte_offset, base_regno, final_regno;
  rtx reg = SUBREG_REG (x);

  /* This is where we attempt to catch illegal subregs
     created by the compiler.  */
  if (GET_CODE (x) != SUBREG
      || GET_CODE (reg) != REG)
    abort ();
  base_regno = REGNO (reg);
  if (base_regno >= FIRST_PSEUDO_REGISTER)
    abort ();
  if (check_mode && ! HARD_REGNO_MODE_OK (base_regno, GET_MODE (reg)))
    abort ();

  /* Catch non-congruent offsets too.  */
  byte_offset = SUBREG_BYTE (x);
  if ((byte_offset % GET_MODE_SIZE (mode)) != 0)
    abort ();

  final_regno = subreg_regno (x);

  return final_regno;
}

/* Return a value representing some low-order bits of X, where the number
   of low-order bits is given by MODE.  Note that no conversion is done
   between floating-point and fixed-point values, rather, the bit
   representation is returned.

   This function handles the cases in common between gen_lowpart, below,
   and two variants in cse.c and combine.c.  These are the cases that can
   be safely handled at all points in the compilation.

   If this is not a case we can handle, return 0.  */

rtx
gen_lowpart_common (mode, x)
     enum machine_mode mode;
     rtx x;
{
  int msize = GET_MODE_SIZE (mode);
  int xsize = GET_MODE_SIZE (GET_MODE (x));
  int offset = 0;

  if (GET_MODE (x) == mode)
    return x;

  /* MODE must occupy no more words than the mode of X.  */
  if (GET_MODE (x) != VOIDmode
      && ((msize + (UNITS_PER_WORD - 1)) / UNITS_PER_WORD
	  > ((xsize + (UNITS_PER_WORD - 1)) / UNITS_PER_WORD)))
    return 0;

  /* Don't allow generating paradoxical FLOAT_MODE subregs.  */
  if (GET_MODE_CLASS (mode) == MODE_FLOAT
      && GET_MODE (x) != VOIDmode && msize > xsize)
    return 0;

  offset = subreg_lowpart_offset (mode, GET_MODE (x));

  if ((GET_CODE (x) == ZERO_EXTEND || GET_CODE (x) == SIGN_EXTEND)
      && (GET_MODE_CLASS (mode) == MODE_INT
	  || GET_MODE_CLASS (mode) == MODE_PARTIAL_INT))
    {
      /* If we are getting the low-order part of something that has been
	 sign- or zero-extended, we can either just use the object being
	 extended or make a narrower extension.  If we want an even smaller
	 piece than the size of the object being extended, call ourselves
	 recursively.

	 This case is used mostly by combine and cse.  */

      if (GET_MODE (XEXP (x, 0)) == mode)
	return XEXP (x, 0);
      else if (GET_MODE_SIZE (mode) < GET_MODE_SIZE (GET_MODE (XEXP (x, 0))))
	return gen_lowpart_common (mode, XEXP (x, 0));
      else if (GET_MODE_SIZE (mode) < GET_MODE_SIZE (GET_MODE (x)))
	return gen_rtx_fmt_e (GET_CODE (x), mode, XEXP (x, 0));
    }
  else if (GET_CODE (x) == SUBREG || GET_CODE (x) == REG
	   || GET_CODE (x) == CONCAT)
    return simplify_gen_subreg (mode, x, GET_MODE (x), offset);
  /* If X is a CONST_INT or a CONST_DOUBLE, extract the appropriate bits
     from the low-order part of the constant.  */
  else if ((GET_MODE_CLASS (mode) == MODE_INT
	    || GET_MODE_CLASS (mode) == MODE_PARTIAL_INT)
	   && GET_MODE (x) == VOIDmode
	   && (GET_CODE (x) == CONST_INT || GET_CODE (x) == CONST_DOUBLE))
    {
      /* If MODE is twice the host word size, X is already the desired
	 representation.  Otherwise, if MODE is wider than a word, we can't
	 do this.  If MODE is exactly a word, return just one CONST_INT.  */

      if (GET_MODE_BITSIZE (mode) >= 2 * HOST_BITS_PER_WIDE_INT)
	return x;
      else if (GET_MODE_BITSIZE (mode) > HOST_BITS_PER_WIDE_INT)
	return 0;
      else if (GET_MODE_BITSIZE (mode) == HOST_BITS_PER_WIDE_INT)
	return (GET_CODE (x) == CONST_INT ? x
		: GEN_INT (CONST_DOUBLE_LOW (x)));
      else
	{
	  /* MODE must be narrower than HOST_BITS_PER_WIDE_INT.  */
	  HOST_WIDE_INT val = (GET_CODE (x) == CONST_INT ? INTVAL (x)
			       : CONST_DOUBLE_LOW (x));

	  /* Sign extend to HOST_WIDE_INT.  */
	  val = trunc_int_for_mode (val, mode);

	  return (GET_CODE (x) == CONST_INT && INTVAL (x) == val ? x
		  : GEN_INT (val));
	}
    }

#ifndef REAL_ARITHMETIC
  /* If X is an integral constant but we want it in floating-point, it
     must be the case that we have a union of an integer and a floating-point
     value.  If the machine-parameters allow it, simulate that union here
     and return the result.  The two-word and single-word cases are
     different.  */

  else if (((HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT
	     && HOST_BITS_PER_WIDE_INT == BITS_PER_WORD)
	    || flag_pretend_float)
	   && GET_MODE_CLASS (mode) == MODE_FLOAT
	   && GET_MODE_SIZE (mode) == UNITS_PER_WORD
	   && GET_CODE (x) == CONST_INT
	   && sizeof (float) * HOST_BITS_PER_CHAR == HOST_BITS_PER_WIDE_INT)
    {
      union {HOST_WIDE_INT i; float d; } u;

      u.i = INTVAL (x);
      return CONST_DOUBLE_FROM_REAL_VALUE (u.d, mode);
    }
  else if (((HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT
	     && HOST_BITS_PER_WIDE_INT == BITS_PER_WORD)
	    || flag_pretend_float)
	   && GET_MODE_CLASS (mode) == MODE_FLOAT
	   && GET_MODE_SIZE (mode) == 2 * UNITS_PER_WORD
	   && (GET_CODE (x) == CONST_INT || GET_CODE (x) == CONST_DOUBLE)
	   && GET_MODE (x) == VOIDmode
	   && (sizeof (double) * HOST_BITS_PER_CHAR
	       == 2 * HOST_BITS_PER_WIDE_INT))
    {
      union {HOST_WIDE_INT i[2]; double d; } u;
      HOST_WIDE_INT low, high;

      if (GET_CODE (x) == CONST_INT)
	low = INTVAL (x), high = low >> (HOST_BITS_PER_WIDE_INT -1);
      else
	low = CONST_DOUBLE_LOW (x), high = CONST_DOUBLE_HIGH (x);
#ifdef HOST_WORDS_BIG_ENDIAN
      u.i[0] = high, u.i[1] = low;
#else
      u.i[0] = low, u.i[1] = high;
#endif
      return CONST_DOUBLE_FROM_REAL_VALUE (u.d, mode);
    }

  /* Similarly, if this is converting a floating-point value into a
     single-word integer.  Only do this is the host and target parameters are
     compatible.  */

  else if (((HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT
	     && HOST_BITS_PER_WIDE_INT == BITS_PER_WORD)
	    || flag_pretend_float)
	   && (GET_MODE_CLASS (mode) == MODE_INT
	       || GET_MODE_CLASS (mode) == MODE_PARTIAL_INT)
	   && GET_CODE (x) == CONST_DOUBLE
	   && GET_MODE_CLASS (GET_MODE (x)) == MODE_FLOAT
	   && GET_MODE_BITSIZE (mode) == BITS_PER_WORD)
    return constant_subword (x, (offset / UNITS_PER_WORD), GET_MODE (x));

  /* Similarly, if this is converting a floating-point value into a
     two-word integer, we can do this one word at a time and make an
     integer.  Only do this is the host and target parameters are
     compatible.  */

  else if (((HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT
	     && HOST_BITS_PER_WIDE_INT == BITS_PER_WORD)
	    || flag_pretend_float)
	   && (GET_MODE_CLASS (mode) == MODE_INT
	       || GET_MODE_CLASS (mode) == MODE_PARTIAL_INT)
	   && GET_CODE (x) == CONST_DOUBLE
	   && GET_MODE_CLASS (GET_MODE (x)) == MODE_FLOAT
	   && GET_MODE_BITSIZE (mode) == 2 * BITS_PER_WORD)
    {
      rtx lowpart, highpart;

      lowpart = constant_subword (x,
				  (offset / UNITS_PER_WORD) + WORDS_BIG_ENDIAN,
				  GET_MODE (x));
      highpart = constant_subword (x,
				   (offset / UNITS_PER_WORD) + (! WORDS_BIG_ENDIAN),
				   GET_MODE (x));
      if (lowpart && GET_CODE (lowpart) == CONST_INT
	  && highpart && GET_CODE (highpart) == CONST_INT)
	return immed_double_const (INTVAL (lowpart), INTVAL (highpart), mode);
    }
#else /* ifndef REAL_ARITHMETIC */

  /* When we have a FP emulator, we can handle all conversions between
     FP and integer operands.  This simplifies reload because it
     doesn't have to deal with constructs like (subreg:DI
     (const_double:SF ...)) or (subreg:DF (const_int ...)).  */
  /* Single-precision floats are always 32-bits and double-precision
     floats are always 64-bits.  */

  else if (GET_MODE_CLASS (mode) == MODE_FLOAT
	   && GET_MODE_BITSIZE (mode) == 32
	   && GET_CODE (x) == CONST_INT)
  {
      REAL_VALUE_TYPE r;
      HOST_WIDE_INT i;

      i = INTVAL (x);
      r = REAL_VALUE_FROM_TARGET_SINGLE (i);
      return CONST_DOUBLE_FROM_REAL_VALUE (r, mode);
  }
  else if (GET_MODE_CLASS (mode) == MODE_FLOAT
	   && GET_MODE_BITSIZE (mode) == 64
	   && (GET_CODE (x) == CONST_INT || GET_CODE (x) == CONST_DOUBLE)
	   && GET_MODE (x) == VOIDmode)
    {
      REAL_VALUE_TYPE r;
      HOST_WIDE_INT i[2];
      HOST_WIDE_INT low, high;

      if (GET_CODE (x) == CONST_INT)
	{
	  low = INTVAL (x);
	  high = low >> (HOST_BITS_PER_WIDE_INT - 1);
	}
      else
	{
	  low = CONST_DOUBLE_LOW (x);
	  high = CONST_DOUBLE_HIGH (x);
	}

#if HOST_BITS_PER_WIDE_INT == 32
      /* REAL_VALUE_TARGET_DOUBLE takes the addressing order of the
	 target machine.  */
      if (WORDS_BIG_ENDIAN)
	i[0] = high, i[1] = low;
      else
	i[0] = low, i[1] = high;
#else
      i[0] = low;
#endif

      r = REAL_VALUE_FROM_TARGET_DOUBLE (i);
      return CONST_DOUBLE_FROM_REAL_VALUE (r, mode);
    }
  else if ((GET_MODE_CLASS (mode) == MODE_INT
	    || GET_MODE_CLASS (mode) == MODE_PARTIAL_INT)
	   && GET_CODE (x) == CONST_DOUBLE
	   && GET_MODE_CLASS (GET_MODE (x)) == MODE_FLOAT)
    {
      REAL_VALUE_TYPE r;
      long i[4];  /* Only the low 32 bits of each 'long' are used.  */
      int endian = WORDS_BIG_ENDIAN ? 1 : 0;

      /* Convert 'r' into an array of four 32-bit words in target word
         order.  */
      REAL_VALUE_FROM_CONST_DOUBLE (r, x);
      switch (GET_MODE_BITSIZE (GET_MODE (x)))
	{
	case 32:
          REAL_VALUE_TO_TARGET_SINGLE (r, i[3 * endian]);
	  i[1] = 0;
	  i[2] = 0;
          i[3 - 3 * endian] = 0;
          break;
	case 64:
          REAL_VALUE_TO_TARGET_DOUBLE (r, i + 2 * endian);
	  i[2 - 2 * endian] = 0;
	  i[3 - 2 * endian] = 0;
          break;
	case 96:
	  REAL_VALUE_TO_TARGET_LONG_DOUBLE (r, i + endian);
	  i[3 - 3 * endian] = 0;
	  break;
	case 128:
	  REAL_VALUE_TO_TARGET_LONG_DOUBLE (r, i);
	  break;
	default:
	  abort ();
	}
      /* Now, pack the 32-bit elements of the array into a CONST_DOUBLE
	 and return it.  */
#if HOST_BITS_PER_WIDE_INT == 32
      return immed_double_const (i[3 * endian], i[1 + endian], mode);
#else
      if (HOST_BITS_PER_WIDE_INT != 64)
	abort ();

      return immed_double_const ((((unsigned long) i[3 * endian])
				  | ((HOST_WIDE_INT) i[1 + endian] << 32)),
				 (((unsigned long) i[2 - endian])
				  | ((HOST_WIDE_INT) i[3 - 3 * endian] << 32)),
				 mode);
#endif
    }
#endif /* ifndef REAL_ARITHMETIC */

  /* Otherwise, we can't do this.  */
  return 0;
}

/* Return the real part (which has mode MODE) of a complex value X.
   This always comes at the low address in memory.  */

rtx
gen_realpart (mode, x)
     enum machine_mode mode;
     rtx x;
{
  if (WORDS_BIG_ENDIAN
      && GET_MODE_BITSIZE (mode) < BITS_PER_WORD
      && REG_P (x)
      && REGNO (x) < FIRST_PSEUDO_REGISTER)
    internal_error
      ("can't access real part of complex value in hard register");
  else if (WORDS_BIG_ENDIAN)
    return gen_highpart (mode, x);
  else
    return gen_lowpart (mode, x);
}

/* Return the imaginary part (which has mode MODE) of a complex value X.
   This always comes at the high address in memory.  */

rtx
gen_imagpart (mode, x)
     enum machine_mode mode;
     rtx x;
{
  if (WORDS_BIG_ENDIAN)
    return gen_lowpart (mode, x);
  else if (! WORDS_BIG_ENDIAN
	   && GET_MODE_BITSIZE (mode) < BITS_PER_WORD
	   && REG_P (x)
	   && REGNO (x) < FIRST_PSEUDO_REGISTER)
    internal_error
      ("can't access imaginary part of complex value in hard register");
  else
    return gen_highpart (mode, x);
}

/* Return 1 iff X, assumed to be a SUBREG,
   refers to the real part of the complex value in its containing reg.
   Complex values are always stored with the real part in the first word,
   regardless of WORDS_BIG_ENDIAN.  */

int
subreg_realpart_p (x)
     rtx x;
{
  if (GET_CODE (x) != SUBREG)
    abort ();

  return ((unsigned int) SUBREG_BYTE (x)
	  < GET_MODE_UNIT_SIZE (GET_MODE (SUBREG_REG (x))));
}

/* Assuming that X is an rtx (e.g., MEM, REG or SUBREG) for a value,
   return an rtx (MEM, SUBREG, or CONST_INT) that refers to the
   least-significant part of X.
   MODE specifies how big a part of X to return;
   it usually should not be larger than a word.
   If X is a MEM whose address is a QUEUED, the value may be so also.  */

rtx
gen_lowpart (mode, x)
     enum machine_mode mode;
     rtx x;
{
  rtx result = gen_lowpart_common (mode, x);

  if (result)
    return result;
  else if (GET_CODE (x) == REG)
    {
      /* Must be a hard reg that's not valid in MODE.  */
      result = gen_lowpart_common (mode, copy_to_reg (x));
      if (result == 0)
	abort ();
      return result;
    }
  else if (GET_CODE (x) == MEM)
    {
      /* The only additional case we can do is MEM.  */
      int offset = 0;
      if (WORDS_BIG_ENDIAN)
	offset = (MAX (GET_MODE_SIZE (GET_MODE (x)), UNITS_PER_WORD)
		  - MAX (GET_MODE_SIZE (mode), UNITS_PER_WORD));

      if (BYTES_BIG_ENDIAN)
	/* Adjust the address so that the address-after-the-data
	   is unchanged.  */
	offset -= (MIN (UNITS_PER_WORD, GET_MODE_SIZE (mode))
		   - MIN (UNITS_PER_WORD, GET_MODE_SIZE (GET_MODE (x))));

      return adjust_address (x, mode, offset);
    }
  else if (GET_CODE (x) == ADDRESSOF)
    return gen_lowpart (mode, force_reg (GET_MODE (x), x));
  else
    abort ();
}

/* Like `gen_lowpart', but refer to the most significant part.
   This is used to access the imaginary part of a complex number.  */

rtx
gen_highpart (mode, x)
     enum machine_mode mode;
     rtx x;
{
  unsigned int msize = GET_MODE_SIZE (mode);
  rtx result;

  /* This case loses if X is a subreg.  To catch bugs early,
     complain if an invalid MODE is used even in other cases.  */
  if (msize > UNITS_PER_WORD
      && msize != GET_MODE_UNIT_SIZE (GET_MODE (x)))
    abort ();

  result = simplify_gen_subreg (mode, x, GET_MODE (x),
				subreg_highpart_offset (mode, GET_MODE (x)));

  /* simplify_gen_subreg is not guaranteed to return a valid operand for
     the target if we have a MEM.  gen_highpart must return a valid operand,
     emitting code if necessary to do so.  */
  if (result != NULL_RTX && GET_CODE (result) == MEM)
    result = validize_mem (result);

  if (!result)
    abort ();
  return result;
}

/* Like gen_highpart_mode, but accept mode of EXP operand in case EXP can
   be VOIDmode constant.  */
rtx
gen_highpart_mode (outermode, innermode, exp)
    enum machine_mode outermode, innermode;
    rtx exp;
{
  if (GET_MODE (exp) != VOIDmode)
    {
      if (GET_MODE (exp) != innermode)
	abort ();
      return gen_highpart (outermode, exp);
    }
  return simplify_gen_subreg (outermode, exp, innermode,
			      subreg_highpart_offset (outermode, innermode));
}
/* Return offset in bytes to get OUTERMODE low part
   of the value in mode INNERMODE stored in memory in target format.  */

unsigned int
subreg_lowpart_offset (outermode, innermode)
     enum machine_mode outermode, innermode;
{
  unsigned int offset = 0;
  int difference = (GET_MODE_SIZE (innermode) - GET_MODE_SIZE (outermode));

  if (difference > 0)
    {
      if (WORDS_BIG_ENDIAN)
	offset += (difference / UNITS_PER_WORD) * UNITS_PER_WORD;
      if (BYTES_BIG_ENDIAN)
	offset += difference % UNITS_PER_WORD;
    }

  return offset;
}

/* Return offset in bytes to get OUTERMODE high part
   of the value in mode INNERMODE stored in memory in target format.  */
unsigned int
subreg_highpart_offset (outermode, innermode)
     enum machine_mode outermode, innermode;
{
  unsigned int offset = 0;
  int difference = (GET_MODE_SIZE (innermode) - GET_MODE_SIZE (outermode));

  if (GET_MODE_SIZE (innermode) < GET_MODE_SIZE (outermode))
     abort ();

  if (difference > 0)
    {
      if (! WORDS_BIG_ENDIAN)
	offset += (difference / UNITS_PER_WORD) * UNITS_PER_WORD;
      if (! BYTES_BIG_ENDIAN)
	offset += difference % UNITS_PER_WORD;
    }

  return offset;
}

/* Return 1 iff X, assumed to be a SUBREG,
   refers to the least significant part of its containing reg.
   If X is not a SUBREG, always return 1 (it is its own low part!).  */

int
subreg_lowpart_p (x)
     rtx x;
{
  if (GET_CODE (x) != SUBREG)
    return 1;
  else if (GET_MODE (SUBREG_REG (x)) == VOIDmode)
    return 0;

  return (subreg_lowpart_offset (GET_MODE (x), GET_MODE (SUBREG_REG (x)))
	  == SUBREG_BYTE (x));
}


/* Helper routine for all the constant cases of operand_subword.
   Some places invoke this directly.  */

rtx
constant_subword (op, offset, mode)
     rtx op;
     int offset;
     enum machine_mode mode;
{
  int size_ratio = HOST_BITS_PER_WIDE_INT / BITS_PER_WORD;
  HOST_WIDE_INT val;

  /* If OP is already an integer word, return it.  */
  if (GET_MODE_CLASS (mode) == MODE_INT
      && GET_MODE_SIZE (mode) == UNITS_PER_WORD)
    return op;

#ifdef REAL_ARITHMETIC
  /* The output is some bits, the width of the target machine's word.
     A wider-word host can surely hold them in a CONST_INT. A narrower-word
     host can't.  */
  if (HOST_BITS_PER_WIDE_INT >= BITS_PER_WORD
      && GET_MODE_CLASS (mode) == MODE_FLOAT
      && GET_MODE_BITSIZE (mode) == 64
      && GET_CODE (op) == CONST_DOUBLE)
    {
      long k[2];
      REAL_VALUE_TYPE rv;

      REAL_VALUE_FROM_CONST_DOUBLE (rv, op);
      REAL_VALUE_TO_TARGET_DOUBLE (rv, k);

      /* We handle 32-bit and >= 64-bit words here.  Note that the order in
	 which the words are written depends on the word endianness.
	 ??? This is a potential portability problem and should
	 be fixed at some point.

	 We must exercise caution with the sign bit.  By definition there
	 are 32 significant bits in K; there may be more in a HOST_WIDE_INT.
	 Consider a host with a 32-bit long and a 64-bit HOST_WIDE_INT.
	 So we explicitly mask and sign-extend as necessary.  */
      if (BITS_PER_WORD == 32)
	{
	  val = k[offset];
	  val = ((val & 0xffffffff) ^ 0x80000000) - 0x80000000;
	  return GEN_INT (val);
	}
#if HOST_BITS_PER_WIDE_INT >= 64
      else if (BITS_PER_WORD >= 64 && offset == 0)
	{
	  val = k[! WORDS_BIG_ENDIAN];
	  val = (((val & 0xffffffff) ^ 0x80000000) - 0x80000000) << 32;
	  val |= (HOST_WIDE_INT) k[WORDS_BIG_ENDIAN] & 0xffffffff;
	  return GEN_INT (val);
	}
#endif
      else if (BITS_PER_WORD == 16)
	{
	  val = k[offset >> 1];
	  if ((offset & 1) == ! WORDS_BIG_ENDIAN)
	    val >>= 16;
	  val = ((val & 0xffff) ^ 0x8000) - 0x8000;
	  return GEN_INT (val);
	}
      else
	abort ();
    }
  else if (HOST_BITS_PER_WIDE_INT >= BITS_PER_WORD
	   && GET_MODE_CLASS (mode) == MODE_FLOAT
	   && GET_MODE_BITSIZE (mode) > 64
	   && GET_CODE (op) == CONST_DOUBLE)
    {
      long k[4];
      REAL_VALUE_TYPE rv;

      REAL_VALUE_FROM_CONST_DOUBLE (rv, op);
      REAL_VALUE_TO_TARGET_LONG_DOUBLE (rv, k);

      if (BITS_PER_WORD == 32)
	{
	  val = k[offset];
	  val = ((val & 0xffffffff) ^ 0x80000000) - 0x80000000;
	  return GEN_INT (val);
	}
#if HOST_BITS_PER_WIDE_INT >= 64
      else if (BITS_PER_WORD >= 64 && offset <= 1)
	{
	  val = k[offset * 2 + ! WORDS_BIG_ENDIAN];
	  val = (((val & 0xffffffff) ^ 0x80000000) - 0x80000000) << 32;
	  val |= (HOST_WIDE_INT) k[offset * 2 + WORDS_BIG_ENDIAN] & 0xffffffff;
	  return GEN_INT (val);
	}
#endif
      else
	abort ();
    }
#else /* no REAL_ARITHMETIC */
  if (((HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT
	&& HOST_BITS_PER_WIDE_INT == BITS_PER_WORD)
       || flag_pretend_float)
      && GET_MODE_CLASS (mode) == MODE_FLOAT
      && GET_MODE_SIZE (mode) == 2 * UNITS_PER_WORD
      && GET_CODE (op) == CONST_DOUBLE)
    {
      /* The constant is stored in the host's word-ordering,
	 but we want to access it in the target's word-ordering.  Some
	 compilers don't like a conditional inside macro args, so we have two
	 copies of the return.  */
#ifdef HOST_WORDS_BIG_ENDIAN
      return GEN_INT (offset == WORDS_BIG_ENDIAN
		      ? CONST_DOUBLE_HIGH (op) : CONST_DOUBLE_LOW (op));
#else
      return GEN_INT (offset != WORDS_BIG_ENDIAN
		      ? CONST_DOUBLE_HIGH (op) : CONST_DOUBLE_LOW (op));
#endif
    }
#endif /* no REAL_ARITHMETIC */

  /* Single word float is a little harder, since single- and double-word
     values often do not have the same high-order bits.  We have already
     verified that we want the only defined word of the single-word value.  */
#ifdef REAL_ARITHMETIC
  if (GET_MODE_CLASS (mode) == MODE_FLOAT
      && GET_MODE_BITSIZE (mode) == 32
      && GET_CODE (op) == CONST_DOUBLE)
    {
      long l;
      REAL_VALUE_TYPE rv;

      REAL_VALUE_FROM_CONST_DOUBLE (rv, op);
      REAL_VALUE_TO_TARGET_SINGLE (rv, l);

      /* Sign extend from known 32-bit value to HOST_WIDE_INT.  */
      val = l;
      val = ((val & 0xffffffff) ^ 0x80000000) - 0x80000000;

      if (BITS_PER_WORD == 16)
	{
	  if ((offset & 1) == ! WORDS_BIG_ENDIAN)
	    val >>= 16;
	  val = ((val & 0xffff) ^ 0x8000) - 0x8000;
	}

      return GEN_INT (val);
    }
#else
  if (((HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT
	&& HOST_BITS_PER_WIDE_INT == BITS_PER_WORD)
       || flag_pretend_float)
      && sizeof (float) * 8 == HOST_BITS_PER_WIDE_INT
      && GET_MODE_CLASS (mode) == MODE_FLOAT
      && GET_MODE_SIZE (mode) == UNITS_PER_WORD
      && GET_CODE (op) == CONST_DOUBLE)
    {
      double d;
      union {float f; HOST_WIDE_INT i; } u;

      REAL_VALUE_FROM_CONST_DOUBLE (d, op);

      u.f = d;
      return GEN_INT (u.i);
    }
  if (((HOST_FLOAT_FORMAT == TARGET_FLOAT_FORMAT
	&& HOST_BITS_PER_WIDE_INT == BITS_PER_WORD)
       || flag_pretend_float)
      && sizeof (double) * 8 == HOST_BITS_PER_WIDE_INT
      && GET_MODE_CLASS (mode) == MODE_FLOAT
      && GET_MODE_SIZE (mode) == UNITS_PER_WORD
      && GET_CODE (op) == CONST_DOUBLE)
    {
      double d;
      union {double d; HOST_WIDE_INT i; } u;

      REAL_VALUE_FROM_CONST_DOUBLE (d, op);

      u.d = d;
      return GEN_INT (u.i);
    }
#endif /* no REAL_ARITHMETIC */

  /* The only remaining cases that we can handle are integers.
     Convert to proper endianness now since these cases need it.
     At this point, offset == 0 means the low-order word.

     We do not want to handle the case when BITS_PER_WORD <= HOST_BITS_PER_INT
     in general.  However, if OP is (const_int 0), we can just return
     it for any word.  */

  if (op == const0_rtx)
    return op;

  if (GET_MODE_CLASS (mode) != MODE_INT
      || (GET_CODE (op) != CONST_INT && GET_CODE (op) != CONST_DOUBLE)
      || BITS_PER_WORD > HOST_BITS_PER_WIDE_INT)
    return 0;

  if (WORDS_BIG_ENDIAN)
    offset = GET_MODE_SIZE (mode) / UNITS_PER_WORD - 1 - offset;

  /* Find out which word on the host machine this value is in and get
     it from the constant.  */
  val = (offset / size_ratio == 0
	 ? (GET_CODE (op) == CONST_INT ? INTVAL (op) : CONST_DOUBLE_LOW (op))
	 : (GET_CODE (op) == CONST_INT
	    ? (INTVAL (op) < 0 ? ~0 : 0) : CONST_DOUBLE_HIGH (op)));

  /* Get the value we want into the low bits of val.  */
  if (BITS_PER_WORD < HOST_BITS_PER_WIDE_INT)
    val = ((val >> ((offset % size_ratio) * BITS_PER_WORD)));

  val = trunc_int_for_mode (val, word_mode);

  return GEN_INT (val);
}

/* Return subword OFFSET of operand OP.
   The word number, OFFSET, is interpreted as the word number starting
   at the low-order address.  OFFSET 0 is the low-order word if not
   WORDS_BIG_ENDIAN, otherwise it is the high-order word.

   If we cannot extract the required word, we return zero.  Otherwise,
   an rtx corresponding to the requested word will be returned.

   VALIDATE_ADDRESS is nonzero if the address should be validated.  Before
   reload has completed, a valid address will always be returned.  After
   reload, if a valid address cannot be returned, we return zero.

   If VALIDATE_ADDRESS is zero, we simply form the required address; validating
   it is the responsibility of the caller.

   MODE is the mode of OP in case it is a CONST_INT.

   ??? This is still rather broken for some cases.  The problem for the
   moment is that all callers of this thing provide no 'goal mode' to
   tell us to work with.  This exists because all callers were written
   in a word based SUBREG world.
   Now use of this function can be deprecated by simplify_subreg in most
   cases.
 */

rtx
operand_subword (op, offset, validate_address, mode)
     rtx op;
     unsigned int offset;
     int validate_address;
     enum machine_mode mode;
{
  if (mode == VOIDmode)
    mode = GET_MODE (op);

  if (mode == VOIDmode)
    abort ();

  /* If OP is narrower than a word, fail.  */
  if (mode != BLKmode
      && (GET_MODE_SIZE (mode) < UNITS_PER_WORD))
    return 0;

  /* If we want a word outside OP, return zero.  */
  if (mode != BLKmode
      && (offset + 1) * UNITS_PER_WORD > GET_MODE_SIZE (mode))
    return const0_rtx;

  /* Form a new MEM at the requested address.  */
  if (GET_CODE (op) == MEM)
    {
      rtx new = adjust_address_nv (op, word_mode, offset * UNITS_PER_WORD);

      if (! validate_address)
	return new;

      else if (reload_completed)
	{
	  if (! strict_memory_address_p (word_mode, XEXP (new, 0)))
	    return 0;
	}
      else
	return replace_equiv_address (new, XEXP (new, 0));
    }

  /* Rest can be handled by simplify_subreg.  */
  return simplify_gen_subreg (word_mode, op, mode, (offset * UNITS_PER_WORD));
}

/* Similar to `operand_subword', but never return 0.  If we can't extract
   the required subword, put OP into a register and try again.  If that fails,
   abort.  We always validate the address in this case.

   MODE is the mode of OP, in case it is CONST_INT.  */

rtx
operand_subword_force (op, offset, mode)
     rtx op;
     unsigned int offset;
     enum machine_mode mode;
{
  rtx result = operand_subword (op, offset, 1, mode);

  if (result)
    return result;

  if (mode != BLKmode && mode != VOIDmode)
    {
      /* If this is a register which can not be accessed by words, copy it
	 to a pseudo register.  */
      if (GET_CODE (op) == REG)
	op = copy_to_reg (op);
      else
	op = force_reg (mode, op);
    }

  result = operand_subword (op, offset, 1, mode);
  if (result == 0)
    abort ();

  return result;
}

/* Given a compare instruction, swap the operands.
   A test instruction is changed into a compare of 0 against the operand.  */

void
reverse_comparison (insn)
     rtx insn;
{
  rtx body = PATTERN (insn);
  rtx comp;

  if (GET_CODE (body) == SET)
    comp = SET_SRC (body);
  else
    comp = SET_SRC (XVECEXP (body, 0, 0));

  if (GET_CODE (comp) == COMPARE)
    {
      rtx op0 = XEXP (comp, 0);
      rtx op1 = XEXP (comp, 1);
      XEXP (comp, 0) = op1;
      XEXP (comp, 1) = op0;
    }
  else
    {
      rtx new = gen_rtx_COMPARE (VOIDmode,
				 CONST0_RTX (GET_MODE (comp)), comp);
      if (GET_CODE (body) == SET)
	SET_SRC (body) = new;
      else
	SET_SRC (XVECEXP (body, 0, 0)) = new;
    }
}

/* Within a MEM_EXPR, we care about either (1) a component ref of a decl,
   or (2) a component ref of something variable.  Represent the later with
   a NULL expression.  */

static tree
component_ref_for_mem_expr (ref)
     tree ref;
{
  tree inner = TREE_OPERAND (ref, 0);

  if (TREE_CODE (inner) == COMPONENT_REF)
    inner = component_ref_for_mem_expr (inner);
  else
    {
      tree placeholder_ptr = 0;

      /* Now remove any conversions: they don't change what the underlying
	 object is.  Likewise for SAVE_EXPR.  Also handle PLACEHOLDER_EXPR.  */
      while (TREE_CODE (inner) == NOP_EXPR || TREE_CODE (inner) == CONVERT_EXPR
	     || TREE_CODE (inner) == NON_LVALUE_EXPR
	     || TREE_CODE (inner) == VIEW_CONVERT_EXPR
	     || TREE_CODE (inner) == SAVE_EXPR
	     || TREE_CODE (inner) == PLACEHOLDER_EXPR)
	  if (TREE_CODE (inner) == PLACEHOLDER_EXPR)
	    inner = find_placeholder (inner, &placeholder_ptr);
	  else
	    inner = TREE_OPERAND (inner, 0);

      if (! DECL_P (inner))
	inner = NULL_TREE;
    }

  if (inner == TREE_OPERAND (ref, 0))
    return ref;
  else
    return build (COMPONENT_REF, TREE_TYPE (ref), inner,
		  TREE_OPERAND (ref, 1));
}

/* Given REF, a MEM, and T, either the type of X or the expression
   corresponding to REF, set the memory attributes.  OBJECTP is nonzero
   if we are making a new object of this type.  */

void
set_mem_attributes (ref, t, objectp)
     rtx ref;
     tree t;
     int objectp;
{
  HOST_WIDE_INT alias = MEM_ALIAS_SET (ref);
  tree expr = MEM_EXPR (ref);
  rtx offset = MEM_OFFSET (ref);
  rtx size = MEM_SIZE (ref);
  unsigned int align = MEM_ALIGN (ref);
  tree type;

  /* It can happen that type_for_mode was given a mode for which there
     is no language-level type.  In which case it returns NULL, which
     we can see here.  */
  if (t == NULL_TREE)
    return;

  type = TYPE_P (t) ? t : TREE_TYPE (t);

  /* If we have already set DECL_RTL = ref, get_alias_set will get the
     wrong answer, as it assumes that DECL_RTL already has the right alias
     info.  Callers should not set DECL_RTL until after the call to
     set_mem_attributes.  */
  if (DECL_P (t) && ref == DECL_RTL_IF_SET (t))
    abort ();

  /* Get the alias set from the expression or type (perhaps using a
     front-end routine) and use it.  */
  alias = get_alias_set (t);

  MEM_VOLATILE_P (ref) = TYPE_VOLATILE (type);
  MEM_IN_STRUCT_P (ref) = AGGREGATE_TYPE_P (type);
  RTX_UNCHANGING_P (ref)
    |= ((lang_hooks.honor_readonly
	 && (TYPE_READONLY (type) || TREE_READONLY (t)))
	|| (! TYPE_P (t) && TREE_CONSTANT (t)));

  /* If we are making an object of this type, or if this is a DECL, we know
     that it is a scalar if the type is not an aggregate.  */
  if ((objectp || DECL_P (t)) && ! AGGREGATE_TYPE_P (type))
    MEM_SCALAR_P (ref) = 1;

  /* We can set the alignment from the type if we are making an object,
     this is an INDIRECT_REF, or if TYPE_ALIGN_OK.  */
  if (objectp || TREE_CODE (t) == INDIRECT_REF || TYPE_ALIGN_OK (type))
    align = MAX (align, TYPE_ALIGN (type));

  /* If the size is known, we can set that.  */
  if (TYPE_SIZE_UNIT (type) && host_integerp (TYPE_SIZE_UNIT (type), 1))
    size = GEN_INT (tree_low_cst (TYPE_SIZE_UNIT (type), 1));

  /* If T is not a type, we may be able to deduce some more information about
     the expression.  */
  if (! TYPE_P (t))
    {
      maybe_set_unchanging (ref, t);
      if (TREE_THIS_VOLATILE (t))
	MEM_VOLATILE_P (ref) = 1;

      /* Now remove any conversions: they don't change what the underlying
	 object is.  Likewise for SAVE_EXPR.  */
      while (TREE_CODE (t) == NOP_EXPR || TREE_CODE (t) == CONVERT_EXPR
	     || TREE_CODE (t) == NON_LVALUE_EXPR
	     || TREE_CODE (t) == VIEW_CONVERT_EXPR
	     || TREE_CODE (t) == SAVE_EXPR)
	t = TREE_OPERAND (t, 0);

      /* If this expression can't be addressed (e.g., it contains a reference
	 to a non-addressable field), show we don't change its alias set.  */
      if (! can_address_p (t))
	MEM_KEEP_ALIAS_SET_P (ref) = 1;

      /* If this is a decl, set the attributes of the MEM from it.  */
      if (DECL_P (t))
	{
	  expr = t;
	  offset = const0_rtx;
	  size = (DECL_SIZE_UNIT (t)
		  && host_integerp (DECL_SIZE_UNIT (t), 1)
		  ? GEN_INT (tree_low_cst (DECL_SIZE_UNIT (t), 1)) : 0);
	  align =  DECL_ALIGN (t);
	}

      /* If this is a constant, we know the alignment.  */
      else if (TREE_CODE_CLASS (TREE_CODE (t)) == 'c')
	{
	  align = TYPE_ALIGN (type);
#ifdef CONSTANT_ALIGNMENT
	  align = CONSTANT_ALIGNMENT (t, align);
#endif
	}

      /* If this is a field reference and not a bit-field, record it.  */
      /* ??? There is some information that can be gleened from bit-fields,
	 such as the word offset in the structure that might be modified.
	 But skip it for now.  */
      else if (TREE_CODE (t) == COMPONENT_REF
	       && ! DECL_BIT_FIELD (TREE_OPERAND (t, 1)))
	{
	  expr = component_ref_for_mem_expr (t);
	  offset = const0_rtx;
	  /* ??? Any reason the field size would be different than
	     the size we got from the type?  */
	}

      /* If this is an array reference, look for an outer field reference.  */
      else if (TREE_CODE (t) == ARRAY_REF)
	{
	  tree off_tree = size_zero_node;

	  do
	    {
	      off_tree
		= fold (build (PLUS_EXPR, sizetype,
			       fold (build (MULT_EXPR, sizetype,
					    TREE_OPERAND (t, 1),
					    TYPE_SIZE_UNIT (TREE_TYPE (t)))),
			       off_tree));
	      t = TREE_OPERAND (t, 0);
	    }
	  while (TREE_CODE (t) == ARRAY_REF);

	  if (TREE_CODE (t) == COMPONENT_REF)
	    {
	      expr = component_ref_for_mem_expr (t);
	      if (host_integerp (off_tree, 1))
		offset = GEN_INT (tree_low_cst (off_tree, 1));
	      /* ??? Any reason the field size would be different than
		 the size we got from the type?  */
	    }
	}
    }

  /* Now set the attributes we computed above.  */
  MEM_ATTRS (ref)
    = get_mem_attrs (alias, expr, offset, size, align, GET_MODE (ref));

  /* If this is already known to be a scalar or aggregate, we are done.  */
  if (MEM_IN_STRUCT_P (ref) || MEM_SCALAR_P (ref))
    return;

  /* If it is a reference into an aggregate, this is part of an aggregate.
     Otherwise we don't know.  */
  else if (TREE_CODE (t) == COMPONENT_REF || TREE_CODE (t) == ARRAY_REF
	   || TREE_CODE (t) == ARRAY_RANGE_REF
	   || TREE_CODE (t) == BIT_FIELD_REF)
    MEM_IN_STRUCT_P (ref) = 1;
}

/* Set the alias set of MEM to SET.  */

void
set_mem_alias_set (mem, set)
     rtx mem;
     HOST_WIDE_INT set;
{
#ifdef ENABLE_CHECKING	
  /* If the new and old alias sets don't conflict, something is wrong.  */
  if (!alias_sets_conflict_p (set, MEM_ALIAS_SET (mem)))
    abort ();
#endif

  MEM_ATTRS (mem) = get_mem_attrs (set, MEM_EXPR (mem), MEM_OFFSET (mem),
				   MEM_SIZE (mem), MEM_ALIGN (mem),
				   GET_MODE (mem));
}

/* Set the alignment of MEM to ALIGN bits.  */

void
set_mem_align (mem, align)
     rtx mem;
     unsigned int align;
{
  MEM_ATTRS (mem) = get_mem_attrs (MEM_ALIAS_SET (mem), MEM_EXPR (mem),
				   MEM_OFFSET (mem), MEM_SIZE (mem), align,
				   GET_MODE (mem));
}

/* Set the expr for MEM to EXPR.  */

void
set_mem_expr (mem, expr)
     rtx mem;
     tree expr;
{
  MEM_ATTRS (mem)
    = get_mem_attrs (MEM_ALIAS_SET (mem), expr, MEM_OFFSET (mem),
		     MEM_SIZE (mem), MEM_ALIGN (mem), GET_MODE (mem));
}

/* Set the offset of MEM to OFFSET.  */

void
set_mem_offset (mem, offset)
     rtx mem, offset;
{
  MEM_ATTRS (mem) = get_mem_attrs (MEM_ALIAS_SET (mem), MEM_EXPR (mem),
				   offset, MEM_SIZE (mem), MEM_ALIGN (mem),
				   GET_MODE (mem));
}

/* Return a memory reference like MEMREF, but with its mode changed to MODE
   and its address changed to ADDR.  (VOIDmode means don't change the mode.
   NULL for ADDR means don't change the address.)  VALIDATE is nonzero if the
   returned memory location is required to be valid.  The memory
   attributes are not changed.  */

static rtx
change_address_1 (memref, mode, addr, validate)
     rtx memref;
     enum machine_mode mode;
     rtx addr;
     int validate;
{
  rtx new;

  if (GET_CODE (memref) != MEM)
    abort ();
  if (mode == VOIDmode)
    mode = GET_MODE (memref);
  if (addr == 0)
    addr = XEXP (memref, 0);

  if (validate)
    {
      if (reload_in_progress || reload_completed)
	{
	  if (! memory_address_p (mode, addr))
	    abort ();
	}
      else
	addr = memory_address (mode, addr);
    }

  if (rtx_equal_p (addr, XEXP (memref, 0)) && mode == GET_MODE (memref))
    return memref;

  new = gen_rtx_MEM (mode, addr);
  MEM_COPY_ATTRIBUTES (new, memref);
  return new;
}

/* Like change_address_1 with VALIDATE nonzero, but we are not saying in what
   way we are changing MEMREF, so we only preserve the alias set.  */

rtx
change_address (memref, mode, addr)
     rtx memref;
     enum machine_mode mode;
     rtx addr;
{
  rtx new = change_address_1 (memref, mode, addr, 1);
  enum machine_mode mmode = GET_MODE (new);

  MEM_ATTRS (new)
    = get_mem_attrs (MEM_ALIAS_SET (memref), 0, 0,
		     mmode == BLKmode ? 0 : GEN_INT (GET_MODE_SIZE (mmode)),
		     (mmode == BLKmode ? BITS_PER_UNIT
		      : GET_MODE_ALIGNMENT (mmode)),
		     mmode);

  return new;
}

/* Return a memory reference like MEMREF, but with its mode changed
   to MODE and its address offset by OFFSET bytes.  If VALIDATE is
   nonzero, the memory address is forced to be valid.
   If ADJUST is zero, OFFSET is only used to update MEM_ATTRS
   and caller is responsible for adjusting MEMREF base register.  */

rtx
adjust_address_1 (memref, mode, offset, validate, adjust)
     rtx memref;
     enum machine_mode mode;
     HOST_WIDE_INT offset;
     int validate, adjust;
{
  rtx addr = XEXP (memref, 0);
  rtx new;
  rtx memoffset = MEM_OFFSET (memref);
  rtx size = 0;
  unsigned int memalign = MEM_ALIGN (memref);

  /* ??? Prefer to create garbage instead of creating shared rtl.
     This may happen even if offset is non-zero -- consider
     (plus (plus reg reg) const_int) -- so do this always.  */
  addr = copy_rtx (addr);

  if (adjust)
    {
      /* If MEMREF is a LO_SUM and the offset is within the alignment of the
	 object, we can merge it into the LO_SUM.  */
      if (GET_MODE (memref) != BLKmode && GET_CODE (addr) == LO_SUM
	  && offset >= 0
	  && (unsigned HOST_WIDE_INT) offset
	      < GET_MODE_ALIGNMENT (GET_MODE (memref)) / BITS_PER_UNIT)
	addr = gen_rtx_LO_SUM (Pmode, XEXP (addr, 0),
			       plus_constant (XEXP (addr, 1), offset));
      else
	addr = plus_constant (addr, offset);
    }

  new = change_address_1 (memref, mode, addr, validate);

  /* Compute the new values of the memory attributes due to this adjustment.
     We add the offsets and update the alignment.  */
  if (memoffset)
    memoffset = GEN_INT (offset + INTVAL (memoffset));

  /* Compute the new alignment by taking the MIN of the alignment and the
     lowest-order set bit in OFFSET, but don't change the alignment if OFFSET
     if zero.  */
  if (offset != 0)
    memalign
      = MIN (memalign,
	     (unsigned HOST_WIDE_INT) (offset & -offset) * BITS_PER_UNIT);

  /* We can compute the size in a number of ways.  */
  if (GET_MODE (new) != BLKmode)
    size = GEN_INT (GET_MODE_SIZE (GET_MODE (new)));
  else if (MEM_SIZE (memref))
    size = plus_constant (MEM_SIZE (memref), -offset);

  MEM_ATTRS (new) = get_mem_attrs (MEM_ALIAS_SET (memref), MEM_EXPR (memref),
				   memoffset, size, memalign, GET_MODE (new));

  /* At some point, we should validate that this offset is within the object,
     if all the appropriate values are known.  */
  return new;
}

/* Return a memory reference like MEMREF, but with its mode changed
   to MODE and its address changed to ADDR, which is assumed to be
   MEMREF offseted by OFFSET bytes.  If VALIDATE is
   nonzero, the memory address is forced to be valid.  */

rtx
adjust_automodify_address_1 (memref, mode, addr, offset, validate)
     rtx memref;
     enum machine_mode mode;
     rtx addr;
     HOST_WIDE_INT offset;
     int validate;
{
  memref = change_address_1 (memref, VOIDmode, addr, validate);
  return adjust_address_1 (memref, mode, offset, validate, 0);
}

/* Return a memory reference like MEMREF, but whose address is changed by
   adding OFFSET, an RTX, to it.  POW2 is the highest power of two factor
   known to be in OFFSET (possibly 1).  */

rtx
offset_address (memref, offset, pow2)
     rtx memref;
     rtx offset;
     HOST_WIDE_INT pow2;
{
  rtx new, addr = XEXP (memref, 0);

  new = simplify_gen_binary (PLUS, Pmode, addr, offset);

  /* At this point we don't know _why_ the address is invalid.  It 
     could have secondary memory refereces, multiplies or anything.

     However, if we did go and rearrange things, we can wind up not
     being able to recognize the magic around pic_offset_table_rtx.
     This stuff is fragile, and is yet another example of why it is
     bad to expose PIC machinery too early.  */
  if (! memory_address_p (GET_MODE (memref), new)
      && GET_CODE (addr) == PLUS
      && XEXP (addr, 0) == pic_offset_table_rtx)
    {
      addr = force_reg (GET_MODE (addr), addr);
      new = simplify_gen_binary (PLUS, Pmode, addr, offset);
    }

  update_temp_slot_address (XEXP (memref, 0), new);
  new = change_address_1 (memref, VOIDmode, new, 1);

  /* Update the alignment to reflect the offset.  Reset the offset, which
     we don't know.  */
  MEM_ATTRS (new)
    = get_mem_attrs (MEM_ALIAS_SET (memref), MEM_EXPR (memref), 0, 0,
		     MIN (MEM_ALIGN (memref),
			  (unsigned HOST_WIDE_INT) pow2 * BITS_PER_UNIT),
		     GET_MODE (new));
  return new;
}
  
/* Return a memory reference like MEMREF, but with its address changed to
   ADDR.  The caller is asserting that the actual piece of memory pointed
   to is the same, just the form of the address is being changed, such as
   by putting something into a register.  */

rtx
replace_equiv_address (memref, addr)
     rtx memref;
     rtx addr;
{
  /* change_address_1 copies the memory attribute structure without change
     and that's exactly what we want here.  */
  update_temp_slot_address (XEXP (memref, 0), addr);
  return change_address_1 (memref, VOIDmode, addr, 1);
}

/* Likewise, but the reference is not required to be valid.  */

rtx
replace_equiv_address_nv (memref, addr)
     rtx memref;
     rtx addr;
{
  return change_address_1 (memref, VOIDmode, addr, 0);
}

/* Return a memory reference like MEMREF, but with its mode widened to
   MODE and offset by OFFSET.  This would be used by targets that e.g.
   cannot issue QImode memory operations and have to use SImode memory
   operations plus masking logic.  */

rtx
widen_memory_access (memref, mode, offset)
     rtx memref;
     enum machine_mode mode;
     HOST_WIDE_INT offset;
{
  rtx new = adjust_address_1 (memref, mode, offset, 1, 1);
  tree expr = MEM_EXPR (new);
  rtx memoffset = MEM_OFFSET (new);
  unsigned int size = GET_MODE_SIZE (mode);

  /* If we don't know what offset we were at within the expression, then
     we can't know if we've overstepped the bounds.  */
  if (! memoffset)
    expr = NULL_TREE;

  while (expr)
    {
      if (TREE_CODE (expr) == COMPONENT_REF)
	{
	  tree field = TREE_OPERAND (expr, 1);

	  if (! DECL_SIZE_UNIT (field))
	    {
	      expr = NULL_TREE;
	      break;
	    }

	  /* Is the field at least as large as the access?  If so, ok,
	     otherwise strip back to the containing structure.  */
	  if (TREE_CODE (DECL_SIZE_UNIT (field)) == INTEGER_CST
	      && compare_tree_int (DECL_SIZE_UNIT (field), size) >= 0
	      && INTVAL (memoffset) >= 0)
	    break;

	  if (! host_integerp (DECL_FIELD_OFFSET (field), 1))
	    {
	      expr = NULL_TREE;
	      break;
	    }

	  expr = TREE_OPERAND (expr, 0);
	  memoffset = (GEN_INT (INTVAL (memoffset)
		       + tree_low_cst (DECL_FIELD_OFFSET (field), 1)
		       + (tree_low_cst (DECL_FIELD_BIT_OFFSET (field), 1)
		          / BITS_PER_UNIT)));
	}
      /* Similarly for the decl.  */
      else if (DECL_P (expr)
	       && DECL_SIZE_UNIT (expr)
	       && TREE_CODE (DECL_SIZE_UNIT (expr)) == INTEGER_CST
	       && compare_tree_int (DECL_SIZE_UNIT (expr), size) >= 0
	       && (! memoffset || INTVAL (memoffset) >= 0))
	break;
      else
	{
	  /* The widened memory access overflows the expression, which means
	     that it could alias another expression.  Zap it.  */
	  expr = NULL_TREE;
	  break;
	}
    }

  if (! expr)
    memoffset = NULL_RTX;

  /* The widened memory may alias other stuff, so zap the alias set.  */
  /* ??? Maybe use get_alias_set on any remaining expression.  */

  MEM_ATTRS (new) = get_mem_attrs (0, expr, memoffset, GEN_INT (size),
				   MEM_ALIGN (new), mode);

  return new;
}

/* Return a newly created CODE_LABEL rtx with a unique label number.  */

rtx
gen_label_rtx ()
{
  rtx label;

  label = gen_rtx_CODE_LABEL (VOIDmode, 0, NULL_RTX,
			      NULL_RTX, label_num++, NULL, NULL);

  LABEL_NUSES (label) = 0;
  LABEL_ALTERNATE_NAME (label) = NULL;
  return label;
}

/* For procedure integration.  */

/* Install new pointers to the first and last insns in the chain.
   Also, set cur_insn_uid to one higher than the last in use.
   Used for an inline-procedure after copying the insn chain.  */

void
set_new_first_and_last_insn (first, last)
     rtx first, last;
{
  rtx insn;

  first_insn = first;
  last_insn = last;
  cur_insn_uid = 0;

  for (insn = first; insn; insn = NEXT_INSN (insn))
    cur_insn_uid = MAX (cur_insn_uid, INSN_UID (insn));

  cur_insn_uid++;
}

/* Set the range of label numbers found in the current function.
   This is used when belatedly compiling an inline function.  */

void
set_new_first_and_last_label_num (first, last)
     int first, last;
{
  base_label_num = label_num;
  first_label_num = first;
  last_label_num = last;
}

/* Set the last label number found in the current function.
   This is used when belatedly compiling an inline function.  */

void
set_new_last_label_num (last)
     int last;
{
  base_label_num = label_num;
  last_label_num = last;
}

/* Restore all variables describing the current status from the structure *P.
   This is used after a nested function.  */

void
restore_emit_status (p)
     struct function *p ATTRIBUTE_UNUSED;
{
  last_label_num = 0;
  clear_emit_caches ();
}

/* Clear out all parts of the state in F that can safely be discarded
   after the function has been compiled, to let garbage collection
   reclaim the memory.  */

void
free_emit_status (f)
     struct function *f;
{
  free (f->emit->x_regno_reg_rtx);
  free (f->emit->regno_pointer_align);
  free (f->emit->regno_decl);
  free (f->emit);
  f->emit = NULL;
}

/* Go through all the RTL insn bodies and copy any invalid shared
   structure.  This routine should only be called once.  */

void
unshare_all_rtl (fndecl, insn)
     tree fndecl;
     rtx insn;
{
  tree decl;

  /* Make sure that virtual parameters are not shared.  */
  for (decl = DECL_ARGUMENTS (fndecl); decl; decl = TREE_CHAIN (decl))
    SET_DECL_RTL (decl, copy_rtx_if_shared (DECL_RTL (decl)));

  /* Make sure that virtual stack slots are not shared.  */
  unshare_all_decls (DECL_INITIAL (fndecl));

  /* Unshare just about everything else.  */
  unshare_all_rtl_1 (insn);

  /* Make sure the addresses of stack slots found outside the insn chain
     (such as, in DECL_RTL of a variable) are not shared
     with the insn chain.

     This special care is necessary when the stack slot MEM does not
     actually appear in the insn chain.  If it does appear, its address
     is unshared from all else at that point.  */
  stack_slot_list = copy_rtx_if_shared (stack_slot_list);
}

/* Go through all the RTL insn bodies and copy any invalid shared
   structure, again.  This is a fairly expensive thing to do so it
   should be done sparingly.  */

void
unshare_all_rtl_again (insn)
     rtx insn;
{
  rtx p;
  tree decl;

  for (p = insn; p; p = NEXT_INSN (p))
    if (INSN_P (p))
      {
	reset_used_flags (PATTERN (p));
	reset_used_flags (REG_NOTES (p));
	reset_used_flags (LOG_LINKS (p));
      }

  /* Make sure that virtual stack slots are not shared.  */
  reset_used_decls (DECL_INITIAL (cfun->decl));

  /* Make sure that virtual parameters are not shared.  */
  for (decl = DECL_ARGUMENTS (cfun->decl); decl; decl = TREE_CHAIN (decl))
    reset_used_flags (DECL_RTL (decl));

  reset_used_flags (stack_slot_list);

  unshare_all_rtl (cfun->decl, insn);
}

/* Go through all the RTL insn bodies and copy any invalid shared structure.
   Assumes the mark bits are cleared at entry.  */

static void
unshare_all_rtl_1 (insn)
     rtx insn;
{
  for (; insn; insn = NEXT_INSN (insn))
    if (INSN_P (insn))
      {
	PATTERN (insn) = copy_rtx_if_shared (PATTERN (insn));
	REG_NOTES (insn) = copy_rtx_if_shared (REG_NOTES (insn));
	LOG_LINKS (insn) = copy_rtx_if_shared (LOG_LINKS (insn));
      }
}

/* Go through all virtual stack slots of a function and copy any
   shared structure.  */
static void
unshare_all_decls (blk)
     tree blk;
{
  tree t;

  /* Copy shared decls.  */
  for (t = BLOCK_VARS (blk); t; t = TREE_CHAIN (t))
    if (DECL_RTL_SET_P (t))
      SET_DECL_RTL (t, copy_rtx_if_shared (DECL_RTL (t)));

  /* Now process sub-blocks.  */
  for (t = BLOCK_SUBBLOCKS (blk); t; t = TREE_CHAIN (t))
    unshare_all_decls (t);
}

/* Go through all virtual stack slots of a function and mark them as
   not shared.  */
static void
reset_used_decls (blk)
     tree blk;
{
  tree t;

  /* Mark decls.  */
  for (t = BLOCK_VARS (blk); t; t = TREE_CHAIN (t))
    if (DECL_RTL_SET_P (t))
      reset_used_flags (DECL_RTL (t));

  /* Now process sub-blocks.  */
  for (t = BLOCK_SUBBLOCKS (blk); t; t = TREE_CHAIN (t))
    reset_used_decls (t);
}

/* Similar to `copy_rtx' except that if MAY_SHARE is present, it is
   placed in the result directly, rather than being copied.  MAY_SHARE is
   either a MEM of an EXPR_LIST of MEMs.  */

rtx
copy_most_rtx (orig, may_share)
     rtx orig;
     rtx may_share;
{
  rtx copy;
  int i, j;
  RTX_CODE code;
  const char *format_ptr;

  if (orig == may_share
      || (GET_CODE (may_share) == EXPR_LIST
	  && in_expr_list_p (may_share, orig)))
    return orig;

  code = GET_CODE (orig);

  switch (code)
    {
    case REG:
    case QUEUED:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
      return orig;
    default:
      break;
    }

  copy = rtx_alloc (code);
  PUT_MODE (copy, GET_MODE (orig));
  copy->in_struct = orig->in_struct;
  copy->volatil = orig->volatil;
  copy->unchanging = orig->unchanging;
  copy->integrated = orig->integrated;
  copy->frame_related = orig->frame_related;

  format_ptr = GET_RTX_FORMAT (GET_CODE (copy));

  for (i = 0; i < GET_RTX_LENGTH (GET_CODE (copy)); i++)
    {
      switch (*format_ptr++)
	{
	case 'e':
	  XEXP (copy, i) = XEXP (orig, i);
	  if (XEXP (orig, i) != NULL && XEXP (orig, i) != may_share)
	    XEXP (copy, i) = copy_most_rtx (XEXP (orig, i), may_share);
	  break;

	case 'u':
	  XEXP (copy, i) = XEXP (orig, i);
	  break;

	case 'E':
	case 'V':
	  XVEC (copy, i) = XVEC (orig, i);
	  if (XVEC (orig, i) != NULL)
	    {
	      XVEC (copy, i) = rtvec_alloc (XVECLEN (orig, i));
	      for (j = 0; j < XVECLEN (copy, i); j++)
		XVECEXP (copy, i, j)
		  = copy_most_rtx (XVECEXP (orig, i, j), may_share);
	    }
	  break;

	case 'w':
	  XWINT (copy, i) = XWINT (orig, i);
	  break;

	case 'n':
	case 'i':
	  XINT (copy, i) = XINT (orig, i);
	  break;

	case 't':
	  XTREE (copy, i) = XTREE (orig, i);
	  break;

	case 's':
	case 'S':
	  XSTR (copy, i) = XSTR (orig, i);
	  break;

	case '0':
	  /* Copy this through the wide int field; that's safest.  */
	  X0WINT (copy, i) = X0WINT (orig, i);
	  break;

	default:
	  abort ();
	}
    }
  return copy;
}

/* Mark ORIG as in use, and return a copy of it if it was already in use.
   Recursively does the same for subexpressions.  */

rtx
copy_rtx_if_shared (orig)
     rtx orig;
{
  rtx x = orig;
  int i;
  enum rtx_code code;
  const char *format_ptr;
  int copied = 0;

  if (x == 0)
    return 0;

  code = GET_CODE (x);

  /* These types may be freely shared.  */

  switch (code)
    {
    case REG:
    case QUEUED:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
    case SCRATCH:
      /* SCRATCH must be shared because they represent distinct values.  */
      return x;

    case CONST:
      /* CONST can be shared if it contains a SYMBOL_REF.  If it contains
	 a LABEL_REF, it isn't sharable.  */
      if (GET_CODE (XEXP (x, 0)) == PLUS
	  && GET_CODE (XEXP (XEXP (x, 0), 0)) == SYMBOL_REF
	  && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT)
	return x;
      break;

    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case NOTE:
    case BARRIER:
      /* The chain of insns is not being copied.  */
      return x;

    case MEM:
      /* A MEM is allowed to be shared if its address is constant.

	 We used to allow sharing of MEMs which referenced
	 virtual_stack_vars_rtx or virtual_incoming_args_rtx, but
	 that can lose.  instantiate_virtual_regs will not unshare
	 the MEMs, and combine may change the structure of the address
	 because it looks safe and profitable in one context, but
	 in some other context it creates unrecognizable RTL.  */
      if (CONSTANT_ADDRESS_P (XEXP (x, 0)))
	return x;

      break;

    default:
      break;
    }

  /* This rtx may not be shared.  If it has already been seen,
     replace it with a copy of itself.  */

  if (x->used)
    {
      rtx copy;

      copy = rtx_alloc (code);
      memcpy (copy, x,
	     (sizeof (*copy) - sizeof (copy->fld)
	      + sizeof (copy->fld[0]) * GET_RTX_LENGTH (code)));
      x = copy;
      copied = 1;
    }
  x->used = 1;

  /* Now scan the subexpressions recursively.
     We can store any replaced subexpressions directly into X
     since we know X is not shared!  Any vectors in X
     must be copied if X was copied.  */

  format_ptr = GET_RTX_FORMAT (code);

  for (i = 0; i < GET_RTX_LENGTH (code); i++)
    {
      switch (*format_ptr++)
	{
	case 'e':
	  XEXP (x, i) = copy_rtx_if_shared (XEXP (x, i));
	  break;

	case 'E':
	  if (XVEC (x, i) != NULL)
	    {
	      int j;
	      int len = XVECLEN (x, i);

	      if (copied && len > 0)
		XVEC (x, i) = gen_rtvec_v (len, XVEC (x, i)->elem);
	      for (j = 0; j < len; j++)
		XVECEXP (x, i, j) = copy_rtx_if_shared (XVECEXP (x, i, j));
	    }
	  break;
	}
    }
  return x;
}

/* Clear all the USED bits in X to allow copy_rtx_if_shared to be used
   to look for shared sub-parts.  */

void
reset_used_flags (x)
     rtx x;
{
  int i, j;
  enum rtx_code code;
  const char *format_ptr;

  if (x == 0)
    return;

  code = GET_CODE (x);

  /* These types may be freely shared so we needn't do any resetting
     for them.  */

  switch (code)
    {
    case REG:
    case QUEUED:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
      return;

    case INSN:
    case JUMP_INSN:
    case CALL_INSN:
    case NOTE:
    case LABEL_REF:
    case BARRIER:
      /* The chain of insns is not being copied.  */
      return;

    default:
      break;
    }

  x->used = 0;

  format_ptr = GET_RTX_FORMAT (code);
  for (i = 0; i < GET_RTX_LENGTH (code); i++)
    {
      switch (*format_ptr++)
	{
	case 'e':
	  reset_used_flags (XEXP (x, i));
	  break;

	case 'E':
	  for (j = 0; j < XVECLEN (x, i); j++)
	    reset_used_flags (XVECEXP (x, i, j));
	  break;
	}
    }
}

/* Copy X if necessary so that it won't be altered by changes in OTHER.
   Return X or the rtx for the pseudo reg the value of X was copied into.
   OTHER must be valid as a SET_DEST.  */

rtx
make_safe_from (x, other)
     rtx x, other;
{
  while (1)
    switch (GET_CODE (other))
      {
      case SUBREG:
	other = SUBREG_REG (other);
	break;
      case STRICT_LOW_PART:
      case SIGN_EXTEND:
      case ZERO_EXTEND:
	other = XEXP (other, 0);
	break;
      default:
	goto done;
      }
 done:
  if ((GET_CODE (other) == MEM
       && ! CONSTANT_P (x)
       && GET_CODE (x) != REG
       && GET_CODE (x) != SUBREG)
      || (GET_CODE (other) == REG
	  && (REGNO (other) < FIRST_PSEUDO_REGISTER
	      || reg_mentioned_p (other, x))))
    {
      rtx temp = gen_reg_rtx (GET_MODE (x));
      emit_move_insn (temp, x);
      return temp;
    }
  return x;
}

/* Emission of insns (adding them to the doubly-linked list).  */

/* Return the first insn of the current sequence or current function.  */

rtx
get_insns ()
{
  return first_insn;
}

/* Return the last insn emitted in current sequence or current function.  */

rtx
get_last_insn ()
{
  return last_insn;
}

/* Specify a new insn as the last in the chain.  */

void
set_last_insn (insn)
     rtx insn;
{
  if (NEXT_INSN (insn) != 0)
    abort ();
  last_insn = insn;
}

/* Return the last insn emitted, even if it is in a sequence now pushed.  */

rtx
get_last_insn_anywhere ()
{
  struct sequence_stack *stack;
  if (last_insn)
    return last_insn;
  for (stack = seq_stack; stack; stack = stack->next)
    if (stack->last != 0)
      return stack->last;
  return 0;
}

/* Return a number larger than any instruction's uid in this function.  */

int
get_max_uid ()
{
  return cur_insn_uid;
}

/* Renumber instructions so that no instruction UIDs are wasted.  */

void
renumber_insns (stream)
     FILE *stream;
{
  rtx insn;

  /* If we're not supposed to renumber instructions, don't.  */
  if (!flag_renumber_insns)
    return;

  /* If there aren't that many instructions, then it's not really
     worth renumbering them.  */
  if (flag_renumber_insns == 1 && get_max_uid () < 25000)
    return;

  cur_insn_uid = 1;

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      if (stream)
	fprintf (stream, "Renumbering insn %d to %d\n",
		 INSN_UID (insn), cur_insn_uid);
      INSN_UID (insn) = cur_insn_uid++;
    }
}

/* Return the next insn.  If it is a SEQUENCE, return the first insn
   of the sequence.  */

rtx
next_insn (insn)
     rtx insn;
{
  if (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn && GET_CODE (insn) == INSN
	  && GET_CODE (PATTERN (insn)) == SEQUENCE)
	insn = XVECEXP (PATTERN (insn), 0, 0);
    }

  return insn;
}

/* Return the previous insn.  If it is a SEQUENCE, return the last insn
   of the sequence.  */

rtx
previous_insn (insn)
     rtx insn;
{
  if (insn)
    {
      insn = PREV_INSN (insn);
      if (insn && GET_CODE (insn) == INSN
	  && GET_CODE (PATTERN (insn)) == SEQUENCE)
	insn = XVECEXP (PATTERN (insn), 0, XVECLEN (PATTERN (insn), 0) - 1);
    }

  return insn;
}

/* Return the next insn after INSN that is not a NOTE.  This routine does not
   look inside SEQUENCEs.  */

rtx
next_nonnote_insn (insn)
     rtx insn;
{
  while (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn == 0 || GET_CODE (insn) != NOTE)
	break;
    }

  return insn;
}

/* Return the previous insn before INSN that is not a NOTE.  This routine does
   not look inside SEQUENCEs.  */

rtx
prev_nonnote_insn (insn)
     rtx insn;
{
  while (insn)
    {
      insn = PREV_INSN (insn);
      if (insn == 0 || GET_CODE (insn) != NOTE)
	break;
    }

  return insn;
}

/* Return the next INSN, CALL_INSN or JUMP_INSN after INSN;
   or 0, if there is none.  This routine does not look inside
   SEQUENCEs.  */

rtx
next_real_insn (insn)
     rtx insn;
{
  while (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn == 0 || GET_CODE (insn) == INSN
	  || GET_CODE (insn) == CALL_INSN || GET_CODE (insn) == JUMP_INSN)
	break;
    }

  return insn;
}

/* Return the last INSN, CALL_INSN or JUMP_INSN before INSN;
   or 0, if there is none.  This routine does not look inside
   SEQUENCEs.  */

rtx
prev_real_insn (insn)
     rtx insn;
{
  while (insn)
    {
      insn = PREV_INSN (insn);
      if (insn == 0 || GET_CODE (insn) == INSN || GET_CODE (insn) == CALL_INSN
	  || GET_CODE (insn) == JUMP_INSN)
	break;
    }

  return insn;
}

/* Find the next insn after INSN that really does something.  This routine
   does not look inside SEQUENCEs.  Until reload has completed, this is the
   same as next_real_insn.  */

int
active_insn_p (insn)
     rtx insn;
{
  return (GET_CODE (insn) == CALL_INSN || GET_CODE (insn) == JUMP_INSN
	  || (GET_CODE (insn) == INSN
	      && (! reload_completed
		  || (GET_CODE (PATTERN (insn)) != USE
		      && GET_CODE (PATTERN (insn)) != CLOBBER))));
}

rtx
next_active_insn (insn)
     rtx insn;
{
  while (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn == 0 || active_insn_p (insn))
	break;
    }

  return insn;
}

/* Find the last insn before INSN that really does something.  This routine
   does not look inside SEQUENCEs.  Until reload has completed, this is the
   same as prev_real_insn.  */

rtx
prev_active_insn (insn)
     rtx insn;
{
  while (insn)
    {
      insn = PREV_INSN (insn);
      if (insn == 0 || active_insn_p (insn))
	break;
    }

  return insn;
}

/* Return the next CODE_LABEL after the insn INSN, or 0 if there is none.  */

rtx
next_label (insn)
     rtx insn;
{
  while (insn)
    {
      insn = NEXT_INSN (insn);
      if (insn == 0 || GET_CODE (insn) == CODE_LABEL)
	break;
    }

  return insn;
}

/* Return the last CODE_LABEL before the insn INSN, or 0 if there is none.  */

rtx
prev_label (insn)
     rtx insn;
{
  while (insn)
    {
      insn = PREV_INSN (insn);
      if (insn == 0 || GET_CODE (insn) == CODE_LABEL)
	break;
    }

  return insn;
}

#ifdef HAVE_cc0
/* INSN uses CC0 and is being moved into a delay slot.  Set up REG_CC_SETTER
   and REG_CC_USER notes so we can find it.  */

void
link_cc0_insns (insn)
     rtx insn;
{
  rtx user = next_nonnote_insn (insn);

  if (GET_CODE (user) == INSN && GET_CODE (PATTERN (user)) == SEQUENCE)
    user = XVECEXP (PATTERN (user), 0, 0);

  REG_NOTES (user) = gen_rtx_INSN_LIST (REG_CC_SETTER, insn,
					REG_NOTES (user));
  REG_NOTES (insn) = gen_rtx_INSN_LIST (REG_CC_USER, user, REG_NOTES (insn));
}

/* Return the next insn that uses CC0 after INSN, which is assumed to
   set it.  This is the inverse of prev_cc0_setter (i.e., prev_cc0_setter
   applied to the result of this function should yield INSN).

   Normally, this is simply the next insn.  However, if a REG_CC_USER note
   is present, it contains the insn that uses CC0.

   Return 0 if we can't find the insn.  */

rtx
next_cc0_user (insn)
     rtx insn;
{
  rtx note = find_reg_note (insn, REG_CC_USER, NULL_RTX);

  if (note)
    return XEXP (note, 0);

  insn = next_nonnote_insn (insn);
  if (insn && GET_CODE (insn) == INSN && GET_CODE (PATTERN (insn)) == SEQUENCE)
    insn = XVECEXP (PATTERN (insn), 0, 0);

  if (insn && INSN_P (insn) && reg_mentioned_p (cc0_rtx, PATTERN (insn)))
    return insn;

  return 0;
}

/* Find the insn that set CC0 for INSN.  Unless INSN has a REG_CC_SETTER
   note, it is the previous insn.  */

rtx
prev_cc0_setter (insn)
     rtx insn;
{
  rtx note = find_reg_note (insn, REG_CC_SETTER, NULL_RTX);

  if (note)
    return XEXP (note, 0);

  insn = prev_nonnote_insn (insn);
  if (! sets_cc0_p (PATTERN (insn)))
    abort ();

  return insn;
}
#endif

/* Increment the label uses for all labels present in rtx.  */

static void
mark_label_nuses(x)
    rtx x;
{
  enum rtx_code code;
  int i, j;
  const char *fmt;

  code = GET_CODE (x);
  if (code == LABEL_REF)
    LABEL_NUSES (XEXP (x, 0))++;

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'e')
        mark_label_nuses (XEXP (x, i));
      else if (fmt[i] == 'E')
        for (j = XVECLEN (x, i) - 1; j >= 0; j--)
	  mark_label_nuses (XVECEXP (x, i, j));
    }
}


/* Try splitting insns that can be split for better scheduling.
   PAT is the pattern which might split.
   TRIAL is the insn providing PAT.
   LAST is non-zero if we should return the last insn of the sequence produced.

   If this routine succeeds in splitting, it returns the first or last
   replacement insn depending on the value of LAST.  Otherwise, it
   returns TRIAL.  If the insn to be returned can be split, it will be.  */

rtx
try_split (pat, trial, last)
     rtx pat, trial;
     int last;
{
  rtx before = PREV_INSN (trial);
  rtx after = NEXT_INSN (trial);
  int has_barrier = 0;
  rtx tem;
  rtx note, seq;
  int probability;

  if (any_condjump_p (trial)
      && (note = find_reg_note (trial, REG_BR_PROB, 0)))
    split_branch_probability = INTVAL (XEXP (note, 0));
  probability = split_branch_probability;

  seq = split_insns (pat, trial);

  split_branch_probability = -1;

  /* If we are splitting a JUMP_INSN, it might be followed by a BARRIER.
     We may need to handle this specially.  */
  if (after && GET_CODE (after) == BARRIER)
    {
      has_barrier = 1;
      after = NEXT_INSN (after);
    }

  if (seq)
    {
      /* SEQ can either be a SEQUENCE or the pattern of a single insn.
	 The latter case will normally arise only when being done so that
	 it, in turn, will be split (SFmode on the 29k is an example).  */
      if (GET_CODE (seq) == SEQUENCE)
	{
	  int i, njumps = 0;

	  /* Avoid infinite loop if any insn of the result matches
	     the original pattern.  */
	  for (i = 0; i < XVECLEN (seq, 0); i++)
	    if (GET_CODE (XVECEXP (seq, 0, i)) == INSN
		&& rtx_equal_p (PATTERN (XVECEXP (seq, 0, i)), pat))
	      return trial;

	  /* Mark labels.  */
	  for (i = XVECLEN (seq, 0) - 1; i >= 0; i--)
	    if (GET_CODE (XVECEXP (seq, 0, i)) == JUMP_INSN)
	      {
		rtx insn = XVECEXP (seq, 0, i);
		mark_jump_label (PATTERN (insn),
				 XVECEXP (seq, 0, i), 0);
		njumps++;
		if (probability != -1
		    && any_condjump_p (insn)
		    && !find_reg_note (insn, REG_BR_PROB, 0))
		  {
		    /* We can preserve the REG_BR_PROB notes only if exactly
		       one jump is created, otherwise the machine description
		       is responsible for this step using
		       split_branch_probability variable.  */
		    if (njumps != 1)
		      abort ();
		    REG_NOTES (insn)
		      = gen_rtx_EXPR_LIST (REG_BR_PROB,
					   GEN_INT (probability),
					   REG_NOTES (insn));
		  }
	      }

	  /* If we are splitting a CALL_INSN, look for the CALL_INSN
	     in SEQ and copy our CALL_INSN_FUNCTION_USAGE to it.  */
	  if (GET_CODE (trial) == CALL_INSN)
	    for (i = XVECLEN (seq, 0) - 1; i >= 0; i--)
	      if (GET_CODE (XVECEXP (seq, 0, i)) == CALL_INSN)
		CALL_INSN_FUNCTION_USAGE (XVECEXP (seq, 0, i))
		  = CALL_INSN_FUNCTION_USAGE (trial);

	  /* Copy notes, particularly those related to the CFG.  */
	  for (note = REG_NOTES (trial); note ; note = XEXP (note, 1))
	    {
	      switch (REG_NOTE_KIND (note))
		{
		case REG_EH_REGION:
		  for (i = XVECLEN (seq, 0) - 1; i >= 0; i--)
		    {
		      rtx insn = XVECEXP (seq, 0, i);
		      if (GET_CODE (insn) == CALL_INSN
			  || (flag_non_call_exceptions
			      && may_trap_p (PATTERN (insn))))
			REG_NOTES (insn)
			  = gen_rtx_EXPR_LIST (REG_EH_REGION,
					       XEXP (note, 0),
					       REG_NOTES (insn));
		    }
		  break;

		case REG_NORETURN:
		case REG_SETJMP:
		case REG_ALWAYS_RETURN:
		  for (i = XVECLEN (seq, 0) - 1; i >= 0; i--)
		    {
		      rtx insn = XVECEXP (seq, 0, i);
		      if (GET_CODE (insn) == CALL_INSN)
			REG_NOTES (insn)
			  = gen_rtx_EXPR_LIST (REG_NOTE_KIND (note),
					       XEXP (note, 0),
					       REG_NOTES (insn));
		    }
		  break;

		case REG_NON_LOCAL_GOTO:
		  for (i = XVECLEN (seq, 0) - 1; i >= 0; i--)
		    {
		      rtx insn = XVECEXP (seq, 0, i);
		      if (GET_CODE (insn) == JUMP_INSN)
			REG_NOTES (insn)
			  = gen_rtx_EXPR_LIST (REG_NOTE_KIND (note),
					       XEXP (note, 0),
					       REG_NOTES (insn));
		    }
		  break;

		default:
		  break;
		}
	    }

	  /* If there are LABELS inside the split insns increment the
	     usage count so we don't delete the label.  */
	  if (GET_CODE (trial) == INSN)
	    for (i = XVECLEN (seq, 0) - 1; i >= 0; i--)
	      if (GET_CODE (XVECEXP (seq, 0, i)) == INSN)
	        mark_label_nuses (PATTERN (XVECEXP (seq, 0, i)));

	  tem = emit_insn_after (seq, trial);

	  delete_related_insns (trial);
	  if (has_barrier)
	    emit_barrier_after (tem);

	  /* Recursively call try_split for each new insn created; by the
	     time control returns here that insn will be fully split, so
	     set LAST and continue from the insn after the one returned.
	     We can't use next_active_insn here since AFTER may be a note.
	     Ignore deleted insns, which can be occur if not optimizing.  */
	  for (tem = NEXT_INSN (before); tem != after; tem = NEXT_INSN (tem))
	    if (! INSN_DELETED_P (tem) && INSN_P (tem))
	      tem = try_split (PATTERN (tem), tem, 1);
	}
      /* Avoid infinite loop if the result matches the original pattern.  */
      else if (rtx_equal_p (seq, pat))
	return trial;
      else
	{
	  PATTERN (trial) = seq;
	  INSN_CODE (trial) = -1;
	  try_split (seq, trial, last);
	}

      /* Return either the first or the last insn, depending on which was
	 requested.  */
      return last
		? (after ? PREV_INSN (after) : last_insn)
		: NEXT_INSN (before);
    }

  return trial;
}

/* Make and return an INSN rtx, initializing all its slots.
   Store PATTERN in the pattern slots.  */

rtx
make_insn_raw (pattern)
     rtx pattern;
{
  rtx insn;

  insn = rtx_alloc (INSN);

  INSN_UID (insn) = cur_insn_uid++;
  PATTERN (insn) = pattern;
  INSN_CODE (insn) = -1;
  LOG_LINKS (insn) = NULL;
  REG_NOTES (insn) = NULL;

#ifdef ENABLE_RTL_CHECKING
  if (insn
      && INSN_P (insn)
      && (returnjump_p (insn)
	  || (GET_CODE (insn) == SET
	      && SET_DEST (insn) == pc_rtx)))
    {
      warning ("ICE: emit_insn used where emit_jump_insn needed:\n");
      debug_rtx (insn);
    }
#endif

  return insn;
}

/* Like `make_insn' but make a JUMP_INSN instead of an insn.  */

static rtx
make_jump_insn_raw (pattern)
     rtx pattern;
{
  rtx insn;

  insn = rtx_alloc (JUMP_INSN);
  INSN_UID (insn) = cur_insn_uid++;

  PATTERN (insn) = pattern;
  INSN_CODE (insn) = -1;
  LOG_LINKS (insn) = NULL;
  REG_NOTES (insn) = NULL;
  JUMP_LABEL (insn) = NULL;

  return insn;
}

/* Like `make_insn' but make a CALL_INSN instead of an insn.  */

static rtx
make_call_insn_raw (pattern)
     rtx pattern;
{
  rtx insn;

  insn = rtx_alloc (CALL_INSN);
  INSN_UID (insn) = cur_insn_uid++;

  PATTERN (insn) = pattern;
  INSN_CODE (insn) = -1;
  LOG_LINKS (insn) = NULL;
  REG_NOTES (insn) = NULL;
  CALL_INSN_FUNCTION_USAGE (insn) = NULL;

  return insn;
}

/* Add INSN to the end of the doubly-linked list.
   INSN may be an INSN, JUMP_INSN, CALL_INSN, CODE_LABEL, BARRIER or NOTE.  */

void
add_insn (insn)
     rtx insn;
{
  PREV_INSN (insn) = last_insn;
  NEXT_INSN (insn) = 0;

  if (NULL != last_insn)
    NEXT_INSN (last_insn) = insn;

  if (NULL == first_insn)
    first_insn = insn;

  last_insn = insn;
}

/* Add INSN into the doubly-linked list after insn AFTER.  This and
   the next should be the only functions called to insert an insn once
   delay slots have been filled since only they know how to update a
   SEQUENCE.  */

void
add_insn_after (insn, after)
     rtx insn, after;
{
  rtx next = NEXT_INSN (after);
  basic_block bb;

  if (optimize && INSN_DELETED_P (after))
    abort ();

  NEXT_INSN (insn) = next;
  PREV_INSN (insn) = after;

  if (next)
    {
      PREV_INSN (next) = insn;
      if (GET_CODE (next) == INSN && GET_CODE (PATTERN (next)) == SEQUENCE)
	PREV_INSN (XVECEXP (PATTERN (next), 0, 0)) = insn;
    }
  else if (last_insn == after)
    last_insn = insn;
  else
    {
      struct sequence_stack *stack = seq_stack;
      /* Scan all pending sequences too.  */
      for (; stack; stack = stack->next)
	if (after == stack->last)
	  {
	    stack->last = insn;
	    break;
	  }

      if (stack == 0)
	abort ();
    }

  if (basic_block_for_insn
      && (unsigned int) INSN_UID (after) < basic_block_for_insn->num_elements
      && (bb = BLOCK_FOR_INSN (after)))
    {
      set_block_for_insn (insn, bb);
      /* Should not happen as first in the BB is always
	 either NOTE or LABEL.  */
      if (bb->end == after
	  /* Avoid clobbering of structure when creating new BB.  */
	  && GET_CODE (insn) != BARRIER
	  && (GET_CODE (insn) != NOTE
	      || NOTE_LINE_NUMBER (insn) != NOTE_INSN_BASIC_BLOCK))
	bb->end = insn;
    }

  NEXT_INSN (after) = insn;
  if (GET_CODE (after) == INSN && GET_CODE (PATTERN (after)) == SEQUENCE)
    {
      rtx sequence = PATTERN (after);
      NEXT_INSN (XVECEXP (sequence, 0, XVECLEN (sequence, 0) - 1)) = insn;
    }
}

/* Add INSN into the doubly-linked list before insn BEFORE.  This and
   the previous should be the only functions called to insert an insn once
   delay slots have been filled since only they know how to update a
   SEQUENCE.  */

void
add_insn_before (insn, before)
     rtx insn, before;
{
  rtx prev = PREV_INSN (before);
  basic_block bb;

  if (optimize && INSN_DELETED_P (before))
    abort ();

  PREV_INSN (insn) = prev;
  NEXT_INSN (insn) = before;

  if (prev)
    {
      NEXT_INSN (prev) = insn;
      if (GET_CODE (prev) == INSN && GET_CODE (PATTERN (prev)) == SEQUENCE)
	{
	  rtx sequence = PATTERN (prev);
	  NEXT_INSN (XVECEXP (sequence, 0, XVECLEN (sequence, 0) - 1)) = insn;
	}
    }
  else if (first_insn == before)
    first_insn = insn;
  else
    {
      struct sequence_stack *stack = seq_stack;
      /* Scan all pending sequences too.  */
      for (; stack; stack = stack->next)
	if (before == stack->first)
	  {
	    stack->first = insn;
	    break;
	  }

      if (stack == 0)
	abort ();
    }

  if (basic_block_for_insn
      && (unsigned int) INSN_UID (before) < basic_block_for_insn->num_elements
      && (bb = BLOCK_FOR_INSN (before)))
    {
      set_block_for_insn (insn, bb);
      /* Should not happen as first in the BB is always
	 either NOTE or LABEl.  */
      if (bb->head == insn
	  /* Avoid clobbering of structure when creating new BB.  */
	  && GET_CODE (insn) != BARRIER
	  && (GET_CODE (insn) != NOTE
	      || NOTE_LINE_NUMBER (insn) != NOTE_INSN_BASIC_BLOCK))
	abort ();
    }

  PREV_INSN (before) = insn;
  if (GET_CODE (before) == INSN && GET_CODE (PATTERN (before)) == SEQUENCE)
    PREV_INSN (XVECEXP (PATTERN (before), 0, 0)) = insn;
}

/* Remove an insn from its doubly-linked list.  This function knows how
   to handle sequences.  */
void
remove_insn (insn)
     rtx insn;
{
  rtx next = NEXT_INSN (insn);
  rtx prev = PREV_INSN (insn);
  basic_block bb;

  if (prev)
    {
      NEXT_INSN (prev) = next;
      if (GET_CODE (prev) == INSN && GET_CODE (PATTERN (prev)) == SEQUENCE)
	{
	  rtx sequence = PATTERN (prev);
	  NEXT_INSN (XVECEXP (sequence, 0, XVECLEN (sequence, 0) - 1)) = next;
	}
    }
  else if (first_insn == insn)
    first_insn = next;
  else
    {
      struct sequence_stack *stack = seq_stack;
      /* Scan all pending sequences too.  */
      for (; stack; stack = stack->next)
	if (insn == stack->first)
	  {
	    stack->first = next;
	    break;
	  }

      if (stack == 0)
	abort ();
    }

  if (next)
    {
      PREV_INSN (next) = prev;
      if (GET_CODE (next) == INSN && GET_CODE (PATTERN (next)) == SEQUENCE)
	PREV_INSN (XVECEXP (PATTERN (next), 0, 0)) = prev;
    }
  else if (last_insn == insn)
    last_insn = prev;
  else
    {
      struct sequence_stack *stack = seq_stack;
      /* Scan all pending sequences too.  */
      for (; stack; stack = stack->next)
	if (insn == stack->last)
	  {
	    stack->last = prev;
	    break;
	  }

      if (stack == 0)
	abort ();
    }
  if (basic_block_for_insn
      && (unsigned int) INSN_UID (insn) < basic_block_for_insn->num_elements
      && (bb = BLOCK_FOR_INSN (insn)))
    {
      if (bb->head == insn)
	{
	  /* Never ever delete the basic block note without deleting whole
	     basic block.  */
	  if (GET_CODE (insn) == NOTE)
	    abort ();
	  bb->head = next;
	}
      if (bb->end == insn)
	bb->end = prev;
    }
}

/* Delete all insns made since FROM.
   FROM becomes the new last instruction.  */

void
delete_insns_since (from)
     rtx from;
{
  if (from == 0)
    first_insn = 0;
  else
    NEXT_INSN (from) = 0;
  last_insn = from;
}

/* This function is deprecated, please use sequences instead.

   Move a consecutive bunch of insns to a different place in the chain.
   The insns to be moved are those between FROM and TO.
   They are moved to a new position after the insn AFTER.
   AFTER must not be FROM or TO or any insn in between.

   This function does not know about SEQUENCEs and hence should not be
   called after delay-slot filling has been done.  */

void
reorder_insns_nobb (from, to, after)
     rtx from, to, after;
{
  /* Splice this bunch out of where it is now.  */
  if (PREV_INSN (from))
    NEXT_INSN (PREV_INSN (from)) = NEXT_INSN (to);
  if (NEXT_INSN (to))
    PREV_INSN (NEXT_INSN (to)) = PREV_INSN (from);
  if (last_insn == to)
    last_insn = PREV_INSN (from);
  if (first_insn == from)
    first_insn = NEXT_INSN (to);

  /* Make the new neighbors point to it and it to them.  */
  if (NEXT_INSN (after))
    PREV_INSN (NEXT_INSN (after)) = to;

  NEXT_INSN (to) = NEXT_INSN (after);
  PREV_INSN (from) = after;
  NEXT_INSN (after) = from;
  if (after == last_insn)
    last_insn = to;
}

/* Same as function above, but take care to update BB boundaries.  */
void
reorder_insns (from, to, after)
     rtx from, to, after;
{
  rtx prev = PREV_INSN (from);
  basic_block bb, bb2;

  reorder_insns_nobb (from, to, after);

  if (basic_block_for_insn
      && (unsigned int) INSN_UID (after) < basic_block_for_insn->num_elements
      && (bb = BLOCK_FOR_INSN (after)))
    {
      rtx x;
 
      if (basic_block_for_insn
	  && ((unsigned int) INSN_UID (from)
	      < basic_block_for_insn->num_elements)
	  && (bb2 = BLOCK_FOR_INSN (from)))
	{
	  if (bb2->end == to)
	    bb2->end = prev;
	}

      if (bb->end == after)
	bb->end = to;

      for (x = from; x != NEXT_INSN (to); x = NEXT_INSN (x))
	set_block_for_insn (x, bb);
    }
}

/* Return the line note insn preceding INSN.  */

static rtx
find_line_note (insn)
     rtx insn;
{
  if (no_line_numbers)
    return 0;

  for (; insn; insn = PREV_INSN (insn))
    if (GET_CODE (insn) == NOTE
        && NOTE_LINE_NUMBER (insn) >= 0)
      break;

  return insn;
}

/* Like reorder_insns, but inserts line notes to preserve the line numbers
   of the moved insns when debugging.  This may insert a note between AFTER
   and FROM, and another one after TO.  */

void
reorder_insns_with_line_notes (from, to, after)
     rtx from, to, after;
{
  rtx from_line = find_line_note (from);
  rtx after_line = find_line_note (after);

  reorder_insns (from, to, after);

  if (from_line == after_line)
    return;

  if (from_line)
    emit_line_note_after (NOTE_SOURCE_FILE (from_line),
			  NOTE_LINE_NUMBER (from_line),
			  after);
  if (after_line)
    emit_line_note_after (NOTE_SOURCE_FILE (after_line),
			  NOTE_LINE_NUMBER (after_line),
			  to);
}

/* Remove unnecessary notes from the instruction stream.  */

void
remove_unnecessary_notes ()
{
  rtx block_stack = NULL_RTX;
  rtx eh_stack = NULL_RTX;
  rtx insn;
  rtx next;
  rtx tmp;

  /* We must not remove the first instruction in the function because
     the compiler depends on the first instruction being a note.  */
  for (insn = NEXT_INSN (get_insns ()); insn; insn = next)
    {
      /* Remember what's next.  */
      next = NEXT_INSN (insn);

      /* We're only interested in notes.  */
      if (GET_CODE (insn) != NOTE)
	continue;

      switch (NOTE_LINE_NUMBER (insn))
	{
	case NOTE_INSN_DELETED:
	case NOTE_INSN_LOOP_END_TOP_COND:
	  remove_insn (insn);
	  break;

	case NOTE_INSN_EH_REGION_BEG:
	  eh_stack = alloc_INSN_LIST (insn, eh_stack);
	  break;

	case NOTE_INSN_EH_REGION_END:
	  /* Too many end notes.  */
	  if (eh_stack == NULL_RTX)
	    abort ();
	  /* Mismatched nesting.  */
	  if (NOTE_EH_HANDLER (XEXP (eh_stack, 0)) != NOTE_EH_HANDLER (insn))
	    abort ();
	  tmp = eh_stack;
	  eh_stack = XEXP (eh_stack, 1);
	  free_INSN_LIST_node (tmp);
	  break;

	case NOTE_INSN_BLOCK_BEG:
	  /* By now, all notes indicating lexical blocks should have
	     NOTE_BLOCK filled in.  */
	  if (NOTE_BLOCK (insn) == NULL_TREE)
	    abort ();
	  block_stack = alloc_INSN_LIST (insn, block_stack);
	  break;

	case NOTE_INSN_BLOCK_END:
	  /* Too many end notes.  */
	  if (block_stack == NULL_RTX)
	    abort ();
	  /* Mismatched nesting.  */
	  if (NOTE_BLOCK (XEXP (block_stack, 0)) != NOTE_BLOCK (insn))
	    abort ();
	  tmp = block_stack;
	  block_stack = XEXP (block_stack, 1);
	  free_INSN_LIST_node (tmp);

	  /* Scan back to see if there are any non-note instructions
	     between INSN and the beginning of this block.  If not,
	     then there is no PC range in the generated code that will
	     actually be in this block, so there's no point in
	     remembering the existence of the block.  */
	  for (tmp = PREV_INSN (insn); tmp ; tmp = PREV_INSN (tmp))
	    {
	      /* This block contains a real instruction.  Note that we
		 don't include labels; if the only thing in the block
		 is a label, then there are still no PC values that
		 lie within the block.  */
	      if (INSN_P (tmp))
		break;

	      /* We're only interested in NOTEs.  */
	      if (GET_CODE (tmp) != NOTE)
		continue;

	      if (NOTE_LINE_NUMBER (tmp) == NOTE_INSN_BLOCK_BEG)
		{
		  /* We just verified that this BLOCK matches us with
		     the block_stack check above.  Never delete the
		     BLOCK for the outermost scope of the function; we
		     can refer to names from that scope even if the
		     block notes are messed up.  */
		  if (! is_body_block (NOTE_BLOCK (insn))
		      && (*debug_hooks->ignore_block) (NOTE_BLOCK (insn)))
		    {
		      remove_insn (tmp);
		      remove_insn (insn);
		    }
		  break;
		}
	      else if (NOTE_LINE_NUMBER (tmp) == NOTE_INSN_BLOCK_END)
		/* There's a nested block.  We need to leave the
		   current block in place since otherwise the debugger
		   wouldn't be able to show symbols from our block in
		   the nested block.  */
		break;
	    }
	}
    }

  /* Too many begin notes.  */
  if (block_stack || eh_stack)
    abort ();
}


/* Emit an insn of given code and pattern
   at a specified place within the doubly-linked list.  */

/* Make an instruction with body PATTERN
   and output it before the instruction BEFORE.  */

rtx
emit_insn_before (pattern, before)
     rtx pattern, before;
{
  rtx insn = before;

  if (GET_CODE (pattern) == SEQUENCE)
    {
      int i;

      for (i = 0; i < XVECLEN (pattern, 0); i++)
	{
	  insn = XVECEXP (pattern, 0, i);
	  add_insn_before (insn, before);
	}
    }
  else
    {
      insn = make_insn_raw (pattern);
      add_insn_before (insn, before);
    }

  return insn;
}

/* Make an instruction with body PATTERN and code JUMP_INSN
   and output it before the instruction BEFORE.  */

rtx
emit_jump_insn_before (pattern, before)
     rtx pattern, before;
{
  rtx insn;

  if (GET_CODE (pattern) == SEQUENCE)
    insn = emit_insn_before (pattern, before);
  else
    {
      insn = make_jump_insn_raw (pattern);
      add_insn_before (insn, before);
    }

  return insn;
}

/* Make an instruction with body PATTERN and code CALL_INSN
   and output it before the instruction BEFORE.  */

rtx
emit_call_insn_before (pattern, before)
     rtx pattern, before;
{
  rtx insn;

  if (GET_CODE (pattern) == SEQUENCE)
    insn = emit_insn_before (pattern, before);
  else
    {
      insn = make_call_insn_raw (pattern);
      add_insn_before (insn, before);
      PUT_CODE (insn, CALL_INSN);
    }

  return insn;
}

/* Make an insn of code BARRIER
   and output it before the insn BEFORE.  */

rtx
emit_barrier_before (before)
     rtx before;
{
  rtx insn = rtx_alloc (BARRIER);

  INSN_UID (insn) = cur_insn_uid++;

  add_insn_before (insn, before);
  return insn;
}

/* Emit the label LABEL before the insn BEFORE.  */

rtx
emit_label_before (label, before)
     rtx label, before;
{
  /* This can be called twice for the same label as a result of the
     confusion that follows a syntax error!  So make it harmless.  */
  if (INSN_UID (label) == 0)
    {
      INSN_UID (label) = cur_insn_uid++;
      add_insn_before (label, before);
    }

  return label;
}

/* Emit a note of subtype SUBTYPE before the insn BEFORE.  */

rtx
emit_note_before (subtype, before)
     int subtype;
     rtx before;
{
  rtx note = rtx_alloc (NOTE);
  INSN_UID (note) = cur_insn_uid++;
  NOTE_SOURCE_FILE (note) = 0;
  NOTE_LINE_NUMBER (note) = subtype;

  add_insn_before (note, before);
  return note;
}

/* Make an insn of code INSN with body PATTERN
   and output it after the insn AFTER.  */

rtx
emit_insn_after (pattern, after)
     rtx pattern, after;
{
  rtx insn = after;

  if (GET_CODE (pattern) == SEQUENCE)
    {
      int i;

      for (i = 0; i < XVECLEN (pattern, 0); i++)
	{
	  insn = XVECEXP (pattern, 0, i);
	  add_insn_after (insn, after);
	  after = insn;
	}
    }
  else
    {
      insn = make_insn_raw (pattern);
      add_insn_after (insn, after);
    }

  return insn;
}

/* Similar to emit_insn_after, except that line notes are to be inserted so
   as to act as if this insn were at FROM.  */

void
emit_insn_after_with_line_notes (pattern, after, from)
     rtx pattern, after, from;
{
  rtx from_line = find_line_note (from);
  rtx after_line = find_line_note (after);
  rtx insn = emit_insn_after (pattern, after);

  if (from_line)
    emit_line_note_after (NOTE_SOURCE_FILE (from_line),
			  NOTE_LINE_NUMBER (from_line),
			  after);

  if (after_line)
    emit_line_note_after (NOTE_SOURCE_FILE (after_line),
			  NOTE_LINE_NUMBER (after_line),
			  insn);
}

/* Make an insn of code JUMP_INSN with body PATTERN
   and output it after the insn AFTER.  */

rtx
emit_jump_insn_after (pattern, after)
     rtx pattern, after;
{
  rtx insn;

  if (GET_CODE (pattern) == SEQUENCE)
    insn = emit_insn_after (pattern, after);
  else
    {
      insn = make_jump_insn_raw (pattern);
      add_insn_after (insn, after);
    }

  return insn;
}

/* Make an insn of code BARRIER
   and output it after the insn AFTER.  */

rtx
emit_barrier_after (after)
     rtx after;
{
  rtx insn = rtx_alloc (BARRIER);

  INSN_UID (insn) = cur_insn_uid++;

  add_insn_after (insn, after);
  return insn;
}

/* Emit the label LABEL after the insn AFTER.  */

rtx
emit_label_after (label, after)
     rtx label, after;
{
  /* This can be called twice for the same label
     as a result of the confusion that follows a syntax error!
     So make it harmless.  */
  if (INSN_UID (label) == 0)
    {
      INSN_UID (label) = cur_insn_uid++;
      add_insn_after (label, after);
    }

  return label;
}

/* Emit a note of subtype SUBTYPE after the insn AFTER.  */

rtx
emit_note_after (subtype, after)
     int subtype;
     rtx after;
{
  rtx note = rtx_alloc (NOTE);
  INSN_UID (note) = cur_insn_uid++;
  NOTE_SOURCE_FILE (note) = 0;
  NOTE_LINE_NUMBER (note) = subtype;
  add_insn_after (note, after);
  return note;
}

/* Emit a line note for FILE and LINE after the insn AFTER.  */

rtx
emit_line_note_after (file, line, after)
     const char *file;
     int line;
     rtx after;
{
  rtx note;

  if (no_line_numbers && line > 0)
    {
      cur_insn_uid++;
      return 0;
    }

  note  = rtx_alloc (NOTE);
  INSN_UID (note) = cur_insn_uid++;
  NOTE_SOURCE_FILE (note) = file;
  NOTE_LINE_NUMBER (note) = line;
  add_insn_after (note, after);
  return note;
}

/* Make an insn of code INSN with pattern PATTERN
   and add it to the end of the doubly-linked list.
   If PATTERN is a SEQUENCE, take the elements of it
   and emit an insn for each element.

   Returns the last insn emitted.  */

rtx
emit_insn (pattern)
     rtx pattern;
{
  rtx insn = last_insn;

  if (GET_CODE (pattern) == SEQUENCE)
    {
      int i;

      for (i = 0; i < XVECLEN (pattern, 0); i++)
	{
	  insn = XVECEXP (pattern, 0, i);
	  add_insn (insn);
	}
    }
  else
    {
      insn = make_insn_raw (pattern);
      add_insn (insn);
    }

  return insn;
}

/* Emit the insns in a chain starting with INSN.
   Return the last insn emitted.  */

rtx
emit_insns (insn)
     rtx insn;
{
  rtx last = 0;

  while (insn)
    {
      rtx next = NEXT_INSN (insn);
      add_insn (insn);
      last = insn;
      insn = next;
    }

  return last;
}

/* Emit the insns in a chain starting with INSN and place them in front of
   the insn BEFORE.  Return the last insn emitted.  */

rtx
emit_insns_before (insn, before)
     rtx insn;
     rtx before;
{
  rtx last = 0;

  while (insn)
    {
      rtx next = NEXT_INSN (insn);
      add_insn_before (insn, before);
      last = insn;
      insn = next;
    }

  return last;
}

/* Emit the insns in a chain starting with FIRST and place them in back of
   the insn AFTER.  Return the last insn emitted.  */

rtx
emit_insns_after (first, after)
     rtx first;
     rtx after;
{
  rtx last;
  rtx after_after;
  basic_block bb;

  if (!after)
    abort ();

  if (!first)
    return after;

  if (basic_block_for_insn
      && (unsigned int) INSN_UID (after) < basic_block_for_insn->num_elements
      && (bb = BLOCK_FOR_INSN (after)))
    {
      for (last = first; NEXT_INSN (last); last = NEXT_INSN (last))
	set_block_for_insn (last, bb);
      set_block_for_insn (last, bb);
      if (bb->end == after)
	bb->end = last;
    }
  else
    for (last = first; NEXT_INSN (last); last = NEXT_INSN (last))
      continue;

  after_after = NEXT_INSN (after);

  NEXT_INSN (after) = first;
  PREV_INSN (first) = after;
  NEXT_INSN (last) = after_after;
  if (after_after)
    PREV_INSN (after_after) = last;

  if (after == last_insn)
    last_insn = last;
  return last;
}

/* Make an insn of code JUMP_INSN with pattern PATTERN
   and add it to the end of the doubly-linked list.  */

rtx
emit_jump_insn (pattern)
     rtx pattern;
{
  if (GET_CODE (pattern) == SEQUENCE)
    return emit_insn (pattern);
  else
    {
      rtx insn = make_jump_insn_raw (pattern);
      add_insn (insn);
      return insn;
    }
}

/* Make an insn of code CALL_INSN with pattern PATTERN
   and add it to the end of the doubly-linked list.  */

rtx
emit_call_insn (pattern)
     rtx pattern;
{
  if (GET_CODE (pattern) == SEQUENCE)
    return emit_insn (pattern);
  else
    {
      rtx insn = make_call_insn_raw (pattern);
      add_insn (insn);
      PUT_CODE (insn, CALL_INSN);
      return insn;
    }
}

/* Add the label LABEL to the end of the doubly-linked list.  */

rtx
emit_label (label)
     rtx label;
{
  /* This can be called twice for the same label
     as a result of the confusion that follows a syntax error!
     So make it harmless.  */
  if (INSN_UID (label) == 0)
    {
      INSN_UID (label) = cur_insn_uid++;
      add_insn (label);
    }
  return label;
}

/* Make an insn of code BARRIER
   and add it to the end of the doubly-linked list.  */

rtx
emit_barrier ()
{
  rtx barrier = rtx_alloc (BARRIER);
  INSN_UID (barrier) = cur_insn_uid++;
  add_insn (barrier);
  return barrier;
}

/* Make an insn of code NOTE
   with data-fields specified by FILE and LINE
   and add it to the end of the doubly-linked list,
   but only if line-numbers are desired for debugging info.  */

rtx
emit_line_note (file, line)
     const char *file;
     int line;
{
  set_file_and_line_for_stmt (file, line);

#if 0
  if (no_line_numbers)
    return 0;
#endif

  return emit_note (file, line);
}

/* Make an insn of code NOTE
   with data-fields specified by FILE and LINE
   and add it to the end of the doubly-linked list.
   If it is a line-number NOTE, omit it if it matches the previous one.  */

rtx
emit_note (file, line)
     const char *file;
     int line;
{
  rtx note;

  if (line > 0)
    {
      if (file && last_filename && !strcmp (file, last_filename)
	  && line == last_linenum)
	return 0;
      last_filename = file;
      last_linenum = line;
    }

  if (no_line_numbers && line > 0)
    {
      cur_insn_uid++;
      return 0;
    }

  note = rtx_alloc (NOTE);
  INSN_UID (note) = cur_insn_uid++;
  NOTE_SOURCE_FILE (note) = file;
  NOTE_LINE_NUMBER (note) = line;
  add_insn (note);
  return note;
}

/* Emit a NOTE, and don't omit it even if LINE is the previous note.  */

rtx
emit_line_note_force (file, line)
     const char *file;
     int line;
{
  last_linenum = -1;
  return emit_line_note (file, line);
}

/* Cause next statement to emit a line note even if the line number
   has not changed.  This is used at the beginning of a function.  */

void
force_next_line_note ()
{
  last_linenum = -1;
}

/* Place a note of KIND on insn INSN with DATUM as the datum. If a
   note of this type already exists, remove it first.  */

rtx
set_unique_reg_note (insn, kind, datum)
     rtx insn;
     enum reg_note kind;
     rtx datum;
{
  rtx note = find_reg_note (insn, kind, NULL_RTX);

  switch (kind)
    {
    case REG_EQUAL:
    case REG_EQUIV:
      /* Don't add REG_EQUAL/REG_EQUIV notes if the insn
	 has multiple sets (some callers assume single_set
	 means the insn only has one set, when in fact it
	 means the insn only has one * useful * set).  */
      if (GET_CODE (PATTERN (insn)) == PARALLEL && multiple_sets (insn))
	{
	  if (note)
	    abort ();
	  return NULL_RTX;
	}

      /* Don't add ASM_OPERAND REG_EQUAL/REG_EQUIV notes.
	 It serves no useful purpose and breaks eliminate_regs.  */
      if (GET_CODE (datum) == ASM_OPERANDS)
	return NULL_RTX;
      break;

    default:
      break;
    }

  if (note)
    {
      XEXP (note, 0) = datum;
      return note;
    }

  REG_NOTES (insn) = gen_rtx_EXPR_LIST (kind, datum, REG_NOTES (insn));
  return REG_NOTES (insn);
}

/* Return an indication of which type of insn should have X as a body.
   The value is CODE_LABEL, INSN, CALL_INSN or JUMP_INSN.  */

enum rtx_code
classify_insn (x)
     rtx x;
{
  if (GET_CODE (x) == CODE_LABEL)
    return CODE_LABEL;
  if (GET_CODE (x) == CALL)
    return CALL_INSN;
  if (GET_CODE (x) == RETURN)
    return JUMP_INSN;
  if (GET_CODE (x) == SET)
    {
      if (SET_DEST (x) == pc_rtx)
	return JUMP_INSN;
      else if (GET_CODE (SET_SRC (x)) == CALL)
	return CALL_INSN;
      else
	return INSN;
    }
  if (GET_CODE (x) == PARALLEL)
    {
      int j;
      for (j = XVECLEN (x, 0) - 1; j >= 0; j--)
	if (GET_CODE (XVECEXP (x, 0, j)) == CALL)
	  return CALL_INSN;
	else if (GET_CODE (XVECEXP (x, 0, j)) == SET
		 && SET_DEST (XVECEXP (x, 0, j)) == pc_rtx)
	  return JUMP_INSN;
	else if (GET_CODE (XVECEXP (x, 0, j)) == SET
		 && GET_CODE (SET_SRC (XVECEXP (x, 0, j))) == CALL)
	  return CALL_INSN;
    }
  return INSN;
}

/* Emit the rtl pattern X as an appropriate kind of insn.
   If X is a label, it is simply added into the insn chain.  */

rtx
emit (x)
     rtx x;
{
  enum rtx_code code = classify_insn (x);

  if (code == CODE_LABEL)
    return emit_label (x);
  else if (code == INSN)
    return emit_insn (x);
  else if (code == JUMP_INSN)
    {
      rtx insn = emit_jump_insn (x);
      if (any_uncondjump_p (insn) || GET_CODE (x) == RETURN)
	return emit_barrier ();
      return insn;
    }
  else if (code == CALL_INSN)
    return emit_call_insn (x);
  else
    abort ();
}

/* Begin emitting insns to a sequence which can be packaged in an
   RTL_EXPR.  If this sequence will contain something that might cause
   the compiler to pop arguments to function calls (because those
   pops have previously been deferred; see INHIBIT_DEFER_POP for more
   details), use do_pending_stack_adjust before calling this function.
   That will ensure that the deferred pops are not accidentally
   emitted in the middle of this sequence.  */

void
start_sequence ()
{
  struct sequence_stack *tem;

  tem = (struct sequence_stack *) xmalloc (sizeof (struct sequence_stack));

  tem->next = seq_stack;
  tem->first = first_insn;
  tem->last = last_insn;
  tem->sequence_rtl_expr = seq_rtl_expr;

  seq_stack = tem;

  first_insn = 0;
  last_insn = 0;
}

/* Similarly, but indicate that this sequence will be placed in T, an
   RTL_EXPR.  See the documentation for start_sequence for more
   information about how to use this function.  */

void
start_sequence_for_rtl_expr (t)
     tree t;
{
  start_sequence ();

  seq_rtl_expr = t;
}

/* Set up the insn chain starting with FIRST as the current sequence,
   saving the previously current one.  See the documentation for
   start_sequence for more information about how to use this function.  */

void
push_to_sequence (first)
     rtx first;
{
  rtx last;

  start_sequence ();

  for (last = first; last && NEXT_INSN (last); last = NEXT_INSN (last));

  first_insn = first;
  last_insn = last;
}

/* Set up the insn chain from a chain stort in FIRST to LAST.  */

void
push_to_full_sequence (first, last)
     rtx first, last;
{
  start_sequence ();
  first_insn = first;
  last_insn = last;
  /* We really should have the end of the insn chain here.  */
  if (last && NEXT_INSN (last))
    abort ();
}

/* Set up the outer-level insn chain
   as the current sequence, saving the previously current one.  */

void
push_topmost_sequence ()
{
  struct sequence_stack *stack, *top = NULL;

  start_sequence ();

  for (stack = seq_stack; stack; stack = stack->next)
    top = stack;

  first_insn = top->first;
  last_insn = top->last;
  seq_rtl_expr = top->sequence_rtl_expr;
}

/* After emitting to the outer-level insn chain, update the outer-level
   insn chain, and restore the previous saved state.  */

void
pop_topmost_sequence ()
{
  struct sequence_stack *stack, *top = NULL;

  for (stack = seq_stack; stack; stack = stack->next)
    top = stack;

  top->first = first_insn;
  top->last = last_insn;
  /* ??? Why don't we save seq_rtl_expr here?  */

  end_sequence ();
}

/* After emitting to a sequence, restore previous saved state.

   To get the contents of the sequence just made, you must call
   `gen_sequence' *before* calling here.

   If the compiler might have deferred popping arguments while
   generating this sequence, and this sequence will not be immediately
   inserted into the instruction stream, use do_pending_stack_adjust
   before calling gen_sequence.  That will ensure that the deferred
   pops are inserted into this sequence, and not into some random
   location in the instruction stream.  See INHIBIT_DEFER_POP for more
   information about deferred popping of arguments.  */

void
end_sequence ()
{
  struct sequence_stack *tem = seq_stack;

  first_insn = tem->first;
  last_insn = tem->last;
  seq_rtl_expr = tem->sequence_rtl_expr;
  seq_stack = tem->next;

  free (tem);
}

/* This works like end_sequence, but records the old sequence in FIRST
   and LAST.  */

void
end_full_sequence (first, last)
     rtx *first, *last;
{
  *first = first_insn;
  *last = last_insn;
  end_sequence();
}

/* Return 1 if currently emitting into a sequence.  */

int
in_sequence_p ()
{
  return seq_stack != 0;
}

/* Generate a SEQUENCE rtx containing the insns already emitted
   to the current sequence.

   This is how the gen_... function from a DEFINE_EXPAND
   constructs the SEQUENCE that it returns.  */

rtx
gen_sequence ()
{
  rtx result;
  rtx tem;
  int i;
  int len;

  /* Count the insns in the chain.  */
  len = 0;
  for (tem = first_insn; tem; tem = NEXT_INSN (tem))
    len++;

  /* If only one insn, return it rather than a SEQUENCE.
     (Now that we cache SEQUENCE expressions, it isn't worth special-casing
     the case of an empty list.)
     We only return the pattern of an insn if its code is INSN and it
     has no notes.  This ensures that no information gets lost.  */
  if (len == 1
      && ! RTX_FRAME_RELATED_P (first_insn)
      && GET_CODE (first_insn) == INSN
      /* Don't throw away any reg notes.  */
      && REG_NOTES (first_insn) == 0)
    return PATTERN (first_insn);

  result = gen_rtx_SEQUENCE (VOIDmode, rtvec_alloc (len));

  for (i = 0, tem = first_insn; tem; tem = NEXT_INSN (tem), i++)
    XVECEXP (result, 0, i) = tem;

  return result;
}

/* Put the various virtual registers into REGNO_REG_RTX.  */

void
init_virtual_regs (es)
     struct emit_status *es;
{
  rtx *ptr = es->x_regno_reg_rtx;
  ptr[VIRTUAL_INCOMING_ARGS_REGNUM] = virtual_incoming_args_rtx;
  ptr[VIRTUAL_STACK_VARS_REGNUM] = virtual_stack_vars_rtx;
  ptr[VIRTUAL_STACK_DYNAMIC_REGNUM] = virtual_stack_dynamic_rtx;
  ptr[VIRTUAL_OUTGOING_ARGS_REGNUM] = virtual_outgoing_args_rtx;
  ptr[VIRTUAL_CFA_REGNUM] = virtual_cfa_rtx;
}

void
clear_emit_caches ()
{
  int i;

  /* Clear the start_sequence/gen_sequence cache.  */
  for (i = 0; i < SEQUENCE_RESULT_SIZE; i++)
    sequence_result[i] = 0;
  free_insn = 0;
}

/* Used by copy_insn_1 to avoid copying SCRATCHes more than once.  */
static rtx copy_insn_scratch_in[MAX_RECOG_OPERANDS];
static rtx copy_insn_scratch_out[MAX_RECOG_OPERANDS];
static int copy_insn_n_scratches;

/* When an insn is being copied by copy_insn_1, this is nonzero if we have
   copied an ASM_OPERANDS.
   In that case, it is the original input-operand vector.  */
static rtvec orig_asm_operands_vector;

/* When an insn is being copied by copy_insn_1, this is nonzero if we have
   copied an ASM_OPERANDS.
   In that case, it is the copied input-operand vector.  */
static rtvec copy_asm_operands_vector;

/* Likewise for the constraints vector.  */
static rtvec orig_asm_constraints_vector;
static rtvec copy_asm_constraints_vector;

/* Recursively create a new copy of an rtx for copy_insn.
   This function differs from copy_rtx in that it handles SCRATCHes and
   ASM_OPERANDs properly.
   Normally, this function is not used directly; use copy_insn as front end.
   However, you could first copy an insn pattern with copy_insn and then use
   this function afterwards to properly copy any REG_NOTEs containing
   SCRATCHes.  */

rtx
copy_insn_1 (orig)
     rtx orig;
{
  rtx copy;
  int i, j;
  RTX_CODE code;
  const char *format_ptr;

  code = GET_CODE (orig);

  switch (code)
    {
    case REG:
    case QUEUED:
    case CONST_INT:
    case CONST_DOUBLE:
    case CONST_VECTOR:
    case SYMBOL_REF:
    case CODE_LABEL:
    case PC:
    case CC0:
    case ADDRESSOF:
      return orig;

    case SCRATCH:
      for (i = 0; i < copy_insn_n_scratches; i++)
	if (copy_insn_scratch_in[i] == orig)
	  return copy_insn_scratch_out[i];
      break;

    case CONST:
      /* CONST can be shared if it contains a SYMBOL_REF.  If it contains
	 a LABEL_REF, it isn't sharable.  */
      if (GET_CODE (XEXP (orig, 0)) == PLUS
	  && GET_CODE (XEXP (XEXP (orig, 0), 0)) == SYMBOL_REF
	  && GET_CODE (XEXP (XEXP (orig, 0), 1)) == CONST_INT)
	return orig;
      break;

      /* A MEM with a constant address is not sharable.  The problem is that
	 the constant address may need to be reloaded.  If the mem is shared,
	 then reloading one copy of this mem will cause all copies to appear
	 to have been reloaded.  */

    default:
      break;
    }

  copy = rtx_alloc (code);

  /* Copy the various flags, and other information.  We assume that
     all fields need copying, and then clear the fields that should
     not be copied.  That is the sensible default behavior, and forces
     us to explicitly document why we are *not* copying a flag.  */
  memcpy (copy, orig, sizeof (struct rtx_def) - sizeof (rtunion));

  /* We do not copy the USED flag, which is used as a mark bit during
     walks over the RTL.  */
  copy->used = 0;

  /* We do not copy JUMP, CALL, or FRAME_RELATED for INSNs.  */
  if (GET_RTX_CLASS (code) == 'i')
    {
      copy->jump = 0;
      copy->call = 0;
      copy->frame_related = 0;
    }

  format_ptr = GET_RTX_FORMAT (GET_CODE (copy));

  for (i = 0; i < GET_RTX_LENGTH (GET_CODE (copy)); i++)
    {
      copy->fld[i] = orig->fld[i];
      switch (*format_ptr++)
	{
	case 'e':
	  if (XEXP (orig, i) != NULL)
	    XEXP (copy, i) = copy_insn_1 (XEXP (orig, i));
	  break;

	case 'E':
	case 'V':
	  if (XVEC (orig, i) == orig_asm_constraints_vector)
	    XVEC (copy, i) = copy_asm_constraints_vector;
	  else if (XVEC (orig, i) == orig_asm_operands_vector)
	    XVEC (copy, i) = copy_asm_operands_vector;
	  else if (XVEC (orig, i) != NULL)
	    {
	      XVEC (copy, i) = rtvec_alloc (XVECLEN (orig, i));
	      for (j = 0; j < XVECLEN (copy, i); j++)
		XVECEXP (copy, i, j) = copy_insn_1 (XVECEXP (orig, i, j));
	    }
	  break;

	case 't':
	case 'w':
	case 'i':
	case 's':
	case 'S':
	case 'u':
	case '0':
	  /* These are left unchanged.  */
	  break;

	default:
	  abort ();
	}
    }

  if (code == SCRATCH)
    {
      i = copy_insn_n_scratches++;
      if (i >= MAX_RECOG_OPERANDS)
	abort ();
      copy_insn_scratch_in[i] = orig;
      copy_insn_scratch_out[i] = copy;
    }
  else if (code == ASM_OPERANDS)
    {
      orig_asm_operands_vector = ASM_OPERANDS_INPUT_VEC (orig);
      copy_asm_operands_vector = ASM_OPERANDS_INPUT_VEC (copy);
      orig_asm_constraints_vector = ASM_OPERANDS_INPUT_CONSTRAINT_VEC (orig);
      copy_asm_constraints_vector = ASM_OPERANDS_INPUT_CONSTRAINT_VEC (copy);
    }

  return copy;
}

/* Create a new copy of an rtx.
   This function differs from copy_rtx in that it handles SCRATCHes and
   ASM_OPERANDs properly.
   INSN doesn't really have to be a full INSN; it could be just the
   pattern.  */
rtx
copy_insn (insn)
     rtx insn;
{
  copy_insn_n_scratches = 0;
  orig_asm_operands_vector = 0;
  orig_asm_constraints_vector = 0;
  copy_asm_operands_vector = 0;
  copy_asm_constraints_vector = 0;
  return copy_insn_1 (insn);
}

/* Initialize data structures and variables in this file
   before generating rtl for each function.  */

void
init_emit ()
{
  struct function *f = cfun;

  f->emit = (struct emit_status *) xmalloc (sizeof (struct emit_status));
  first_insn = NULL;
  last_insn = NULL;
  seq_rtl_expr = NULL;
  cur_insn_uid = 1;
  reg_rtx_no = LAST_VIRTUAL_REGISTER + 1;
  last_linenum = 0;
  last_filename = 0;
  first_label_num = label_num;
  last_label_num = 0;
  seq_stack = NULL;

  clear_emit_caches ();

  /* Init the tables that describe all the pseudo regs.  */

  f->emit->regno_pointer_align_length = LAST_VIRTUAL_REGISTER + 101;

  f->emit->regno_pointer_align
    = (unsigned char *) xcalloc (f->emit->regno_pointer_align_length,
				 sizeof (unsigned char));

  regno_reg_rtx
    = (rtx *) xcalloc (f->emit->regno_pointer_align_length, sizeof (rtx));

  f->emit->regno_decl
    = (tree *) xcalloc (f->emit->regno_pointer_align_length, sizeof (tree));

  /* Put copies of all the virtual register rtx into regno_reg_rtx.  */
  init_virtual_regs (f->emit);

  /* Indicate that the virtual registers and stack locations are
     all pointers.  */
  REG_POINTER (stack_pointer_rtx) = 1;
  REG_POINTER (frame_pointer_rtx) = 1;
  REG_POINTER (hard_frame_pointer_rtx) = 1;
  REG_POINTER (arg_pointer_rtx) = 1;

  REG_POINTER (virtual_incoming_args_rtx) = 1;
  REG_POINTER (virtual_stack_vars_rtx) = 1;
  REG_POINTER (virtual_stack_dynamic_rtx) = 1;
  REG_POINTER (virtual_outgoing_args_rtx) = 1;
  REG_POINTER (virtual_cfa_rtx) = 1;

#ifdef STACK_BOUNDARY
  REGNO_POINTER_ALIGN (STACK_POINTER_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (FRAME_POINTER_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (HARD_FRAME_POINTER_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (ARG_POINTER_REGNUM) = STACK_BOUNDARY;

  REGNO_POINTER_ALIGN (VIRTUAL_INCOMING_ARGS_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (VIRTUAL_STACK_VARS_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (VIRTUAL_STACK_DYNAMIC_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (VIRTUAL_OUTGOING_ARGS_REGNUM) = STACK_BOUNDARY;
  REGNO_POINTER_ALIGN (VIRTUAL_CFA_REGNUM) = BITS_PER_WORD;
#endif

#ifdef INIT_EXPANDERS
  INIT_EXPANDERS;
#endif
}

/* Mark SS for GC.  */

static void
mark_sequence_stack (ss)
     struct sequence_stack *ss;
{
  while (ss)
    {
      ggc_mark_rtx (ss->first);
      ggc_mark_tree (ss->sequence_rtl_expr);
      ss = ss->next;
    }
}

/* Mark ES for GC.  */

void
mark_emit_status (es)
     struct emit_status *es;
{
  rtx *r;
  tree *t;
  int i;

  if (es == 0)
    return;

  for (i = es->regno_pointer_align_length, r = es->x_regno_reg_rtx,
       t = es->regno_decl;
       i > 0; --i, ++r, ++t)
    {
      ggc_mark_rtx (*r);
      ggc_mark_tree (*t);
    }

  mark_sequence_stack (es->sequence_stack);
  ggc_mark_tree (es->sequence_rtl_expr);
  ggc_mark_rtx (es->x_first_insn);
}

/* Generate the constant 0.  */

static rtx
gen_const_vector_0 (mode)
     enum machine_mode mode;
{
  rtx tem;
  rtvec v;
  int units, i;
  enum machine_mode inner;

  units = GET_MODE_NUNITS (mode);
  inner = GET_MODE_INNER (mode);

  v = rtvec_alloc (units);

  /* We need to call this function after we to set CONST0_RTX first.  */
  if (!CONST0_RTX (inner))
    abort ();

  for (i = 0; i < units; ++i)
    RTVEC_ELT (v, i) = CONST0_RTX (inner);

  tem = gen_rtx_CONST_VECTOR (mode, v);
  return tem;
}

/* Create some permanent unique rtl objects shared between all functions.
   LINE_NUMBERS is nonzero if line numbers are to be generated.  */

void
init_emit_once (line_numbers)
     int line_numbers;
{
  int i;
  enum machine_mode mode;
  enum machine_mode double_mode;

  /* Initialize the CONST_INT and memory attribute hash tables.  */
  const_int_htab = htab_create (37, const_int_htab_hash,
				const_int_htab_eq, NULL);
  ggc_add_deletable_htab (const_int_htab, 0, 0);

  mem_attrs_htab = htab_create (37, mem_attrs_htab_hash,
				mem_attrs_htab_eq, NULL);
  ggc_add_deletable_htab (mem_attrs_htab, 0, mem_attrs_mark);

  no_line_numbers = ! line_numbers;

  /* Compute the word and byte modes.  */

  byte_mode = VOIDmode;
  word_mode = VOIDmode;
  double_mode = VOIDmode;

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_INT); mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      if (GET_MODE_BITSIZE (mode) == BITS_PER_UNIT
	  && byte_mode == VOIDmode)
	byte_mode = mode;

      if (GET_MODE_BITSIZE (mode) == BITS_PER_WORD
	  && word_mode == VOIDmode)
	word_mode = mode;
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_FLOAT); mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    {
      if (GET_MODE_BITSIZE (mode) == DOUBLE_TYPE_SIZE
	  && double_mode == VOIDmode)
	double_mode = mode;
    }

  ptr_mode = mode_for_size (POINTER_SIZE, GET_MODE_CLASS (Pmode), 0);

  /* Assign register numbers to the globally defined register rtx.
     This must be done at runtime because the register number field
     is in a union and some compilers can't initialize unions.  */

  pc_rtx = gen_rtx (PC, VOIDmode);
  cc0_rtx = gen_rtx (CC0, VOIDmode);
  stack_pointer_rtx = gen_raw_REG (Pmode, STACK_POINTER_REGNUM);
  frame_pointer_rtx = gen_raw_REG (Pmode, FRAME_POINTER_REGNUM);
  if (hard_frame_pointer_rtx == 0)
    hard_frame_pointer_rtx = gen_raw_REG (Pmode,
					  HARD_FRAME_POINTER_REGNUM);
  if (arg_pointer_rtx == 0)
    arg_pointer_rtx = gen_raw_REG (Pmode, ARG_POINTER_REGNUM);
  virtual_incoming_args_rtx =
    gen_raw_REG (Pmode, VIRTUAL_INCOMING_ARGS_REGNUM);
  virtual_stack_vars_rtx =
    gen_raw_REG (Pmode, VIRTUAL_STACK_VARS_REGNUM);
  virtual_stack_dynamic_rtx =
    gen_raw_REG (Pmode, VIRTUAL_STACK_DYNAMIC_REGNUM);
  virtual_outgoing_args_rtx =
    gen_raw_REG (Pmode, VIRTUAL_OUTGOING_ARGS_REGNUM);
  virtual_cfa_rtx = gen_raw_REG (Pmode, VIRTUAL_CFA_REGNUM);

  /* These rtx must be roots if GC is enabled.  */
  ggc_add_rtx_root (global_rtl, GR_MAX);

#ifdef INIT_EXPANDERS
  /* This is to initialize {init|mark|free}_machine_status before the first
     call to push_function_context_to.  This is needed by the Chill front
     end which calls push_function_context_to before the first call to
     init_function_start.  */
  INIT_EXPANDERS;
#endif

  /* Create the unique rtx's for certain rtx codes and operand values.  */

  /* Don't use gen_rtx here since gen_rtx in this case
     tries to use these variables.  */
  for (i = - MAX_SAVED_CONST_INT; i <= MAX_SAVED_CONST_INT; i++)
    const_int_rtx[i + MAX_SAVED_CONST_INT] =
      gen_rtx_raw_CONST_INT (VOIDmode, (HOST_WIDE_INT) i);
  ggc_add_rtx_root (const_int_rtx, 2 * MAX_SAVED_CONST_INT + 1);

  if (STORE_FLAG_VALUE >= - MAX_SAVED_CONST_INT
      && STORE_FLAG_VALUE <= MAX_SAVED_CONST_INT)
    const_true_rtx = const_int_rtx[STORE_FLAG_VALUE + MAX_SAVED_CONST_INT];
  else
    const_true_rtx = gen_rtx_CONST_INT (VOIDmode, STORE_FLAG_VALUE);

  dconst0 = REAL_VALUE_ATOF ("0", double_mode);
  dconst1 = REAL_VALUE_ATOF ("1", double_mode);
  dconst2 = REAL_VALUE_ATOF ("2", double_mode);
  dconstm1 = REAL_VALUE_ATOF ("-1", double_mode);

  for (i = 0; i <= 2; i++)
    {
      for (mode = GET_CLASS_NARROWEST_MODE (MODE_FLOAT); mode != VOIDmode;
	   mode = GET_MODE_WIDER_MODE (mode))
	{
	  rtx tem = rtx_alloc (CONST_DOUBLE);
	  union real_extract u;

	  /* Zero any holes in a structure.  */
	  memset ((char *) &u, 0, sizeof u);
	  u.d = i == 0 ? dconst0 : i == 1 ? dconst1 : dconst2;

	  /* Avoid trailing garbage in the rtx.  */
	  if (sizeof (u) < sizeof (HOST_WIDE_INT))
	    CONST_DOUBLE_LOW (tem) = 0;
	  if (sizeof (u) < 2 * sizeof (HOST_WIDE_INT))
	    CONST_DOUBLE_HIGH (tem) = 0;

	  memcpy (&CONST_DOUBLE_LOW (tem), &u, sizeof u);
	  CONST_DOUBLE_CHAIN (tem) = NULL_RTX;
	  PUT_MODE (tem, mode);

	  const_tiny_rtx[i][(int) mode] = tem;
	}

      const_tiny_rtx[i][(int) VOIDmode] = GEN_INT (i);

      for (mode = GET_CLASS_NARROWEST_MODE (MODE_INT); mode != VOIDmode;
	   mode = GET_MODE_WIDER_MODE (mode))
	const_tiny_rtx[i][(int) mode] = GEN_INT (i);

      for (mode = GET_CLASS_NARROWEST_MODE (MODE_PARTIAL_INT);
	   mode != VOIDmode;
	   mode = GET_MODE_WIDER_MODE (mode))
	const_tiny_rtx[i][(int) mode] = GEN_INT (i);
    }

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_INT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    const_tiny_rtx[0][(int) mode] = gen_const_vector_0 (mode);

  for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_FLOAT);
       mode != VOIDmode;
       mode = GET_MODE_WIDER_MODE (mode))
    const_tiny_rtx[0][(int) mode] = gen_const_vector_0 (mode);

  for (i = (int) CCmode; i < (int) MAX_MACHINE_MODE; ++i)
    if (GET_MODE_CLASS ((enum machine_mode) i) == MODE_CC)
      const_tiny_rtx[0][i] = const0_rtx;

  const_tiny_rtx[0][(int) BImode] = const0_rtx;
  if (STORE_FLAG_VALUE == 1)
    const_tiny_rtx[1][(int) BImode] = const1_rtx;

  /* For bounded pointers, `&const_tiny_rtx[0][0]' is not the same as
     `(rtx *) const_tiny_rtx'.  The former has bounds that only cover
     `const_tiny_rtx[0]', whereas the latter has bounds that cover all.  */
  ggc_add_rtx_root ((rtx *) const_tiny_rtx, sizeof const_tiny_rtx / sizeof (rtx));
  ggc_add_rtx_root (&const_true_rtx, 1);

#ifdef RETURN_ADDRESS_POINTER_REGNUM
  return_address_pointer_rtx
    = gen_raw_REG (Pmode, RETURN_ADDRESS_POINTER_REGNUM);
#endif

#ifdef STRUCT_VALUE
  struct_value_rtx = STRUCT_VALUE;
#else
  struct_value_rtx = gen_rtx_REG (Pmode, STRUCT_VALUE_REGNUM);
#endif

#ifdef STRUCT_VALUE_INCOMING
  struct_value_incoming_rtx = STRUCT_VALUE_INCOMING;
#else
#ifdef STRUCT_VALUE_INCOMING_REGNUM
  struct_value_incoming_rtx
    = gen_rtx_REG (Pmode, STRUCT_VALUE_INCOMING_REGNUM);
#else
  struct_value_incoming_rtx = struct_value_rtx;
#endif
#endif

#ifdef STATIC_CHAIN_REGNUM
  static_chain_rtx = gen_rtx_REG (Pmode, STATIC_CHAIN_REGNUM);

#ifdef STATIC_CHAIN_INCOMING_REGNUM
  if (STATIC_CHAIN_INCOMING_REGNUM != STATIC_CHAIN_REGNUM)
    static_chain_incoming_rtx
      = gen_rtx_REG (Pmode, STATIC_CHAIN_INCOMING_REGNUM);
  else
#endif
    static_chain_incoming_rtx = static_chain_rtx;
#endif

#ifdef STATIC_CHAIN
  static_chain_rtx = STATIC_CHAIN;

#ifdef STATIC_CHAIN_INCOMING
  static_chain_incoming_rtx = STATIC_CHAIN_INCOMING;
#else
  static_chain_incoming_rtx = static_chain_rtx;
#endif
#endif

  if (PIC_OFFSET_TABLE_REGNUM != INVALID_REGNUM)
    pic_offset_table_rtx = gen_raw_REG (Pmode, PIC_OFFSET_TABLE_REGNUM);

  ggc_add_rtx_root (&pic_offset_table_rtx, 1);
  ggc_add_rtx_root (&struct_value_rtx, 1);
  ggc_add_rtx_root (&struct_value_incoming_rtx, 1);
  ggc_add_rtx_root (&static_chain_rtx, 1);
  ggc_add_rtx_root (&static_chain_incoming_rtx, 1);
  ggc_add_rtx_root (&return_address_pointer_rtx, 1);
}

/* Query and clear/ restore no_line_numbers.  This is used by the
   switch / case handling in stmt.c to give proper line numbers in
   warnings about unreachable code.  */

int
force_line_numbers ()
{
  int old = no_line_numbers;

  no_line_numbers = 0;
  if (old)
    force_next_line_note ();
  return old;
}

void
restore_line_number_status (old_value)
     int old_value;
{
  no_line_numbers = old_value;
}
