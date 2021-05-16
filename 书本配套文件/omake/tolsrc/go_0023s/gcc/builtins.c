/* Expand builtin functions.
   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
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

/* !kawai! */
#include "config.h"
#include "system.h"
#include "machmode.h"
#include "rtl.h"
#include "tree.h"
#include "../include/obstack.h"
#include "flags.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "except.h"
#include "function.h"
#include "insn-config.h"
#include "expr.h"
#include "optabs.h"
#include "libfuncs.h"
#include "recog.h"
#include "output.h"
#include "typeclass.h"
#include "toplev.h"
#include "predict.h"
#include "tm_p.h"
#include "target.h"
/* end of !kawai! */

#define CALLED_AS_BUILT_IN(NODE) \
   (!strncmp (IDENTIFIER_POINTER (DECL_NAME (NODE)), "__builtin_", 10))

/* Register mappings for target machines without register windows.  */
#ifndef INCOMING_REGNO
#define INCOMING_REGNO(OUT) (OUT)
#endif
#ifndef OUTGOING_REGNO
#define OUTGOING_REGNO(IN) (IN)
#endif

#ifndef PAD_VARARGS_DOWN
#define PAD_VARARGS_DOWN BYTES_BIG_ENDIAN
#endif

/* Define the names of the builtin function types and codes.  */
const char *const built_in_class_names[4]
  = {"NOT_BUILT_IN", "BUILT_IN_FRONTEND", "BUILT_IN_MD", "BUILT_IN_NORMAL"};

#define DEF_BUILTIN(X, N, C, T, LT, B, F, NA) STRINGX(X),
const char *const built_in_names[(int) END_BUILTINS] =
{
#include "builtins.def"
};
#undef DEF_BUILTIN

/* Setup an array of _DECL trees, make sure each element is
   initialized to NULL_TREE.  */
tree built_in_decls[(int) END_BUILTINS];

tree (*lang_type_promotes_to) PARAMS ((tree));

static int get_pointer_alignment	PARAMS ((tree, unsigned int));
static tree c_strlen			PARAMS ((tree));
static const char *c_getstr		PARAMS ((tree));
static rtx c_readstr			PARAMS ((const char *,
						 enum machine_mode));
static int target_char_cast		PARAMS ((tree, char *));
static rtx get_memory_rtx		PARAMS ((tree));
static int apply_args_size		PARAMS ((void));
static int apply_result_size		PARAMS ((void));
#if defined (HAVE_untyped_call) || defined (HAVE_untyped_return)
static rtx result_vector		PARAMS ((int, rtx));
#endif
static rtx expand_builtin_setjmp	PARAMS ((tree, rtx));
static void expand_builtin_prefetch	PARAMS ((tree));
static rtx expand_builtin_apply_args	PARAMS ((void));
static rtx expand_builtin_apply_args_1	PARAMS ((void));
static rtx expand_builtin_apply		PARAMS ((rtx, rtx, rtx));
static void expand_builtin_return	PARAMS ((rtx));
static enum type_class type_to_class	PARAMS ((tree));
static rtx expand_builtin_classify_type	PARAMS ((tree));
static rtx expand_builtin_mathfn	PARAMS ((tree, rtx, rtx));
static rtx expand_builtin_constant_p	PARAMS ((tree));
static rtx expand_builtin_args_info	PARAMS ((tree));
static rtx expand_builtin_next_arg	PARAMS ((tree));
static rtx expand_builtin_va_start	PARAMS ((int, tree));
static rtx expand_builtin_va_end	PARAMS ((tree));
static rtx expand_builtin_va_copy	PARAMS ((tree));
static rtx expand_builtin_memcmp	PARAMS ((tree, tree, rtx,
                                                 enum machine_mode));
static rtx expand_builtin_strcmp	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strncmp	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx builtin_memcpy_read_str	PARAMS ((PTR, HOST_WIDE_INT,
						 enum machine_mode));
static rtx expand_builtin_strcat	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strncat	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strspn	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strcspn	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_memcpy	PARAMS ((tree, rtx,
                                                 enum machine_mode));
static rtx expand_builtin_strcpy	PARAMS ((tree, rtx,
                                                 enum machine_mode));
static rtx builtin_strncpy_read_str	PARAMS ((PTR, HOST_WIDE_INT,
						 enum machine_mode));
static rtx expand_builtin_strncpy	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx builtin_memset_read_str	PARAMS ((PTR, HOST_WIDE_INT,
						 enum machine_mode));
static rtx expand_builtin_memset	PARAMS ((tree, rtx,
                                                 enum machine_mode));
static rtx expand_builtin_bzero		PARAMS ((tree));
static rtx expand_builtin_strlen	PARAMS ((tree, rtx));
static rtx expand_builtin_strstr	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strpbrk	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strchr	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_strrchr	PARAMS ((tree, rtx,
						 enum machine_mode));
static rtx expand_builtin_alloca	PARAMS ((tree, rtx));
static rtx expand_builtin_ffs		PARAMS ((tree, rtx, rtx));
static rtx expand_builtin_frame_address	PARAMS ((tree));
static rtx expand_builtin_fputs		PARAMS ((tree, int, int));
static tree stabilize_va_list		PARAMS ((tree, int));
static rtx expand_builtin_expect	PARAMS ((tree, rtx));
static tree fold_builtin_constant_p	PARAMS ((tree));
static tree fold_builtin_classify_type	PARAMS ((tree));
static tree build_function_call_expr	PARAMS ((tree, tree));
static int validate_arglist		PARAMS ((tree, ...));

/* Return the alignment in bits of EXP, a pointer valued expression.
   But don't return more than MAX_ALIGN no matter what.
   The alignment returned is, by default, the alignment of the thing that
   EXP points to.  If it is not a POINTER_TYPE, 0 is returned.

   Otherwise, look at the expression to see if we can do better, i.e., if the
   expression is actually pointing at an object whose alignment is tighter.  */

static int
get_pointer_alignment (exp, max_align)
     tree exp;
     unsigned int max_align;
{
  unsigned int align, inner;

  if (TREE_CODE (TREE_TYPE (exp)) != POINTER_TYPE)
    return 0;

  align = TYPE_ALIGN (TREE_TYPE (TREE_TYPE (exp)));
  align = MIN (align, max_align);

  while (1)
    {
      switch (TREE_CODE (exp))
	{
	case NOP_EXPR:
	case CONVERT_EXPR:
	case NON_LVALUE_EXPR:
	  exp = TREE_OPERAND (exp, 0);
	  if (TREE_CODE (TREE_TYPE (exp)) != POINTER_TYPE)
	    return align;

	  inner = TYPE_ALIGN (TREE_TYPE (TREE_TYPE (exp)));
	  align = MIN (inner, max_align);
	  break;

	case PLUS_EXPR:
	  /* If sum of pointer + int, restrict our maximum alignment to that
	     imposed by the integer.  If not, we can't do any better than
	     ALIGN.  */
	  if (! host_integerp (TREE_OPERAND (exp, 1), 1))
	    return align;

	  while (((tree_low_cst (TREE_OPERAND (exp, 1), 1))
		  & (max_align / BITS_PER_UNIT - 1))
		 != 0)
	    max_align >>= 1;

	  exp = TREE_OPERAND (exp, 0);
	  break;

	case ADDR_EXPR:
	  /* See what we are pointing at and look at its alignment.  */
	  exp = TREE_OPERAND (exp, 0);
	  if (TREE_CODE (exp) == FUNCTION_DECL)
	    align = FUNCTION_BOUNDARY;
	  else if (DECL_P (exp))
	    align = DECL_ALIGN (exp);
#ifdef CONSTANT_ALIGNMENT
	  else if (TREE_CODE_CLASS (TREE_CODE (exp)) == 'c')
	    align = CONSTANT_ALIGNMENT (exp, align);
#endif
	  return MIN (align, max_align);

	default:
	  return align;
	}
    }
}

/* Compute the length of a C string.  TREE_STRING_LENGTH is not the right
   way, because it could contain a zero byte in the middle.
   TREE_STRING_LENGTH is the size of the character array, not the string.

   The value returned is of type `ssizetype'.

   Unfortunately, string_constant can't access the values of const char
   arrays with initializers, so neither can we do so here.  */

static tree
c_strlen (src)
     tree src;
{
  tree offset_node;
  HOST_WIDE_INT offset;
  int max;
  const char *ptr;

  src = string_constant (src, &offset_node);
  if (src == 0)
    return 0;

  max = TREE_STRING_LENGTH (src) - 1;
  ptr = TREE_STRING_POINTER (src);

  if (offset_node && TREE_CODE (offset_node) != INTEGER_CST)
    {
      /* If the string has an internal zero byte (e.g., "foo\0bar"), we can't
	 compute the offset to the following null if we don't know where to
	 start searching for it.  */
      int i;

      for (i = 0; i < max; i++)
	if (ptr[i] == 0)
	  return 0;

      /* We don't know the starting offset, but we do know that the string
	 has no internal zero bytes.  We can assume that the offset falls
	 within the bounds of the string; otherwise, the programmer deserves
	 what he gets.  Subtract the offset from the length of the string,
	 and return that.  This would perhaps not be valid if we were dealing
	 with named arrays in addition to literal string constants.  */

      return size_diffop (size_int (max), offset_node);
    }

  /* We have a known offset into the string.  Start searching there for
     a null character if we can represent it as a single HOST_WIDE_INT.  */
  if (offset_node == 0)
    offset = 0;
  else if (! host_integerp (offset_node, 0))
    offset = -1;
  else
    offset = tree_low_cst (offset_node, 0);

  /* If the offset is known to be out of bounds, warn, and call strlen at
     runtime.  */
  if (offset < 0 || offset > max)
    {
      warning ("offset outside bounds of constant string");
      return 0;
    }

  /* Use strlen to search for the first zero byte.  Since any strings
     constructed with build_string will have nulls appended, we win even
     if we get handed something like (char[4])"abcd".

     Since OFFSET is our starting index into the string, no further
     calculation is needed.  */
  return ssize_int (strlen (ptr + offset));
}

/* Return a char pointer for a C string if it is a string constant
   or sum of string constant and integer constant.  */

static const char *
c_getstr (src)
     tree src;
{
  tree offset_node;

  src = string_constant (src, &offset_node);
  if (src == 0)
    return 0;

  if (offset_node == 0)
    return TREE_STRING_POINTER (src);
  else if (!host_integerp (offset_node, 1)
	   || compare_tree_int (offset_node, TREE_STRING_LENGTH (src) - 1) > 0)
    return 0;

  return TREE_STRING_POINTER (src) + tree_low_cst (offset_node, 1);
}

/* Return a CONST_INT or CONST_DOUBLE corresponding to target reading
   GET_MODE_BITSIZE (MODE) bits from string constant STR.  */

static rtx
c_readstr (str, mode)
     const char *str;
     enum machine_mode mode;
{
  HOST_WIDE_INT c[2];
  HOST_WIDE_INT ch;
  unsigned int i, j;

  if (GET_MODE_CLASS (mode) != MODE_INT)
    abort ();
  c[0] = 0;
  c[1] = 0;
  ch = 1;
  for (i = 0; i < GET_MODE_SIZE (mode); i++)
    {
      j = i;
      if (WORDS_BIG_ENDIAN)
	j = GET_MODE_SIZE (mode) - i - 1;
      if (BYTES_BIG_ENDIAN != WORDS_BIG_ENDIAN
	  && GET_MODE_SIZE (mode) > UNITS_PER_WORD)
	j = j + UNITS_PER_WORD - 2 * (j % UNITS_PER_WORD) - 1;
      j *= BITS_PER_UNIT;
      if (j > 2 * HOST_BITS_PER_WIDE_INT)
	abort ();
      if (ch)
	ch = (unsigned char) str[i];
      c[j / HOST_BITS_PER_WIDE_INT] |= ch << (j % HOST_BITS_PER_WIDE_INT);
    }
  return immed_double_const (c[0], c[1], mode);
}

/* Cast a target constant CST to target CHAR and if that value fits into
   host char type, return zero and put that value into variable pointed by
   P.  */

static int
target_char_cast (cst, p)
     tree cst;
     char *p;
{
  unsigned HOST_WIDE_INT val, hostval;

  if (!host_integerp (cst, 1)
      || CHAR_TYPE_SIZE > HOST_BITS_PER_WIDE_INT)
    return 1;

  val = tree_low_cst (cst, 1);
  if (CHAR_TYPE_SIZE < HOST_BITS_PER_WIDE_INT)
    val &= (((unsigned HOST_WIDE_INT) 1) << CHAR_TYPE_SIZE) - 1;

  hostval = val;
  if (HOST_BITS_PER_CHAR < HOST_BITS_PER_WIDE_INT)
    hostval &= (((unsigned HOST_WIDE_INT) 1) << HOST_BITS_PER_CHAR) - 1;

  if (val != hostval)
    return 1;

  *p = hostval;
  return 0;
}

/* Given TEM, a pointer to a stack frame, follow the dynamic chain COUNT
   times to get the address of either a higher stack frame, or a return
   address located within it (depending on FNDECL_CODE).  */

rtx
expand_builtin_return_addr (fndecl_code, count, tem)
     enum built_in_function fndecl_code;
     int count;
     rtx tem;
{
  int i;

  /* Some machines need special handling before we can access
     arbitrary frames.  For example, on the sparc, we must first flush
     all register windows to the stack.  */
#ifdef SETUP_FRAME_ADDRESSES
  if (count > 0)
    SETUP_FRAME_ADDRESSES ();
#endif

  /* On the sparc, the return address is not in the frame, it is in a
     register.  There is no way to access it off of the current frame
     pointer, but it can be accessed off the previous frame pointer by
     reading the value from the register window save area.  */
#ifdef RETURN_ADDR_IN_PREVIOUS_FRAME
  if (fndecl_code == BUILT_IN_RETURN_ADDRESS)
    count--;
#endif

  /* Scan back COUNT frames to the specified frame.  */
  for (i = 0; i < count; i++)
    {
      /* Assume the dynamic chain pointer is in the word that the
	 frame address points to, unless otherwise specified.  */
#ifdef DYNAMIC_CHAIN_ADDRESS
      tem = DYNAMIC_CHAIN_ADDRESS (tem);
#endif
      tem = memory_address (Pmode, tem);
      tem = gen_rtx_MEM (Pmode, tem);
      set_mem_alias_set (tem, get_frame_alias_set ());
      tem = copy_to_reg (tem);
    }

  /* For __builtin_frame_address, return what we've got.  */
  if (fndecl_code == BUILT_IN_FRAME_ADDRESS)
    return tem;

  /* For __builtin_return_address, Get the return address from that
     frame.  */
#ifdef RETURN_ADDR_RTX
  tem = RETURN_ADDR_RTX (count, tem);
#else
  tem = memory_address (Pmode,
			plus_constant (tem, GET_MODE_SIZE (Pmode)));
  tem = gen_rtx_MEM (Pmode, tem);
  set_mem_alias_set (tem, get_frame_alias_set ());
#endif
  return tem;
}

/* Alias set used for setjmp buffer.  */
static HOST_WIDE_INT setjmp_alias_set = -1;

/* Construct the leading half of a __builtin_setjmp call.  Control will
   return to RECEIVER_LABEL.  This is used directly by sjlj exception
   handling code.  */

void
expand_builtin_setjmp_setup (buf_addr, receiver_label)
     rtx buf_addr;
     rtx receiver_label;
{
  enum machine_mode sa_mode = STACK_SAVEAREA_MODE (SAVE_NONLOCAL);
  rtx stack_save;
  rtx mem;

  if (setjmp_alias_set == -1)
    setjmp_alias_set = new_alias_set ();

#ifdef POINTERS_EXTEND_UNSIGNED
  if (GET_MODE (buf_addr) != Pmode)
    buf_addr = convert_memory_address (Pmode, buf_addr);
#endif

  buf_addr = force_reg (Pmode, force_operand (buf_addr, NULL_RTX));

  emit_queue ();

  /* We store the frame pointer and the address of receiver_label in
     the buffer and use the rest of it for the stack save area, which
     is machine-dependent.  */

#ifndef BUILTIN_SETJMP_FRAME_VALUE
#define BUILTIN_SETJMP_FRAME_VALUE virtual_stack_vars_rtx
#endif

  mem = gen_rtx_MEM (Pmode, buf_addr);
  set_mem_alias_set (mem, setjmp_alias_set);
  emit_move_insn (mem, BUILTIN_SETJMP_FRAME_VALUE);

  mem = gen_rtx_MEM (Pmode, plus_constant (buf_addr, GET_MODE_SIZE (Pmode))),
  set_mem_alias_set (mem, setjmp_alias_set);

  emit_move_insn (validize_mem (mem),
		  force_reg (Pmode, gen_rtx_LABEL_REF (Pmode, receiver_label)));

  stack_save = gen_rtx_MEM (sa_mode,
			    plus_constant (buf_addr,
					   2 * GET_MODE_SIZE (Pmode)));
  set_mem_alias_set (stack_save, setjmp_alias_set);
  emit_stack_save (SAVE_NONLOCAL, &stack_save, NULL_RTX);

  /* If there is further processing to do, do it.  */
#ifdef HAVE_builtin_setjmp_setup
  if (HAVE_builtin_setjmp_setup)
    emit_insn (gen_builtin_setjmp_setup (buf_addr));
#endif

  /* Tell optimize_save_area_alloca that extra work is going to
     need to go on during alloca.  */
  current_function_calls_setjmp = 1;

  /* Set this so all the registers get saved in our frame; we need to be
     able to copy the saved values for any registers from frames we unwind.  */
  current_function_has_nonlocal_label = 1;
}

/* Construct the trailing part of a __builtin_setjmp call.
   This is used directly by sjlj exception handling code.  */

void
expand_builtin_setjmp_receiver (receiver_label)
      rtx receiver_label ATTRIBUTE_UNUSED;
{
  /* Clobber the FP when we get here, so we have to make sure it's
     marked as used by this function.  */
  emit_insn (gen_rtx_USE (VOIDmode, hard_frame_pointer_rtx));

  /* Mark the static chain as clobbered here so life information
     doesn't get messed up for it.  */
  emit_insn (gen_rtx_CLOBBER (VOIDmode, static_chain_rtx));

  /* Now put in the code to restore the frame pointer, and argument
     pointer, if needed.  The code below is from expand_end_bindings
     in stmt.c; see detailed documentation there.  */
#ifdef HAVE_nonlocal_goto
  if (! HAVE_nonlocal_goto)
#endif
    emit_move_insn (virtual_stack_vars_rtx, hard_frame_pointer_rtx);

#if ARG_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
  if (fixed_regs[ARG_POINTER_REGNUM])
    {
#ifdef ELIMINABLE_REGS
      size_t i;
      static const struct elims {const int from, to;} elim_regs[] = ELIMINABLE_REGS;

      for (i = 0; i < ARRAY_SIZE (elim_regs); i++)
	if (elim_regs[i].from == ARG_POINTER_REGNUM
	    && elim_regs[i].to == HARD_FRAME_POINTER_REGNUM)
	  break;

      if (i == ARRAY_SIZE (elim_regs))
#endif
	{
	  /* Now restore our arg pointer from the address at which it
	     was saved in our stack frame.  */
	  emit_move_insn (virtual_incoming_args_rtx,
			  copy_to_reg (get_arg_pointer_save_area (cfun)));
	}
    }
#endif

#ifdef HAVE_builtin_setjmp_receiver
  if (HAVE_builtin_setjmp_receiver)
    emit_insn (gen_builtin_setjmp_receiver (receiver_label));
  else
#endif
#ifdef HAVE_nonlocal_goto_receiver
    if (HAVE_nonlocal_goto_receiver)
      emit_insn (gen_nonlocal_goto_receiver ());
    else
#endif
      { /* Nothing */ }

  /* @@@ This is a kludge.  Not all machine descriptions define a blockage
     insn, but we must not allow the code we just generated to be reordered
     by scheduling.  Specifically, the update of the frame pointer must
     happen immediately, not later.  So emit an ASM_INPUT to act as blockage
     insn.  */
  emit_insn (gen_rtx_ASM_INPUT (VOIDmode, ""));
}

/* __builtin_setjmp is passed a pointer to an array of five words (not
   all will be used on all machines).  It operates similarly to the C
   library function of the same name, but is more efficient.  Much of
   the code below (and for longjmp) is copied from the handling of
   non-local gotos.

   NOTE: This is intended for use by GNAT and the exception handling
   scheme in the compiler and will only work in the method used by
   them.  */

static rtx
expand_builtin_setjmp (arglist, target)
     tree arglist;
     rtx target;
{
  rtx buf_addr, next_lab, cont_lab;

  if (!validate_arglist (arglist, POINTER_TYPE, VOID_TYPE))
    return NULL_RTX;

  if (target == 0 || GET_CODE (target) != REG
      || REGNO (target) < FIRST_PSEUDO_REGISTER)
    target = gen_reg_rtx (TYPE_MODE (integer_type_node));

  buf_addr = expand_expr (TREE_VALUE (arglist), NULL_RTX, VOIDmode, 0);

  next_lab = gen_label_rtx ();
  cont_lab = gen_label_rtx ();

  expand_builtin_setjmp_setup (buf_addr, next_lab);

  /* Set TARGET to zero and branch to the continue label.  */
  emit_move_insn (target, const0_rtx);
  emit_jump_insn (gen_jump (cont_lab));
  emit_barrier ();
  emit_label (next_lab);

  expand_builtin_setjmp_receiver (next_lab);

  /* Set TARGET to one.  */
  emit_move_insn (target, const1_rtx);
  emit_label (cont_lab);

  /* Tell flow about the strange goings on.  Putting `next_lab' on
     `nonlocal_goto_handler_labels' to indicates that function
     calls may traverse the arc back to this label.  */

  current_function_has_nonlocal_label = 1;
  nonlocal_goto_handler_labels
    = gen_rtx_EXPR_LIST (VOIDmode, next_lab, nonlocal_goto_handler_labels);

  return target;
}

/* __builtin_longjmp is passed a pointer to an array of five words (not
   all will be used on all machines).  It operates similarly to the C
   library function of the same name, but is more efficient.  Much of
   the code below is copied from the handling of non-local gotos.

   NOTE: This is intended for use by GNAT and the exception handling
   scheme in the compiler and will only work in the method used by
   them.  */

void
expand_builtin_longjmp (buf_addr, value)
     rtx buf_addr, value;
{
  rtx fp, lab, stack, insn;
  enum machine_mode sa_mode = STACK_SAVEAREA_MODE (SAVE_NONLOCAL);

  if (setjmp_alias_set == -1)
    setjmp_alias_set = new_alias_set ();

#ifdef POINTERS_EXTEND_UNSIGNED
  if (GET_MODE (buf_addr) != Pmode)
    buf_addr = convert_memory_address (Pmode, buf_addr);
#endif

  buf_addr = force_reg (Pmode, buf_addr);

  /* We used to store value in static_chain_rtx, but that fails if pointers
     are smaller than integers.  We instead require that the user must pass
     a second argument of 1, because that is what builtin_setjmp will
     return.  This also makes EH slightly more efficient, since we are no
     longer copying around a value that we don't care about.  */
  if (value != const1_rtx)
    abort ();

  current_function_calls_longjmp = 1;

#ifdef HAVE_builtin_longjmp
  if (HAVE_builtin_longjmp)
    emit_insn (gen_builtin_longjmp (buf_addr));
  else
#endif
    {
      fp = gen_rtx_MEM (Pmode, buf_addr);
      lab = gen_rtx_MEM (Pmode, plus_constant (buf_addr,
					       GET_MODE_SIZE (Pmode)));

      stack = gen_rtx_MEM (sa_mode, plus_constant (buf_addr,
						   2 * GET_MODE_SIZE (Pmode)));
      set_mem_alias_set (fp, setjmp_alias_set);
      set_mem_alias_set (lab, setjmp_alias_set);
      set_mem_alias_set (stack, setjmp_alias_set);

      /* Pick up FP, label, and SP from the block and jump.  This code is
	 from expand_goto in stmt.c; see there for detailed comments.  */
#if HAVE_nonlocal_goto
      if (HAVE_nonlocal_goto)
	/* We have to pass a value to the nonlocal_goto pattern that will
	   get copied into the static_chain pointer, but it does not matter
	   what that value is, because builtin_setjmp does not use it.  */
	emit_insn (gen_nonlocal_goto (value, lab, stack, fp));
      else
#endif
	{
	  lab = copy_to_reg (lab);

	  emit_move_insn (hard_frame_pointer_rtx, fp);
	  emit_stack_restore (SAVE_NONLOCAL, stack, NULL_RTX);

	  emit_insn (gen_rtx_USE (VOIDmode, hard_frame_pointer_rtx));
	  emit_insn (gen_rtx_USE (VOIDmode, stack_pointer_rtx));
	  emit_indirect_jump (lab);
	}
    }

  /* Search backwards and mark the jump insn as a non-local goto.
     Note that this precludes the use of __builtin_longjmp to a
     __builtin_setjmp target in the same function.  However, we've
     already cautioned the user that these functions are for
     internal exception handling use only.  */
  for (insn = get_last_insn (); insn; insn = PREV_INSN (insn))
    {
      if (GET_CODE (insn) == JUMP_INSN)
	{
	  REG_NOTES (insn) = alloc_EXPR_LIST (REG_NON_LOCAL_GOTO, const0_rtx,
					      REG_NOTES (insn));
	  break;
	}
      else if (GET_CODE (insn) == CALL_INSN)
        break;
    }
}

/* Expand a call to __builtin_prefetch.  For a target that does not support
   data prefetch, evaluate the memory address argument in case it has side
   effects.  */

static void
expand_builtin_prefetch (arglist)
     tree arglist;
{
  tree arg0, arg1, arg2;
  rtx op0, op1, op2;

  if (!validate_arglist (arglist, POINTER_TYPE, 0))
    return;

  arg0 = TREE_VALUE (arglist);
  /* Arguments 1 and 2 are optional; argument 1 (read/write) defaults to
     zero (read) and argument 2 (locality) defaults to 3 (high degree of
     locality).  */
  if (TREE_CHAIN (arglist))
    {
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      if (TREE_CHAIN (TREE_CHAIN (arglist)))
        arg2 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      else
	arg2 = build_int_2 (3, 0);
    }
  else
    {
      arg1 = integer_zero_node;
      arg2 = build_int_2 (3, 0);
    }

  /* Argument 0 is an address.  */
  op0 = expand_expr (arg0, NULL_RTX, Pmode, EXPAND_NORMAL);

  /* Argument 1 (read/write flag) must be a compile-time constant int.  */
  if (TREE_CODE (arg1) != INTEGER_CST)
    {
       error ("second arg to `__builtin_prefetch' must be a constant");
       arg1 = integer_zero_node;
    }
  op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
  /* Argument 1 must be either zero or one.  */
  if (INTVAL (op1) != 0 && INTVAL (op1) != 1)
    {
      warning ("invalid second arg to __builtin_prefetch; using zero");
      op1 = const0_rtx;
    }

  /* Argument 2 (locality) must be a compile-time constant int.  */
  if (TREE_CODE (arg2) != INTEGER_CST)
    {
      error ("third arg to `__builtin_prefetch' must be a constant");
      arg2 = integer_zero_node;
    }
  op2 = expand_expr (arg2, NULL_RTX, VOIDmode, 0);
  /* Argument 2 must be 0, 1, 2, or 3.  */
  if (INTVAL (op2) < 0 || INTVAL (op2) > 3)
    {
      warning ("invalid third arg to __builtin_prefetch; using zero");
      op2 = const0_rtx;
    }

#ifdef HAVE_prefetch
  if (HAVE_prefetch)
    {
      if (! (*insn_data[(int)CODE_FOR_prefetch].operand[0].predicate)
	    (op0,
	     insn_data[(int)CODE_FOR_prefetch].operand[0].mode))
        op0 = force_reg (Pmode, op0);
      emit_insn (gen_prefetch (op0, op1, op2));
    }
  else
#endif
    op0 = protect_from_queue (op0, 0);
    /* Don't do anything with direct references to volatile memory, but
       generate code to handle other side effects.  */
    if (GET_CODE (op0) != MEM && side_effects_p (op0))
      emit_insn (op0);
}

/* Get a MEM rtx for expression EXP which is the address of an operand
   to be used to be used in a string instruction (cmpstrsi, movstrsi, ..).  */

static rtx
get_memory_rtx (exp)
     tree exp;
{
  rtx addr = expand_expr (exp, NULL_RTX, ptr_mode, EXPAND_SUM);
  rtx mem;

#ifdef POINTERS_EXTEND_UNSIGNED
  if (GET_MODE (addr) != Pmode)
    addr = convert_memory_address (Pmode, addr);
#endif

  mem = gen_rtx_MEM (BLKmode, memory_address (BLKmode, addr));

  /* Get an expression we can use to find the attributes to assign to MEM.
     If it is an ADDR_EXPR, use the operand.  Otherwise, dereference it if
     we can.  First remove any nops.  */
  while ((TREE_CODE (exp) == NOP_EXPR || TREE_CODE (exp) == CONVERT_EXPR
	 || TREE_CODE (exp) == NON_LVALUE_EXPR)
	 && POINTER_TYPE_P (TREE_TYPE (TREE_OPERAND (exp, 0))))
    exp = TREE_OPERAND (exp, 0);

  if (TREE_CODE (exp) == ADDR_EXPR)
    {
      exp = TREE_OPERAND (exp, 0);
      set_mem_attributes (mem, exp, 0);
    }
  else if (POINTER_TYPE_P (TREE_TYPE (exp)))
    {
      exp = build1 (INDIRECT_REF, TREE_TYPE (TREE_TYPE (exp)), exp);
      /* memcpy, memset and other builtin stringops can alias with anything.  */
      set_mem_alias_set (mem, 0);
    }

  return mem;
}

/* Built-in functions to perform an untyped call and return.  */

/* For each register that may be used for calling a function, this
   gives a mode used to copy the register's value.  VOIDmode indicates
   the register is not used for calling a function.  If the machine
   has register windows, this gives only the outbound registers.
   INCOMING_REGNO gives the corresponding inbound register.  */
static enum machine_mode apply_args_mode[FIRST_PSEUDO_REGISTER];

/* For each register that may be used for returning values, this gives
   a mode used to copy the register's value.  VOIDmode indicates the
   register is not used for returning values.  If the machine has
   register windows, this gives only the outbound registers.
   INCOMING_REGNO gives the corresponding inbound register.  */
static enum machine_mode apply_result_mode[FIRST_PSEUDO_REGISTER];

/* For each register that may be used for calling a function, this
   gives the offset of that register into the block returned by
   __builtin_apply_args.  0 indicates that the register is not
   used for calling a function.  */
static int apply_args_reg_offset[FIRST_PSEUDO_REGISTER];

/* Return the offset of register REGNO into the block returned by
   __builtin_apply_args.  This is not declared static, since it is
   needed in objc-act.c.  */

int
apply_args_register_offset (regno)
     int regno;
{
  apply_args_size ();

  /* Arguments are always put in outgoing registers (in the argument
     block) if such make sense.  */
#ifdef OUTGOING_REGNO
  regno = OUTGOING_REGNO(regno);
#endif
  return apply_args_reg_offset[regno];
}

/* Return the size required for the block returned by __builtin_apply_args,
   and initialize apply_args_mode.  */

static int
apply_args_size ()
{
  static int size = -1;
  int align;
  unsigned int regno;
  enum machine_mode mode;

  /* The values computed by this function never change.  */
  if (size < 0)
    {
      /* The first value is the incoming arg-pointer.  */
      size = GET_MODE_SIZE (Pmode);

      /* The second value is the structure value address unless this is
	 passed as an "invisible" first argument.  */
      if (struct_value_rtx)
	size += GET_MODE_SIZE (Pmode);

      for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
	if (FUNCTION_ARG_REGNO_P (regno))
	  {
	    /* Search for the proper mode for copying this register's
	       value.  I'm not sure this is right, but it works so far.  */
	    enum machine_mode best_mode = VOIDmode;

	    for (mode = GET_CLASS_NARROWEST_MODE (MODE_INT);
		 mode != VOIDmode;
		 mode = GET_MODE_WIDER_MODE (mode))
	      if (HARD_REGNO_MODE_OK (regno, mode)
		  && HARD_REGNO_NREGS (regno, mode) == 1)
		best_mode = mode;

	    if (best_mode == VOIDmode)
	      for (mode = GET_CLASS_NARROWEST_MODE (MODE_FLOAT);
		   mode != VOIDmode;
		   mode = GET_MODE_WIDER_MODE (mode))
		if (HARD_REGNO_MODE_OK (regno, mode)
		    && have_insn_for (SET, mode))
		  best_mode = mode;

	    if (best_mode == VOIDmode)
	      for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_FLOAT);
		   mode != VOIDmode;
		   mode = GET_MODE_WIDER_MODE (mode))
		if (HARD_REGNO_MODE_OK (regno, mode)
		    && have_insn_for (SET, mode))
		  best_mode = mode;

	    if (best_mode == VOIDmode)
	      for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_INT);
		   mode != VOIDmode;
		   mode = GET_MODE_WIDER_MODE (mode))
		if (HARD_REGNO_MODE_OK (regno, mode)
		    && have_insn_for (SET, mode))
		  best_mode = mode;

	    mode = best_mode;
	    if (mode == VOIDmode)
	      abort ();

	    align = GET_MODE_ALIGNMENT (mode) / BITS_PER_UNIT;
	    if (size % align != 0)
	      size = CEIL (size, align) * align;
	    apply_args_reg_offset[regno] = size;
	    size += GET_MODE_SIZE (mode);
	    apply_args_mode[regno] = mode;
	  }
	else
	  {
	    apply_args_mode[regno] = VOIDmode;
	    apply_args_reg_offset[regno] = 0;
	  }
    }
  return size;
}

/* Return the size required for the block returned by __builtin_apply,
   and initialize apply_result_mode.  */

static int
apply_result_size ()
{
  static int size = -1;
  int align, regno;
  enum machine_mode mode;

  /* The values computed by this function never change.  */
  if (size < 0)
    {
      size = 0;

      for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
	if (FUNCTION_VALUE_REGNO_P (regno))
	  {
	    /* Search for the proper mode for copying this register's
	       value.  I'm not sure this is right, but it works so far.  */
	    enum machine_mode best_mode = VOIDmode;

	    for (mode = GET_CLASS_NARROWEST_MODE (MODE_INT);
		 mode != TImode;
		 mode = GET_MODE_WIDER_MODE (mode))
	      if (HARD_REGNO_MODE_OK (regno, mode))
		best_mode = mode;

	    if (best_mode == VOIDmode)
	      for (mode = GET_CLASS_NARROWEST_MODE (MODE_FLOAT);
		   mode != VOIDmode;
		   mode = GET_MODE_WIDER_MODE (mode))
		if (HARD_REGNO_MODE_OK (regno, mode)
		    && have_insn_for (SET, mode))
		  best_mode = mode;

	    if (best_mode == VOIDmode)
	      for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_FLOAT);
		   mode != VOIDmode;
		   mode = GET_MODE_WIDER_MODE (mode))
		if (HARD_REGNO_MODE_OK (regno, mode)
		    && have_insn_for (SET, mode))
		      best_mode = mode;

	    if (best_mode == VOIDmode)
	      for (mode = GET_CLASS_NARROWEST_MODE (MODE_VECTOR_INT);
		   mode != VOIDmode;
		   mode = GET_MODE_WIDER_MODE (mode))
		if (HARD_REGNO_MODE_OK (regno, mode)
		    && have_insn_for (SET, mode))
		  best_mode = mode;

	    mode = best_mode;
	    if (mode == VOIDmode)
	      abort ();

	    align = GET_MODE_ALIGNMENT (mode) / BITS_PER_UNIT;
	    if (size % align != 0)
	      size = CEIL (size, align) * align;
	    size += GET_MODE_SIZE (mode);
	    apply_result_mode[regno] = mode;
	  }
	else
	  apply_result_mode[regno] = VOIDmode;

      /* Allow targets that use untyped_call and untyped_return to override
	 the size so that machine-specific information can be stored here.  */
#ifdef APPLY_RESULT_SIZE
      size = APPLY_RESULT_SIZE;
#endif
    }
  return size;
}

#if defined (HAVE_untyped_call) || defined (HAVE_untyped_return)
/* Create a vector describing the result block RESULT.  If SAVEP is true,
   the result block is used to save the values; otherwise it is used to
   restore the values.  */

static rtx
result_vector (savep, result)
     int savep;
     rtx result;
{
  int regno, size, align, nelts;
  enum machine_mode mode;
  rtx reg, mem;
  rtx *savevec = (rtx *) alloca (FIRST_PSEUDO_REGISTER * sizeof (rtx));

  size = nelts = 0;
  for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    if ((mode = apply_result_mode[regno]) != VOIDmode)
      {
	align = GET_MODE_ALIGNMENT (mode) / BITS_PER_UNIT;
	if (size % align != 0)
	  size = CEIL (size, align) * align;
	reg = gen_rtx_REG (mode, savep ? regno : INCOMING_REGNO (regno));
	mem = adjust_address (result, mode, size);
	savevec[nelts++] = (savep
			    ? gen_rtx_SET (VOIDmode, mem, reg)
			    : gen_rtx_SET (VOIDmode, reg, mem));
	size += GET_MODE_SIZE (mode);
      }
  return gen_rtx_PARALLEL (VOIDmode, gen_rtvec_v (nelts, savevec));
}
#endif /* HAVE_untyped_call or HAVE_untyped_return */

/* Save the state required to perform an untyped call with the same
   arguments as were passed to the current function.  */

static rtx
expand_builtin_apply_args_1 ()
{
  rtx registers;
  int size, align, regno;
  enum machine_mode mode;

  /* Create a block where the arg-pointer, structure value address,
     and argument registers can be saved.  */
  registers = assign_stack_local (BLKmode, apply_args_size (), -1);

  /* Walk past the arg-pointer and structure value address.  */
  size = GET_MODE_SIZE (Pmode);
  if (struct_value_rtx)
    size += GET_MODE_SIZE (Pmode);

  /* Save each register used in calling a function to the block.  */
  for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    if ((mode = apply_args_mode[regno]) != VOIDmode)
      {
	rtx tem;

	align = GET_MODE_ALIGNMENT (mode) / BITS_PER_UNIT;
	if (size % align != 0)
	  size = CEIL (size, align) * align;

	tem = gen_rtx_REG (mode, INCOMING_REGNO (regno));

	emit_move_insn (adjust_address (registers, mode, size), tem);
	size += GET_MODE_SIZE (mode);
      }

  /* Save the arg pointer to the block.  */
  emit_move_insn (adjust_address (registers, Pmode, 0),
		  copy_to_reg (virtual_incoming_args_rtx));
  size = GET_MODE_SIZE (Pmode);

  /* Save the structure value address unless this is passed as an
     "invisible" first argument.  */
  if (struct_value_incoming_rtx)
    {
      emit_move_insn (adjust_address (registers, Pmode, size),
		      copy_to_reg (struct_value_incoming_rtx));
      size += GET_MODE_SIZE (Pmode);
    }

  /* Return the address of the block.  */
  return copy_addr_to_reg (XEXP (registers, 0));
}

/* __builtin_apply_args returns block of memory allocated on
   the stack into which is stored the arg pointer, structure
   value address, static chain, and all the registers that might
   possibly be used in performing a function call.  The code is
   moved to the start of the function so the incoming values are
   saved.  */

static rtx
expand_builtin_apply_args ()
{
  /* Don't do __builtin_apply_args more than once in a function.
     Save the result of the first call and reuse it.  */
  if (apply_args_value != 0)
    return apply_args_value;
  {
    /* When this function is called, it means that registers must be
       saved on entry to this function.  So we migrate the
       call to the first insn of this function.  */
    rtx temp;
    rtx seq;

    start_sequence ();
    temp = expand_builtin_apply_args_1 ();
    seq = get_insns ();
    end_sequence ();

    apply_args_value = temp;

    /* Put the sequence after the NOTE that starts the function.
       If this is inside a SEQUENCE, make the outer-level insn
       chain current, so the code is placed at the start of the
       function.  */
    push_topmost_sequence ();
    emit_insns_before (seq, NEXT_INSN (get_insns ()));
    pop_topmost_sequence ();
    return temp;
  }
}

/* Perform an untyped call and save the state required to perform an
   untyped return of whatever value was returned by the given function.  */

static rtx
expand_builtin_apply (function, arguments, argsize)
     rtx function, arguments, argsize;
{
  int size, align, regno;
  enum machine_mode mode;
  rtx incoming_args, result, reg, dest, src, call_insn;
  rtx old_stack_level = 0;
  rtx call_fusage = 0;

#ifdef POINTERS_EXTEND_UNSIGNED
  if (GET_MODE (arguments) != Pmode)
    arguments = convert_memory_address (Pmode, arguments);
#endif

  /* Create a block where the return registers can be saved.  */
  result = assign_stack_local (BLKmode, apply_result_size (), -1);

  /* Fetch the arg pointer from the ARGUMENTS block.  */
  incoming_args = gen_reg_rtx (Pmode);
  emit_move_insn (incoming_args, gen_rtx_MEM (Pmode, arguments));
#ifndef STACK_GROWS_DOWNWARD
  incoming_args = expand_simple_binop (Pmode, MINUS, incoming_args, argsize,
				       incoming_args, 0, OPTAB_LIB_WIDEN);
#endif

  /* Perform postincrements before actually calling the function.  */
  emit_queue ();

  /* Push a new argument block and copy the arguments.  Do not allow
     the (potential) memcpy call below to interfere with our stack
     manipulations.  */
  do_pending_stack_adjust ();
  NO_DEFER_POP;

  /* Save the stack with nonlocal if available */
#ifdef HAVE_save_stack_nonlocal
  if (HAVE_save_stack_nonlocal)
    emit_stack_save (SAVE_NONLOCAL, &old_stack_level, NULL_RTX);
  else
#endif
    emit_stack_save (SAVE_BLOCK, &old_stack_level, NULL_RTX);

  /* Push a block of memory onto the stack to store the memory arguments.
     Save the address in a register, and copy the memory arguments.  ??? I
     haven't figured out how the calling convention macros effect this,
     but it's likely that the source and/or destination addresses in
     the block copy will need updating in machine specific ways.  */
  dest = allocate_dynamic_stack_space (argsize, 0, BITS_PER_UNIT);
  dest = gen_rtx_MEM (BLKmode, dest);
  set_mem_align (dest, PARM_BOUNDARY);
  src = gen_rtx_MEM (BLKmode, incoming_args);
  set_mem_align (src, PARM_BOUNDARY);
  emit_block_move (dest, src, argsize);

  /* Refer to the argument block.  */
  apply_args_size ();
  arguments = gen_rtx_MEM (BLKmode, arguments);
  set_mem_align (arguments, PARM_BOUNDARY);

  /* Walk past the arg-pointer and structure value address.  */
  size = GET_MODE_SIZE (Pmode);
  if (struct_value_rtx)
    size += GET_MODE_SIZE (Pmode);

  /* Restore each of the registers previously saved.  Make USE insns
     for each of these registers for use in making the call.  */
  for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    if ((mode = apply_args_mode[regno]) != VOIDmode)
      {
	align = GET_MODE_ALIGNMENT (mode) / BITS_PER_UNIT;
	if (size % align != 0)
	  size = CEIL (size, align) * align;
	reg = gen_rtx_REG (mode, regno);
	emit_move_insn (reg, adjust_address (arguments, mode, size));
	use_reg (&call_fusage, reg);
	size += GET_MODE_SIZE (mode);
      }

  /* Restore the structure value address unless this is passed as an
     "invisible" first argument.  */
  size = GET_MODE_SIZE (Pmode);
  if (struct_value_rtx)
    {
      rtx value = gen_reg_rtx (Pmode);
      emit_move_insn (value, adjust_address (arguments, Pmode, size));
      emit_move_insn (struct_value_rtx, value);
      if (GET_CODE (struct_value_rtx) == REG)
	  use_reg (&call_fusage, struct_value_rtx);
      size += GET_MODE_SIZE (Pmode);
    }

  /* All arguments and registers used for the call are set up by now!  */
  function = prepare_call_address (function, NULL_TREE, &call_fusage, 0, 0);

  /* Ensure address is valid.  SYMBOL_REF is already valid, so no need,
     and we don't want to load it into a register as an optimization,
     because prepare_call_address already did it if it should be done.  */
  if (GET_CODE (function) != SYMBOL_REF)
    function = memory_address (FUNCTION_MODE, function);

  /* Generate the actual call instruction and save the return value.  */
#ifdef HAVE_untyped_call
  if (HAVE_untyped_call)
    emit_call_insn (gen_untyped_call (gen_rtx_MEM (FUNCTION_MODE, function),
				      result, result_vector (1, result)));
  else
#endif
#ifdef HAVE_call_value
  if (HAVE_call_value)
    {
      rtx valreg = 0;

      /* Locate the unique return register.  It is not possible to
	 express a call that sets more than one return register using
	 call_value; use untyped_call for that.  In fact, untyped_call
	 only needs to save the return registers in the given block.  */
      for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
	if ((mode = apply_result_mode[regno]) != VOIDmode)
	  {
	    if (valreg)
	      abort (); /* HAVE_untyped_call required.  */
	    valreg = gen_rtx_REG (mode, regno);
	  }

      emit_call_insn (GEN_CALL_VALUE (valreg,
				      gen_rtx_MEM (FUNCTION_MODE, function),
				      const0_rtx, NULL_RTX, const0_rtx));

      emit_move_insn (adjust_address (result, GET_MODE (valreg), 0), valreg);
    }
  else
#endif
    abort ();

  /* Find the CALL insn we just emitted.  */
  for (call_insn = get_last_insn ();
       call_insn && GET_CODE (call_insn) != CALL_INSN;
       call_insn = PREV_INSN (call_insn))
    ;

  if (! call_insn)
    abort ();

  /* Put the register usage information on the CALL.  If there is already
     some usage information, put ours at the end.  */
  if (CALL_INSN_FUNCTION_USAGE (call_insn))
    {
      rtx link;

      for (link = CALL_INSN_FUNCTION_USAGE (call_insn); XEXP (link, 1) != 0;
	   link = XEXP (link, 1))
	;

      XEXP (link, 1) = call_fusage;
    }
  else
    CALL_INSN_FUNCTION_USAGE (call_insn) = call_fusage;

  /* Restore the stack.  */
#ifdef HAVE_save_stack_nonlocal
  if (HAVE_save_stack_nonlocal)
    emit_stack_restore (SAVE_NONLOCAL, old_stack_level, NULL_RTX);
  else
#endif
    emit_stack_restore (SAVE_BLOCK, old_stack_level, NULL_RTX);

  OK_DEFER_POP;

  /* Return the address of the result block.  */
  return copy_addr_to_reg (XEXP (result, 0));
}

/* Perform an untyped return.  */

static void
expand_builtin_return (result)
     rtx result;
{
  int size, align, regno;
  enum machine_mode mode;
  rtx reg;
  rtx call_fusage = 0;

#ifdef POINTERS_EXTEND_UNSIGNED
  if (GET_MODE (result) != Pmode)
    result = convert_memory_address (Pmode, result);
#endif

  apply_result_size ();
  result = gen_rtx_MEM (BLKmode, result);

#ifdef HAVE_untyped_return
  if (HAVE_untyped_return)
    {
      emit_jump_insn (gen_untyped_return (result, result_vector (0, result)));
      emit_barrier ();
      return;
    }
#endif

  /* Restore the return value and note that each value is used.  */
  size = 0;
  for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    if ((mode = apply_result_mode[regno]) != VOIDmode)
      {
	align = GET_MODE_ALIGNMENT (mode) / BITS_PER_UNIT;
	if (size % align != 0)
	  size = CEIL (size, align) * align;
	reg = gen_rtx_REG (mode, INCOMING_REGNO (regno));
	emit_move_insn (reg, adjust_address (result, mode, size));

	push_to_sequence (call_fusage);
	emit_insn (gen_rtx_USE (VOIDmode, reg));
	call_fusage = get_insns ();
	end_sequence ();
	size += GET_MODE_SIZE (mode);
      }

  /* Put the USE insns before the return.  */
  emit_insns (call_fusage);

  /* Return whatever values was restored by jumping directly to the end
     of the function.  */
  expand_null_return ();
}

/* Used by expand_builtin_classify_type and fold_builtin_classify_type.  */

static enum type_class
type_to_class (type)
     tree type;
{
  switch (TREE_CODE (type))
    {
    case VOID_TYPE:	   return void_type_class;
    case INTEGER_TYPE:	   return integer_type_class;
    case CHAR_TYPE:	   return char_type_class;
    case ENUMERAL_TYPE:	   return enumeral_type_class;
    case BOOLEAN_TYPE:	   return boolean_type_class;
    case POINTER_TYPE:	   return pointer_type_class;
    case REFERENCE_TYPE:   return reference_type_class;
    case OFFSET_TYPE:	   return offset_type_class;
    case REAL_TYPE:	   return real_type_class;
    case COMPLEX_TYPE:	   return complex_type_class;
    case FUNCTION_TYPE:	   return function_type_class;
    case METHOD_TYPE:	   return method_type_class;
    case RECORD_TYPE:	   return record_type_class;
    case UNION_TYPE:
    case QUAL_UNION_TYPE:  return union_type_class;
    case ARRAY_TYPE:	   return (TYPE_STRING_FLAG (type)
				   ? string_type_class : array_type_class);
    case SET_TYPE:	   return set_type_class;
    case FILE_TYPE:	   return file_type_class;
    case LANG_TYPE:	   return lang_type_class;
    default:		   return no_type_class;
    }
}

/* Expand a call to __builtin_classify_type with arguments found in
   ARGLIST.  */

static rtx
expand_builtin_classify_type (arglist)
     tree arglist;
{
  if (arglist != 0)
    return GEN_INT (type_to_class (TREE_TYPE (TREE_VALUE (arglist))));
  return GEN_INT (no_type_class);
}

/* Expand expression EXP, which is a call to __builtin_constant_p.  */

static rtx
expand_builtin_constant_p (exp)
     tree exp;
{
  tree arglist = TREE_OPERAND (exp, 1);
  enum machine_mode value_mode = TYPE_MODE (TREE_TYPE (exp));
  rtx tmp;

  if (arglist == 0)
    return const0_rtx;
  arglist = TREE_VALUE (arglist);

  /* We have taken care of the easy cases during constant folding.  This
     case is not obvious, so emit (constant_p_rtx (ARGLIST)) and let CSE get a
     chance to see if it can deduce whether ARGLIST is constant.  */

  tmp = expand_expr (arglist, NULL_RTX, VOIDmode, 0);
  tmp = gen_rtx_CONSTANT_P_RTX (value_mode, tmp);
  return tmp;
}

/* Expand a call to one of the builtin math functions (sin, cos, or sqrt).
   Return 0 if a normal call should be emitted rather than expanding the
   function in-line.  EXP is the expression that is a call to the builtin
   function; if convenient, the result should be placed in TARGET.
   SUBTARGET may be used as the target for computing one of EXP's operands.  */

static rtx
expand_builtin_mathfn (exp, target, subtarget)
     tree exp;
     rtx target, subtarget;
{
  optab builtin_optab;
  rtx op0, insns;
  tree fndecl = TREE_OPERAND (TREE_OPERAND (exp, 0), 0);
  tree arglist = TREE_OPERAND (exp, 1);

  if (!validate_arglist (arglist, REAL_TYPE, VOID_TYPE))
    return 0;

  /* Stabilize and compute the argument.  */
  if (TREE_CODE (TREE_VALUE (arglist)) != VAR_DECL
      && TREE_CODE (TREE_VALUE (arglist)) != PARM_DECL)
    {
      exp = copy_node (exp);
      TREE_OPERAND (exp, 1) = arglist;
      /* Wrap the computation of the argument in a SAVE_EXPR.  That
	 way, if we need to expand the argument again (as in the
	 flag_errno_math case below where we cannot directly set
	 errno), we will not perform side-effects more than once.
	 Note that here we're mutating the original EXP as well as the
	 copy; that's the right thing to do in case the original EXP
	 is expanded later.  */
      TREE_VALUE (arglist) = save_expr (TREE_VALUE (arglist));
      arglist = copy_node (arglist);
    }
  op0 = expand_expr (TREE_VALUE (arglist), subtarget, VOIDmode, 0);

  /* Make a suitable register to place result in.  */
  target = gen_reg_rtx (TYPE_MODE (TREE_TYPE (exp)));

  emit_queue ();
  start_sequence ();

  switch (DECL_FUNCTION_CODE (fndecl))
    {
    case BUILT_IN_SIN:
    case BUILT_IN_SINF:
    case BUILT_IN_SINL:
      builtin_optab = sin_optab; break;
    case BUILT_IN_COS:
    case BUILT_IN_COSF:
    case BUILT_IN_COSL:
      builtin_optab = cos_optab; break;
    case BUILT_IN_SQRT:
    case BUILT_IN_SQRTF:
    case BUILT_IN_SQRTL:
      builtin_optab = sqrt_optab; break;
     default:
      abort ();
    }

  /* Compute into TARGET.
     Set TARGET to wherever the result comes back.  */
  target = expand_unop (TYPE_MODE (TREE_TYPE (TREE_VALUE (arglist))),
			builtin_optab, op0, target, 0);

  /* If we were unable to expand via the builtin, stop the
     sequence (without outputting the insns) and return 0, causing
     a call to the library function.  */
  if (target == 0)
    {
      end_sequence ();
      return 0;
    }

  /* If errno must be maintained and if we are not allowing unsafe
     math optimizations, check the result.  */

  if (flag_errno_math && ! flag_unsafe_math_optimizations)
    {
      rtx lab1;

      /* Don't define the builtin FP instructions
	 if your machine is not IEEE.  */
      if (TARGET_FLOAT_FORMAT != IEEE_FLOAT_FORMAT)
	abort ();

      lab1 = gen_label_rtx ();

      /* Test the result; if it is NaN, set errno=EDOM because
	 the argument was not in the domain.  */
      emit_cmp_and_jump_insns (target, target, EQ, 0, GET_MODE (target),
			       0, lab1);

#ifdef TARGET_EDOM
	{
#ifdef GEN_ERRNO_RTX
	  rtx errno_rtx = GEN_ERRNO_RTX;
#else
	  rtx errno_rtx
	    = gen_rtx_MEM (word_mode, gen_rtx_SYMBOL_REF (Pmode, "errno"));
#endif

	  emit_move_insn (errno_rtx, GEN_INT (TARGET_EDOM));
	}
#else
      /* We can't set errno=EDOM directly; let the library call do it.
	 Pop the arguments right away in case the call gets deleted.  */
      NO_DEFER_POP;
      expand_call (exp, target, 0);
      OK_DEFER_POP;
#endif

      emit_label (lab1);
    }

  /* Output the entire sequence.  */
  insns = get_insns ();
  end_sequence ();
  emit_insns (insns);

  return target;
}

/* Expand expression EXP which is a call to the strlen builtin.  Return 0
   if we failed the caller should emit a normal call, otherwise
   try to get the result in TARGET, if convenient.  */

static rtx
expand_builtin_strlen (exp, target)
     tree exp;
     rtx target;
{
  tree arglist = TREE_OPERAND (exp, 1);
  enum machine_mode value_mode = TYPE_MODE (TREE_TYPE (exp));

  if (!validate_arglist (arglist, POINTER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      rtx pat;
      tree src = TREE_VALUE (arglist);

      int align
	= get_pointer_alignment (src, BIGGEST_ALIGNMENT) / BITS_PER_UNIT;

      rtx result, src_reg, char_rtx, before_strlen;
      enum machine_mode insn_mode = value_mode, char_mode;
      enum insn_code icode = CODE_FOR_nothing;

      /* If SRC is not a pointer type, don't do this operation inline.  */
      if (align == 0)
	return 0;

      /* Bail out if we can't compute strlen in the right mode.  */
      while (insn_mode != VOIDmode)
	{
	  icode = strlen_optab->handlers[(int) insn_mode].insn_code;
	  if (icode != CODE_FOR_nothing)
	    break;

	  insn_mode = GET_MODE_WIDER_MODE (insn_mode);
	}
      if (insn_mode == VOIDmode)
	return 0;

      /* Make a place to write the result of the instruction.  */
      result = target;
      if (! (result != 0
	     && GET_CODE (result) == REG
	     && GET_MODE (result) == insn_mode
	     && REGNO (result) >= FIRST_PSEUDO_REGISTER))
	result = gen_reg_rtx (insn_mode);

      /* Make a place to hold the source address.  We will not expand
	 the actual source until we are sure that the expansion will
	 not fail -- there are trees that cannot be expanded twice.  */
      src_reg = gen_reg_rtx (Pmode);

      /* Mark the beginning of the strlen sequence so we can emit the
	 source operand later.  */
      before_strlen = get_last_insn();

      char_rtx = const0_rtx;
      char_mode = insn_data[(int) icode].operand[2].mode;
      if (! (*insn_data[(int) icode].operand[2].predicate) (char_rtx,
							    char_mode))
	char_rtx = copy_to_mode_reg (char_mode, char_rtx);

      pat = GEN_FCN (icode) (result, gen_rtx_MEM (BLKmode, src_reg),
			     char_rtx, GEN_INT (align));
      if (! pat)
	return 0;
      emit_insn (pat);

      /* Now that we are assured of success, expand the source.  */
      start_sequence ();
      pat = memory_address (BLKmode,
			    expand_expr (src, src_reg, ptr_mode, EXPAND_SUM));
      if (pat != src_reg)
	emit_move_insn (src_reg, pat);
      pat = gen_sequence ();
      end_sequence ();

      if (before_strlen)
	emit_insn_after (pat, before_strlen);
      else
	emit_insn_before (pat, get_insns ());

      /* Return the value in the proper mode for this function.  */
      if (GET_MODE (result) == value_mode)
	target = result;
      else if (target != 0)
	convert_move (target, result, 0);
      else
	target = convert_to_mode (value_mode, result, 0);

      return target;
    }
}

/* Expand a call to the strstr builtin.  Return 0 if we failed the
   caller should emit a normal call, otherwise try to get the result
   in TARGET, if convenient (and in mode MODE if that's convenient).  */

static rtx
expand_builtin_strstr (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist, POINTER_TYPE, POINTER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree s1 = TREE_VALUE (arglist), s2 = TREE_VALUE (TREE_CHAIN (arglist));
      tree fn;
      const char *p1, *p2;

      p2 = c_getstr (s2);
      if (p2 == NULL)
	return 0;

      p1 = c_getstr (s1);
      if (p1 != NULL)
	{
	  const char *r = strstr (p1, p2);

	  if (r == NULL)
	    return const0_rtx;

	  /* Return an offset into the constant string argument.  */
	  return expand_expr (fold (build (PLUS_EXPR, TREE_TYPE (s1),
					   s1, ssize_int (r - p1))),
			      target, mode, EXPAND_NORMAL);
	}

      if (p2[0] == '\0')
	return expand_expr (s1, target, mode, EXPAND_NORMAL);

      if (p2[1] != '\0')
	return 0;

      fn = built_in_decls[BUILT_IN_STRCHR];
      if (!fn)
	return 0;

      /* New argument list transforming strstr(s1, s2) to
	 strchr(s1, s2[0]).  */
      arglist =
	build_tree_list (NULL_TREE, build_int_2 (p2[0], 0));
      arglist = tree_cons (NULL_TREE, s1, arglist);
      return expand_expr (build_function_call_expr (fn, arglist),
			  target, mode, EXPAND_NORMAL);
    }
}

/* Expand a call to the strchr builtin.  Return 0 if we failed the
   caller should emit a normal call, otherwise try to get the result
   in TARGET, if convenient (and in mode MODE if that's convenient).  */

static rtx
expand_builtin_strchr (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree s1 = TREE_VALUE (arglist), s2 = TREE_VALUE (TREE_CHAIN (arglist));
      const char *p1;

      if (TREE_CODE (s2) != INTEGER_CST)
	return 0;

      p1 = c_getstr (s1);
      if (p1 != NULL)
	{
	  char c;
	  const char *r;

	  if (target_char_cast (s2, &c))
	    return 0;

	  r = strchr (p1, c);

	  if (r == NULL)
	    return const0_rtx;

	  /* Return an offset into the constant string argument.  */
	  return expand_expr (fold (build (PLUS_EXPR, TREE_TYPE (s1),
					   s1, ssize_int (r - p1))),
			      target, mode, EXPAND_NORMAL);
	}

      /* FIXME: Should use here strchrM optab so that ports can optimize
	 this.  */
      return 0;
    }
}

/* Expand a call to the strrchr builtin.  Return 0 if we failed the
   caller should emit a normal call, otherwise try to get the result
   in TARGET, if convenient (and in mode MODE if that's convenient).  */

static rtx
expand_builtin_strrchr (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree s1 = TREE_VALUE (arglist), s2 = TREE_VALUE (TREE_CHAIN (arglist));
      tree fn;
      const char *p1;

      if (TREE_CODE (s2) != INTEGER_CST)
	return 0;

      p1 = c_getstr (s1);
      if (p1 != NULL)
	{
	  char c;
	  const char *r;

	  if (target_char_cast (s2, &c))
	    return 0;

	  r = strrchr (p1, c);

	  if (r == NULL)
	    return const0_rtx;

	  /* Return an offset into the constant string argument.  */
	  return expand_expr (fold (build (PLUS_EXPR, TREE_TYPE (s1),
					   s1, ssize_int (r - p1))),
			      target, mode, EXPAND_NORMAL);
	}

      if (! integer_zerop (s2))
	return 0;

      fn = built_in_decls[BUILT_IN_STRCHR];
      if (!fn)
	return 0;

      /* Transform strrchr(s1, '\0') to strchr(s1, '\0').  */
      return expand_expr (build_function_call_expr (fn, arglist),
			  target, mode, EXPAND_NORMAL);
    }
}

/* Expand a call to the strpbrk builtin.  Return 0 if we failed the
   caller should emit a normal call, otherwise try to get the result
   in TARGET, if convenient (and in mode MODE if that's convenient).  */

static rtx
expand_builtin_strpbrk (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist, POINTER_TYPE, POINTER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree s1 = TREE_VALUE (arglist), s2 = TREE_VALUE (TREE_CHAIN (arglist));
      tree fn;
      const char *p1, *p2;

      p2 = c_getstr (s2);
      if (p2 == NULL)
	return 0;

      p1 = c_getstr (s1);
      if (p1 != NULL)
	{
	  const char *r = strpbrk (p1, p2);

	  if (r == NULL)
	    return const0_rtx;

	  /* Return an offset into the constant string argument.  */
	  return expand_expr (fold (build (PLUS_EXPR, TREE_TYPE (s1),
					   s1, ssize_int (r - p1))),
			      target, mode, EXPAND_NORMAL);
	}

      if (p2[0] == '\0')
	{
	  /* strpbrk(x, "") == NULL.
	     Evaluate and ignore the arguments in case they had
	     side-effects.  */
	  expand_expr (s1, const0_rtx, VOIDmode, EXPAND_NORMAL);
	  return const0_rtx;
	}

      if (p2[1] != '\0')
	return 0;  /* Really call strpbrk.  */

      fn = built_in_decls[BUILT_IN_STRCHR];
      if (!fn)
	return 0;

      /* New argument list transforming strpbrk(s1, s2) to
	 strchr(s1, s2[0]).  */
      arglist =
	build_tree_list (NULL_TREE, build_int_2 (p2[0], 0));
      arglist = tree_cons (NULL_TREE, s1, arglist);
      return expand_expr (build_function_call_expr (fn, arglist),
			  target, mode, EXPAND_NORMAL);
    }
}

/* Callback routine for store_by_pieces.  Read GET_MODE_BITSIZE (MODE)
   bytes from constant string DATA + OFFSET and return it as target
   constant.  */

static rtx
builtin_memcpy_read_str (data, offset, mode)
     PTR data;
     HOST_WIDE_INT offset;
     enum machine_mode mode;
{
  const char *str = (const char *) data;

  if (offset < 0
      || ((unsigned HOST_WIDE_INT) offset + GET_MODE_SIZE (mode)
	  > strlen (str) + 1))
    abort ();  /* Attempt to read past the end of constant string.  */

  return c_readstr (str + offset, mode);
}

/* Expand a call to the memcpy builtin, with arguments in ARGLIST.
   Return 0 if we failed, the caller should emit a normal call, otherwise
   try to get the result in TARGET, if convenient (and in mode MODE if
   that's convenient).  */

static rtx
expand_builtin_memcpy (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist,
			 POINTER_TYPE, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree dest = TREE_VALUE (arglist);
      tree src = TREE_VALUE (TREE_CHAIN (arglist));
      tree len = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      const char *src_str;

      unsigned int src_align = get_pointer_alignment (src, BIGGEST_ALIGNMENT);
      unsigned int dest_align
	= get_pointer_alignment (dest, BIGGEST_ALIGNMENT);
      rtx dest_mem, src_mem, dest_addr, len_rtx;

      /* If DEST is not a pointer type, call the normal function.  */
      if (dest_align == 0)
        return 0;

      /* If the LEN parameter is zero, return DEST.  */
      if (host_integerp (len, 1) && tree_low_cst (len, 1) == 0)
        {
          /* Evaluate and ignore SRC in case it has side-effects.  */
          expand_expr (src, const0_rtx, VOIDmode, EXPAND_NORMAL);
          return expand_expr (dest, target, mode, EXPAND_NORMAL);
        }

      /* If either SRC is not a pointer type, don't do this
         operation in-line.  */
      if (src_align == 0)
        return 0;

      dest_mem = get_memory_rtx (dest);
      set_mem_align (dest_mem, dest_align);
      len_rtx = expand_expr (len, NULL_RTX, VOIDmode, 0);
      src_str = c_getstr (src);

      /* If SRC is a string constant and block move would be done
	 by pieces, we can avoid loading the string from memory
	 and only stored the computed constants.  */
      if (src_str
	  && GET_CODE (len_rtx) == CONST_INT
	  && (unsigned HOST_WIDE_INT) INTVAL (len_rtx) <= strlen (src_str) + 1
	  && can_store_by_pieces (INTVAL (len_rtx), builtin_memcpy_read_str,
				  (PTR) src_str, dest_align))
	{
	  store_by_pieces (dest_mem, INTVAL (len_rtx),
			   builtin_memcpy_read_str,
			   (PTR) src_str, dest_align);
	  return force_operand (XEXP (dest_mem, 0), NULL_RTX);
	}

      src_mem = get_memory_rtx (src);
      set_mem_align (src_mem, src_align);

      /* Copy word part most expediently.  */
      dest_addr = emit_block_move (dest_mem, src_mem, len_rtx);

      if (dest_addr == 0)
	dest_addr = force_operand (XEXP (dest_mem, 0), NULL_RTX);

      return dest_addr;
    }
}

/* Expand expression EXP, which is a call to the strcpy builtin.  Return 0
   if we failed the caller should emit a normal call, otherwise try to get
   the result in TARGET, if convenient (and in mode MODE if that's
   convenient).  */

static rtx
expand_builtin_strcpy (exp, target, mode)
     tree exp;
     rtx target;
     enum machine_mode mode;
{
  tree arglist = TREE_OPERAND (exp, 1);
  tree fn, len;

  if (!validate_arglist (arglist, POINTER_TYPE, POINTER_TYPE, VOID_TYPE))
    return 0;

  fn = built_in_decls[BUILT_IN_MEMCPY];
  if (!fn)
    return 0;

  len = c_strlen (TREE_VALUE (TREE_CHAIN (arglist)));
  if (len == 0)
    return 0;

  len = size_binop (PLUS_EXPR, len, ssize_int (1));
  chainon (arglist, build_tree_list (NULL_TREE, len));
  return expand_expr (build_function_call_expr (fn, arglist),
                      target, mode, EXPAND_NORMAL);
}

/* Callback routine for store_by_pieces.  Read GET_MODE_BITSIZE (MODE)
   bytes from constant string DATA + OFFSET and return it as target
   constant.  */

static rtx
builtin_strncpy_read_str (data, offset, mode)
     PTR data;
     HOST_WIDE_INT offset;
     enum machine_mode mode;
{
  const char *str = (const char *) data;

  if ((unsigned HOST_WIDE_INT) offset > strlen (str))
    return const0_rtx;

  return c_readstr (str + offset, mode);
}

/* Expand expression EXP, which is a call to the strncpy builtin.  Return 0
   if we failed the caller should emit a normal call.  */

static rtx
expand_builtin_strncpy (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist,
			 POINTER_TYPE, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree slen = c_strlen (TREE_VALUE (TREE_CHAIN (arglist)));
      tree len = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      tree fn;

      /* We must be passed a constant len parameter.  */
      if (TREE_CODE (len) != INTEGER_CST)
	return 0;

      /* If the len parameter is zero, return the dst parameter.  */
      if (integer_zerop (len))
        {
	/* Evaluate and ignore the src argument in case it has
           side-effects.  */
	  expand_expr (TREE_VALUE (TREE_CHAIN (arglist)), const0_rtx,
		       VOIDmode, EXPAND_NORMAL);
	  /* Return the dst parameter.  */
	  return expand_expr (TREE_VALUE (arglist), target, mode,
			      EXPAND_NORMAL);
	}

      /* Now, we must be passed a constant src ptr parameter.  */
      if (slen == 0 || TREE_CODE (slen) != INTEGER_CST)
	return 0;

      slen = size_binop (PLUS_EXPR, slen, ssize_int (1));

      /* We're required to pad with trailing zeros if the requested
         len is greater than strlen(s2)+1.  In that case try to
	 use store_by_pieces, if it fails, punt.  */
      if (tree_int_cst_lt (slen, len))
	{
	  tree dest = TREE_VALUE (arglist);
	  unsigned int dest_align
	    = get_pointer_alignment (dest, BIGGEST_ALIGNMENT);
	  const char *p = c_getstr (TREE_VALUE (TREE_CHAIN (arglist)));
	  rtx dest_mem;

	  if (!p || dest_align == 0 || !host_integerp (len, 1)
	      || !can_store_by_pieces (tree_low_cst (len, 1),
				       builtin_strncpy_read_str,
				       (PTR) p, dest_align))
	    return 0;

	  dest_mem = get_memory_rtx (dest);
	  store_by_pieces (dest_mem, tree_low_cst (len, 1),
			   builtin_strncpy_read_str,
			   (PTR) p, dest_align);
	  return force_operand (XEXP (dest_mem, 0), NULL_RTX);
	}

      /* OK transform into builtin memcpy.  */
      fn = built_in_decls[BUILT_IN_MEMCPY];
      if (!fn)
        return 0;
      return expand_expr (build_function_call_expr (fn, arglist),
                          target, mode, EXPAND_NORMAL);
    }
}

/* Callback routine for store_by_pieces.  Read GET_MODE_BITSIZE (MODE)
   bytes from constant string DATA + OFFSET and return it as target
   constant.  */

static rtx
builtin_memset_read_str (data, offset, mode)
     PTR data;
     HOST_WIDE_INT offset ATTRIBUTE_UNUSED;
     enum machine_mode mode;
{
  const char *c = (const char *) data;
  char *p = alloca (GET_MODE_SIZE (mode));

  memset (p, *c, GET_MODE_SIZE (mode));

  return c_readstr (p, mode);
}

/* Expand expression EXP, which is a call to the memset builtin.  Return 0
   if we failed the caller should emit a normal call, otherwise try to get
   the result in TARGET, if convenient (and in mode MODE if that's
   convenient).  */

static rtx
expand_builtin_memset (exp, target, mode)
     tree exp;
     rtx target;
     enum machine_mode mode;
{
  tree arglist = TREE_OPERAND (exp, 1);

  if (!validate_arglist (arglist,
			 POINTER_TYPE, INTEGER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree dest = TREE_VALUE (arglist);
      tree val = TREE_VALUE (TREE_CHAIN (arglist));
      tree len = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      char c;

      unsigned int dest_align
	= get_pointer_alignment (dest, BIGGEST_ALIGNMENT);
      rtx dest_mem, dest_addr, len_rtx;

      /* If DEST is not a pointer type, don't do this
	 operation in-line.  */
      if (dest_align == 0)
	return 0;

      /* If the LEN parameter is zero, return DEST.  */
      if (host_integerp (len, 1) && tree_low_cst (len, 1) == 0)
        {
          /* Evaluate and ignore VAL in case it has side-effects.  */
          expand_expr (val, const0_rtx, VOIDmode, EXPAND_NORMAL);
          return expand_expr (dest, target, mode, EXPAND_NORMAL);
        }

      if (TREE_CODE (val) != INTEGER_CST)
	return 0;

      if (target_char_cast (val, &c))
	return 0;

      if (c)
	{
	  if (!host_integerp (len, 1))
	    return 0;
	  if (!can_store_by_pieces (tree_low_cst (len, 1),
				    builtin_memset_read_str, (PTR) &c,
				    dest_align))
	    return 0;

	  dest_mem = get_memory_rtx (dest);
	  store_by_pieces (dest_mem, tree_low_cst (len, 1),
			   builtin_memset_read_str,
			   (PTR) &c, dest_align);
	  return force_operand (XEXP (dest_mem, 0), NULL_RTX);
	}

      len_rtx = expand_expr (len, NULL_RTX, VOIDmode, 0);

      dest_mem = get_memory_rtx (dest);
      set_mem_align (dest_mem, dest_align);
      dest_addr = clear_storage (dest_mem, len_rtx);

      if (dest_addr == 0)
	dest_addr = force_operand (XEXP (dest_mem, 0), NULL_RTX);

      return dest_addr;
    }
}

/* Expand expression EXP, which is a call to the bzero builtin.  Return 0
   if we failed the caller should emit a normal call.  */

static rtx
expand_builtin_bzero (exp)
     tree exp;
{
  tree arglist = TREE_OPERAND (exp, 1);
  tree dest, size, newarglist;
  rtx result;

  if (!validate_arglist (arglist, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return NULL_RTX;

  dest = TREE_VALUE (arglist);
  size = TREE_VALUE (TREE_CHAIN (arglist));

  /* New argument list transforming bzero(ptr x, int y) to
     memset(ptr x, int 0, size_t y).   This is done this way
     so that if it isn't expanded inline, we fallback to
     calling bzero instead of memset.  */

  newarglist = build_tree_list (NULL_TREE, convert (sizetype, size));
  newarglist = tree_cons (NULL_TREE, integer_zero_node, newarglist);
  newarglist = tree_cons (NULL_TREE, dest, newarglist);

  TREE_OPERAND (exp, 1) = newarglist;
  result = expand_builtin_memset (exp, const0_rtx, VOIDmode);

  /* Always restore the original arguments.  */
  TREE_OPERAND (exp, 1) = arglist;

  return result;
}

/* Expand expression EXP, which is a call to the memcmp or the strcmp builtin.
   ARGLIST is the argument list for this call.  Return 0 if we failed and the
   caller should emit a normal call, otherwise try to get the result in
   TARGET, if convenient (and in mode MODE, if that's convenient).  */

static rtx
expand_builtin_memcmp (exp, arglist, target, mode)
     tree exp ATTRIBUTE_UNUSED;
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  tree arg1, arg2, len;
  const char *p1, *p2;

  if (!validate_arglist (arglist,
		      POINTER_TYPE, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return 0;

  arg1 = TREE_VALUE (arglist);
  arg2 = TREE_VALUE (TREE_CHAIN (arglist));
  len = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));

  /* If the len parameter is zero, return zero.  */
  if (host_integerp (len, 1) && tree_low_cst (len, 1) == 0)
    {
      /* Evaluate and ignore arg1 and arg2 in case they have
         side-effects.  */
      expand_expr (arg1, const0_rtx, VOIDmode, EXPAND_NORMAL);
      expand_expr (arg2, const0_rtx, VOIDmode, EXPAND_NORMAL);
      return const0_rtx;
    }

  p1 = c_getstr (arg1);
  p2 = c_getstr (arg2);

  /* If all arguments are constant, and the value of len is not greater
     than the lengths of arg1 and arg2, evaluate at compile-time.  */
  if (host_integerp (len, 1) && p1 && p2
      && compare_tree_int (len, strlen (p1) + 1) <= 0
      && compare_tree_int (len, strlen (p2) + 1) <= 0)
    {
      const int r = memcmp (p1, p2, tree_low_cst (len, 1));

      return (r < 0 ? constm1_rtx : (r > 0 ? const1_rtx : const0_rtx));
    }

  /* If len parameter is one, return an expression corresponding to
     (*(const unsigned char*)arg1 - (const unsigned char*)arg2).  */
  if (host_integerp (len, 1) && tree_low_cst (len, 1) == 1)
    {
      tree cst_uchar_node = build_type_variant (unsigned_char_type_node, 1, 0);
      tree cst_uchar_ptr_node = build_pointer_type (cst_uchar_node);
      tree ind1 =
      fold (build1 (CONVERT_EXPR, integer_type_node,
                    build1 (INDIRECT_REF, cst_uchar_node,
                            build1 (NOP_EXPR, cst_uchar_ptr_node, arg1))));
      tree ind2 =
      fold (build1 (CONVERT_EXPR, integer_type_node,
                    build1 (INDIRECT_REF, cst_uchar_node,
                            build1 (NOP_EXPR, cst_uchar_ptr_node, arg2))));
      tree result = fold (build (MINUS_EXPR, integer_type_node, ind1, ind2));
      return expand_expr (result, target, mode, EXPAND_NORMAL);
    }

#ifdef HAVE_cmpstrsi
  {
    rtx arg1_rtx, arg2_rtx, arg3_rtx;
    rtx result;
    rtx insn;

    int arg1_align
      = get_pointer_alignment (arg1, BIGGEST_ALIGNMENT) / BITS_PER_UNIT;
    int arg2_align
      = get_pointer_alignment (arg2, BIGGEST_ALIGNMENT) / BITS_PER_UNIT;
    enum machine_mode insn_mode
      = insn_data[(int) CODE_FOR_cmpstrsi].operand[0].mode;

    /* If we don't have POINTER_TYPE, call the function.  */
    if (arg1_align == 0 || arg2_align == 0)
      return 0;

    /* Make a place to write the result of the instruction.  */
    result = target;
    if (! (result != 0
	   && GET_CODE (result) == REG && GET_MODE (result) == insn_mode
	   && REGNO (result) >= FIRST_PSEUDO_REGISTER))
      result = gen_reg_rtx (insn_mode);

    arg1_rtx = get_memory_rtx (arg1);
    arg2_rtx = get_memory_rtx (arg2);
    arg3_rtx = expand_expr (len, NULL_RTX, VOIDmode, 0);
    if (!HAVE_cmpstrsi)
      insn = NULL_RTX;
    else
      insn = gen_cmpstrsi (result, arg1_rtx, arg2_rtx, arg3_rtx,
			   GEN_INT (MIN (arg1_align, arg2_align)));

    if (insn)
      emit_insn (insn);
    else
      emit_library_call_value (memcmp_libfunc, result, LCT_PURE_MAKE_BLOCK,
			       TYPE_MODE (integer_type_node), 3,
			       XEXP (arg1_rtx, 0), Pmode,
			       XEXP (arg2_rtx, 0), Pmode,
			       convert_to_mode (TYPE_MODE (sizetype), arg3_rtx,
						TREE_UNSIGNED (sizetype)),
			       TYPE_MODE (sizetype));

    /* Return the value in the proper mode for this function.  */
    mode = TYPE_MODE (TREE_TYPE (exp));
    if (GET_MODE (result) == mode)
      return result;
    else if (target != 0)
      {
	convert_move (target, result, 0);
	return target;
      }
    else
      return convert_to_mode (mode, result, 0);
  }
#endif

  return 0;
}

/* Expand expression EXP, which is a call to the strcmp builtin.  Return 0
   if we failed the caller should emit a normal call, otherwise try to get
   the result in TARGET, if convenient.  */

static rtx
expand_builtin_strcmp (exp, target, mode)
     tree exp;
     rtx target;
     enum machine_mode mode;
{
  tree arglist = TREE_OPERAND (exp, 1);
  tree arg1, arg2, len, len2, fn;
  const char *p1, *p2;

  if (!validate_arglist (arglist, POINTER_TYPE, POINTER_TYPE, VOID_TYPE))
    return 0;

  arg1 = TREE_VALUE (arglist);
  arg2 = TREE_VALUE (TREE_CHAIN (arglist));

  p1 = c_getstr (arg1);
  p2 = c_getstr (arg2);

  if (p1 && p2)
    {
      const int i = strcmp (p1, p2);
      return (i < 0 ? constm1_rtx : (i > 0 ? const1_rtx : const0_rtx));
    }

  /* If either arg is "", return an expression corresponding to
     (*(const unsigned char*)arg1 - (const unsigned char*)arg2).  */
  if ((p1 && *p1 == '\0') || (p2 && *p2 == '\0'))
    {
      tree cst_uchar_node = build_type_variant (unsigned_char_type_node, 1, 0);
      tree cst_uchar_ptr_node = build_pointer_type (cst_uchar_node);
      tree ind1 =
	fold (build1 (CONVERT_EXPR, integer_type_node,
		      build1 (INDIRECT_REF, cst_uchar_node,
			      build1 (NOP_EXPR, cst_uchar_ptr_node, arg1))));
      tree ind2 =
	fold (build1 (CONVERT_EXPR, integer_type_node,
		      build1 (INDIRECT_REF, cst_uchar_node,
			      build1 (NOP_EXPR, cst_uchar_ptr_node, arg2))));
      tree result = fold (build (MINUS_EXPR, integer_type_node, ind1, ind2));
      return expand_expr (result, target, mode, EXPAND_NORMAL);
    }

  len = c_strlen (arg1);
  len2 = c_strlen (arg2);

  if (len)
    len = size_binop (PLUS_EXPR, ssize_int (1), len);

  if (len2)
    len2 = size_binop (PLUS_EXPR, ssize_int (1), len2);

  /* If we don't have a constant length for the first, use the length
     of the second, if we know it.  We don't require a constant for
     this case; some cost analysis could be done if both are available
     but neither is constant.  For now, assume they're equally cheap
     unless one has side effects.

     If both strings have constant lengths, use the smaller.  This
     could arise if optimization results in strcpy being called with
     two fixed strings, or if the code was machine-generated.  We should
     add some code to the `memcmp' handler below to deal with such
     situations, someday.  */

  if (!len || TREE_CODE (len) != INTEGER_CST)
    {
      if (len2 && !TREE_SIDE_EFFECTS (len2))
        len = len2;
      else if (len == 0)
        return 0;
    }
  else if (len2 && TREE_CODE (len2) == INTEGER_CST
           && tree_int_cst_lt (len2, len))
    len = len2;

  /* If both arguments have side effects, we cannot optimize.  */
  if (TREE_SIDE_EFFECTS (len))
    return 0;

  fn = built_in_decls[BUILT_IN_MEMCMP];
  if (!fn)
    return 0;

  chainon (arglist, build_tree_list (NULL_TREE, len));
  return expand_expr (build_function_call_expr (fn, arglist),
                      target, mode, EXPAND_NORMAL);
}

/* Expand expression EXP, which is a call to the strncmp builtin.  Return 0
   if we failed the caller should emit a normal call, otherwise try to get
   the result in TARGET, if convenient.  */

static rtx
expand_builtin_strncmp (exp, target, mode)
     tree exp;
     rtx target;
     enum machine_mode mode;
{
  tree arglist = TREE_OPERAND (exp, 1);
  tree fn, newarglist, len = 0;
  tree arg1, arg2, arg3;
  const char *p1, *p2;

  if (!validate_arglist (arglist,
			 POINTER_TYPE, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return 0;

  arg1 = TREE_VALUE (arglist);
  arg2 = TREE_VALUE (TREE_CHAIN (arglist));
  arg3 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));

  /* If the len parameter is zero, return zero.  */
  if (host_integerp (arg3, 1) && tree_low_cst (arg3, 1) == 0)
  {
    /* Evaluate and ignore arg1 and arg2 in case they have
       side-effects.  */
    expand_expr (arg1, const0_rtx, VOIDmode, EXPAND_NORMAL);
    expand_expr (arg2, const0_rtx, VOIDmode, EXPAND_NORMAL);
    return const0_rtx;
  }

  p1 = c_getstr (arg1);
  p2 = c_getstr (arg2);

  /* If all arguments are constant, evaluate at compile-time.  */
  if (host_integerp (arg3, 1) && p1 && p2)
  {
    const int r = strncmp (p1, p2, tree_low_cst (arg3, 1));
    return (r < 0 ? constm1_rtx : (r > 0 ? const1_rtx : const0_rtx));
  }

  /* If len == 1 or (either string parameter is "" and (len >= 1)),
      return (*(const u_char*)arg1 - *(const u_char*)arg2).  */
  if (host_integerp (arg3, 1)
      && (tree_low_cst (arg3, 1) == 1
	  || (tree_low_cst (arg3, 1) > 1
	      && ((p1 && *p1 == '\0') || (p2 && *p2 == '\0')))))
    {
      tree cst_uchar_node = build_type_variant (unsigned_char_type_node, 1, 0);
      tree cst_uchar_ptr_node = build_pointer_type (cst_uchar_node);
      tree ind1 =
	fold (build1 (CONVERT_EXPR, integer_type_node,
		      build1 (INDIRECT_REF, cst_uchar_node,
			      build1 (NOP_EXPR, cst_uchar_ptr_node, arg1))));
      tree ind2 =
	fold (build1 (CONVERT_EXPR, integer_type_node,
		      build1 (INDIRECT_REF, cst_uchar_node,
			      build1 (NOP_EXPR, cst_uchar_ptr_node, arg2))));
      tree result = fold (build (MINUS_EXPR, integer_type_node, ind1, ind2));
      return expand_expr (result, target, mode, EXPAND_NORMAL);
    }

  /* If c_strlen can determine an expression for one of the string
     lengths, and it doesn't have side effects, then call
     expand_builtin_memcmp() using length MIN(strlen(string)+1, arg3).  */

  /* Perhaps one of the strings is really constant, if so prefer
     that constant length over the other string's length.  */
  if (p1)
    len = c_strlen (arg1);
  else if (p2)
    len = c_strlen (arg2);

  /* If we still don't have a len, try either string arg as long
     as they don't have side effects.  */
  if (!len && !TREE_SIDE_EFFECTS (arg1))
    len = c_strlen (arg1);
  if (!len && !TREE_SIDE_EFFECTS (arg2))
    len = c_strlen (arg2);
  /* If we still don't have a length, punt.  */
  if (!len)
    return 0;

  fn = built_in_decls[BUILT_IN_MEMCMP];
  if (!fn)
    return 0;

  /* Add one to the string length.  */
  len = fold (size_binop (PLUS_EXPR, len, ssize_int (1)));

  /* The actual new length parameter is MIN(len,arg3).  */
  len = fold (build (MIN_EXPR, TREE_TYPE (len), len, arg3));

  newarglist = build_tree_list (NULL_TREE, len);
  newarglist = tree_cons (NULL_TREE, arg2, newarglist);
  newarglist = tree_cons (NULL_TREE, arg1, newarglist);
  return expand_expr (build_function_call_expr (fn, newarglist),
                      target, mode, EXPAND_NORMAL);
}

/* Expand expression EXP, which is a call to the strcat builtin.
   Return 0 if we failed the caller should emit a normal call,
   otherwise try to get the result in TARGET, if convenient.  */

static rtx
expand_builtin_strcat (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist, POINTER_TYPE, POINTER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree dst = TREE_VALUE (arglist),
	src = TREE_VALUE (TREE_CHAIN (arglist));
      const char *p = c_getstr (src);

      /* If the string length is zero, return the dst parameter.  */
      if (p && *p == '\0')
	return expand_expr (dst, target, mode, EXPAND_NORMAL);

      return 0;
    }
}

/* Expand expression EXP, which is a call to the strncat builtin.
   Return 0 if we failed the caller should emit a normal call,
   otherwise try to get the result in TARGET, if convenient.  */

static rtx
expand_builtin_strncat (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist,
			 POINTER_TYPE, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree dst = TREE_VALUE (arglist),
	src = TREE_VALUE (TREE_CHAIN (arglist)),
	len = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      const char *p = c_getstr (src);

      /* If the requested length is zero, or the src parameter string
          length is zero, return the dst parameter.  */
      if (integer_zerop (len) || (p && *p == '\0'))
        {
	  /* Evaluate and ignore the src and len parameters in case
	     they have side-effects.  */
	  expand_expr (src, const0_rtx, VOIDmode, EXPAND_NORMAL);
	  expand_expr (len, const0_rtx, VOIDmode, EXPAND_NORMAL);
	  return expand_expr (dst, target, mode, EXPAND_NORMAL);
	}

      /* If the requested len is greater than or equal to the string
         length, call strcat.  */
      if (TREE_CODE (len) == INTEGER_CST && p
	  && compare_tree_int (len, strlen (p)) >= 0)
        {
	  tree newarglist
	    = tree_cons (NULL_TREE, dst, build_tree_list (NULL_TREE, src));
	  tree fn = built_in_decls[BUILT_IN_STRCAT];

	  /* If the replacement _DECL isn't initialized, don't do the
	     transformation.  */
	  if (!fn)
	    return 0;

	  return expand_expr (build_function_call_expr (fn, newarglist),
			      target, mode, EXPAND_NORMAL);
	}
      return 0;
    }
}

/* Expand expression EXP, which is a call to the strspn builtin.
   Return 0 if we failed the caller should emit a normal call,
   otherwise try to get the result in TARGET, if convenient.  */

static rtx
expand_builtin_strspn (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist, POINTER_TYPE, POINTER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree s1 = TREE_VALUE (arglist), s2 = TREE_VALUE (TREE_CHAIN (arglist));
      const char *p1 = c_getstr (s1), *p2 = c_getstr (s2);

      /* If both arguments are constants, evaluate at compile-time.  */
      if (p1 && p2)
        {
	  const size_t r = strspn (p1, p2);
	  return expand_expr (size_int (r), target, mode, EXPAND_NORMAL);
	}

      /* If either argument is "", return 0.  */
      if ((p1 && *p1 == '\0') || (p2 && *p2 == '\0'))
        {
	  /* Evaluate and ignore both arguments in case either one has
	     side-effects.  */
	  expand_expr (s1, const0_rtx, VOIDmode, EXPAND_NORMAL);
	  expand_expr (s2, const0_rtx, VOIDmode, EXPAND_NORMAL);
	  return const0_rtx;
	}
      return 0;
    }
}

/* Expand expression EXP, which is a call to the strcspn builtin.
   Return 0 if we failed the caller should emit a normal call,
   otherwise try to get the result in TARGET, if convenient.  */

static rtx
expand_builtin_strcspn (arglist, target, mode)
     tree arglist;
     rtx target;
     enum machine_mode mode;
{
  if (!validate_arglist (arglist, POINTER_TYPE, POINTER_TYPE, VOID_TYPE))
    return 0;
  else
    {
      tree s1 = TREE_VALUE (arglist), s2 = TREE_VALUE (TREE_CHAIN (arglist));
      const char *p1 = c_getstr (s1), *p2 = c_getstr (s2);

      /* If both arguments are constants, evaluate at compile-time.  */
      if (p1 && p2)
        {
	  const size_t r = strcspn (p1, p2);
	  return expand_expr (size_int (r), target, mode, EXPAND_NORMAL);
	}

      /* If the first argument is "", return 0.  */
      if (p1 && *p1 == '\0')
        {
	  /* Evaluate and ignore argument s2 in case it has
	     side-effects.  */
	  expand_expr (s2, const0_rtx, VOIDmode, EXPAND_NORMAL);
	  return const0_rtx;
	}

      /* If the second argument is "", return __builtin_strlen(s1).  */
      if (p2 && *p2 == '\0')
        {
	  tree newarglist = build_tree_list (NULL_TREE, s1),
	    fn = built_in_decls[BUILT_IN_STRLEN];

	  /* If the replacement _DECL isn't initialized, don't do the
	     transformation.  */
	  if (!fn)
	    return 0;

	  return expand_expr (build_function_call_expr (fn, newarglist),
			      target, mode, EXPAND_NORMAL);
	}
      return 0;
    }
}

/* Expand a call to __builtin_saveregs, generating the result in TARGET,
   if that's convenient.  */

rtx
expand_builtin_saveregs ()
{
  rtx val, seq;

  /* Don't do __builtin_saveregs more than once in a function.
     Save the result of the first call and reuse it.  */
  if (saveregs_value != 0)
    return saveregs_value;

  /* When this function is called, it means that registers must be
     saved on entry to this function.  So we migrate the call to the
     first insn of this function.  */

  start_sequence ();

#ifdef EXPAND_BUILTIN_SAVEREGS
  /* Do whatever the machine needs done in this case.  */
  val = EXPAND_BUILTIN_SAVEREGS ();
#else
  /* ??? We used to try and build up a call to the out of line function,
     guessing about what registers needed saving etc.  This became much
     harder with __builtin_va_start, since we don't have a tree for a
     call to __builtin_saveregs to fall back on.  There was exactly one
     port (i860) that used this code, and I'm unconvinced it could actually
     handle the general case.  So we no longer try to handle anything
     weird and make the backend absorb the evil.  */

  error ("__builtin_saveregs not supported by this target");
  val = const0_rtx;
#endif

  seq = get_insns ();
  end_sequence ();

  saveregs_value = val;

  /* Put the sequence after the NOTE that starts the function.  If this
     is inside a SEQUENCE, make the outer-level insn chain current, so
     the code is placed at the start of the function.  */
  push_topmost_sequence ();
  emit_insns_after (seq, get_insns ());
  pop_topmost_sequence ();

  return val;
}

/* __builtin_args_info (N) returns word N of the arg space info
   for the current function.  The number and meanings of words
   is controlled by the definition of CUMULATIVE_ARGS.  */

static rtx
expand_builtin_args_info (exp)
     tree exp;
{
  tree arglist = TREE_OPERAND (exp, 1);
  int nwords = sizeof (CUMULATIVE_ARGS) / sizeof (int);
  int *word_ptr = (int *) &current_function_args_info;
#if 0
  /* These are used by the code below that is if 0'ed away */
  int i;
  tree type, elts, result;
#endif

  if (sizeof (CUMULATIVE_ARGS) % sizeof (int) != 0)
    abort ();

  if (arglist != 0)
    {
      if (!host_integerp (TREE_VALUE (arglist), 0))
	error ("argument of `__builtin_args_info' must be constant");
      else
	{
	  HOST_WIDE_INT wordnum = tree_low_cst (TREE_VALUE (arglist), 0);

	  if (wordnum < 0 || wordnum >= nwords)
	    error ("argument of `__builtin_args_info' out of range");
	  else
	    return GEN_INT (word_ptr[wordnum]);
	}
    }
  else
    error ("missing argument in `__builtin_args_info'");

  return const0_rtx;

#if 0
  for (i = 0; i < nwords; i++)
    elts = tree_cons (NULL_TREE, build_int_2 (word_ptr[i], 0));

  type = build_array_type (integer_type_node,
			   build_index_type (build_int_2 (nwords, 0)));
  result = build (CONSTRUCTOR, type, NULL_TREE, nreverse (elts));
  TREE_CONSTANT (result) = 1;
  TREE_STATIC (result) = 1;
  result = build1 (INDIRECT_REF, build_pointer_type (type), result);
  TREE_CONSTANT (result) = 1;
  return expand_expr (result, NULL_RTX, VOIDmode, 0);
#endif
}

/* Expand ARGLIST, from a call to __builtin_next_arg.  */

static rtx
expand_builtin_next_arg (arglist)
     tree arglist;
{
  tree fntype = TREE_TYPE (current_function_decl);

  if ((TYPE_ARG_TYPES (fntype) == 0
       || (TREE_VALUE (tree_last (TYPE_ARG_TYPES (fntype)))
	   == void_type_node))
      && ! current_function_varargs)
    {
      error ("`va_start' used in function with fixed args");
      return const0_rtx;
    }

  if (arglist)
    {
      tree last_parm = tree_last (DECL_ARGUMENTS (current_function_decl));
      tree arg = TREE_VALUE (arglist);

      /* Strip off all nops for the sake of the comparison.  This
	 is not quite the same as STRIP_NOPS.  It does more.
	 We must also strip off INDIRECT_EXPR for C++ reference
	 parameters.  */
      while (TREE_CODE (arg) == NOP_EXPR
	     || TREE_CODE (arg) == CONVERT_EXPR
	     || TREE_CODE (arg) == NON_LVALUE_EXPR
	     || TREE_CODE (arg) == INDIRECT_REF)
	arg = TREE_OPERAND (arg, 0);
      if (arg != last_parm)
	warning ("second parameter of `va_start' not last named argument");
    }
  else if (! current_function_varargs)
    /* Evidently an out of date version of <stdarg.h>; can't validate
       va_start's second argument, but can still work as intended.  */
    warning ("`__builtin_next_arg' called without an argument");

  return expand_binop (Pmode, add_optab,
		       current_function_internal_arg_pointer,
		       current_function_arg_offset_rtx,
		       NULL_RTX, 0, OPTAB_LIB_WIDEN);
}

/* Make it easier for the backends by protecting the valist argument
   from multiple evaluations.  */

static tree
stabilize_va_list (valist, needs_lvalue)
     tree valist;
     int needs_lvalue;
{
  if (TREE_CODE (va_list_type_node) == ARRAY_TYPE)
    {
      if (TREE_SIDE_EFFECTS (valist))
	valist = save_expr (valist);

      /* For this case, the backends will be expecting a pointer to
	 TREE_TYPE (va_list_type_node), but it's possible we've
	 actually been given an array (an actual va_list_type_node).
	 So fix it.  */
      if (TREE_CODE (TREE_TYPE (valist)) == ARRAY_TYPE)
	{
	  tree p1 = build_pointer_type (TREE_TYPE (va_list_type_node));
	  tree p2 = build_pointer_type (va_list_type_node);

	  valist = build1 (ADDR_EXPR, p2, valist);
	  valist = fold (build1 (NOP_EXPR, p1, valist));
	}
    }
  else
    {
      tree pt;

      if (! needs_lvalue)
	{
	  if (! TREE_SIDE_EFFECTS (valist))
	    return valist;

	  pt = build_pointer_type (va_list_type_node);
	  valist = fold (build1 (ADDR_EXPR, pt, valist));
	  TREE_SIDE_EFFECTS (valist) = 1;
	}

      if (TREE_SIDE_EFFECTS (valist))
	valist = save_expr (valist);
      valist = fold (build1 (INDIRECT_REF, TREE_TYPE (TREE_TYPE (valist)),
			     valist));
    }

  return valist;
}

/* The "standard" implementation of va_start: just assign `nextarg' to
   the variable.  */

void
std_expand_builtin_va_start (stdarg_p, valist, nextarg)
     int stdarg_p;
     tree valist;
     rtx nextarg;
{
  tree t;

  if (! stdarg_p)
    {
      /* The dummy named parameter is declared as a 'word' sized
	 object, but if a 'word' is smaller than an 'int', it would
	 have been promoted to int when it was added to the arglist.  */
      int align = PARM_BOUNDARY / BITS_PER_UNIT;
      int size = MAX (UNITS_PER_WORD,
		      GET_MODE_SIZE (TYPE_MODE (integer_type_node)));
      int offset = ((size + align - 1) / align) * align;
      nextarg = plus_constant (nextarg, -offset);
    }

  t = build (MODIFY_EXPR, TREE_TYPE (valist), valist,
	     make_tree (ptr_type_node, nextarg));
  TREE_SIDE_EFFECTS (t) = 1;

  expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);
}

/* Expand ARGLIST, which from a call to __builtin_stdarg_va_start or
   __builtin_varargs_va_start, depending on STDARG_P.  */

static rtx
expand_builtin_va_start (stdarg_p, arglist)
     int stdarg_p;
     tree arglist;
{
  rtx nextarg;
  tree chain = arglist, valist;

  if (stdarg_p)
    nextarg = expand_builtin_next_arg (chain = TREE_CHAIN (arglist));
  else
    nextarg = expand_builtin_next_arg (NULL_TREE);

  if (TREE_CHAIN (chain))
    error ("too many arguments to function `va_start'");

  valist = stabilize_va_list (TREE_VALUE (arglist), 1);

#ifdef EXPAND_BUILTIN_VA_START
  EXPAND_BUILTIN_VA_START (stdarg_p, valist, nextarg);
#else
  std_expand_builtin_va_start (stdarg_p, valist, nextarg);
#endif

  return const0_rtx;
}

/* The "standard" implementation of va_arg: read the value from the
   current (padded) address and increment by the (padded) size.  */

rtx
std_expand_builtin_va_arg (valist, type)
     tree valist, type;
{
  tree addr_tree, t, type_size = NULL;
  tree align, alignm1;
  tree rounded_size;
  rtx addr;

  /* Compute the rounded size of the type.  */
  align = size_int (PARM_BOUNDARY / BITS_PER_UNIT);
  alignm1 = size_int (PARM_BOUNDARY / BITS_PER_UNIT - 1);
  if (type == error_mark_node
      || (type_size = TYPE_SIZE_UNIT (TYPE_MAIN_VARIANT (type))) == NULL
      || TREE_OVERFLOW (type_size))
    rounded_size = size_zero_node;
  else
    rounded_size = fold (build (MULT_EXPR, sizetype,
				fold (build (TRUNC_DIV_EXPR, sizetype,
					     fold (build (PLUS_EXPR, sizetype,
							  type_size, alignm1)),
					     align)),
				align));

  /* Get AP.  */
  addr_tree = valist;
  if (PAD_VARARGS_DOWN && ! integer_zerop (rounded_size))
    {
      /* Small args are padded downward.  */
      addr_tree = fold (build (PLUS_EXPR, TREE_TYPE (addr_tree), addr_tree,
			       fold (build (COND_EXPR, sizetype,
					    fold (build (GT_EXPR, sizetype,
							 rounded_size,
							 align)),
					    size_zero_node,
					    fold (build (MINUS_EXPR, sizetype,
							 rounded_size,
							 type_size))))));
    }

  addr = expand_expr (addr_tree, NULL_RTX, Pmode, EXPAND_NORMAL);
  addr = copy_to_reg (addr);

  /* Compute new value for AP.  */
  if (! integer_zerop (rounded_size))
    {
      t = build (MODIFY_EXPR, TREE_TYPE (valist), valist,
		 build (PLUS_EXPR, TREE_TYPE (valist), valist,
			rounded_size));
      TREE_SIDE_EFFECTS (t) = 1;
      expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);
    }

  return addr;
}

/* Expand __builtin_va_arg, which is not really a builtin function, but
   a very special sort of operator.  */

rtx
expand_builtin_va_arg (valist, type)
     tree valist, type;
{
  rtx addr, result;
  tree promoted_type, want_va_type, have_va_type;

  /* Verify that valist is of the proper type.  */

  want_va_type = va_list_type_node;
  have_va_type = TREE_TYPE (valist);
  if (TREE_CODE (want_va_type) == ARRAY_TYPE)
    {
      /* If va_list is an array type, the argument may have decayed
	 to a pointer type, e.g. by being passed to another function.
         In that case, unwrap both types so that we can compare the
	 underlying records.  */
      if (TREE_CODE (have_va_type) == ARRAY_TYPE
	  || TREE_CODE (have_va_type) == POINTER_TYPE)
	{
	  want_va_type = TREE_TYPE (want_va_type);
	  have_va_type = TREE_TYPE (have_va_type);
	}
    }
  if (TYPE_MAIN_VARIANT (want_va_type) != TYPE_MAIN_VARIANT (have_va_type))
    {
      error ("first argument to `va_arg' not of type `va_list'");
      addr = const0_rtx;
    }

  /* Generate a diagnostic for requesting data of a type that cannot
     be passed through `...' due to type promotion at the call site.  */
  else if ((promoted_type = (*lang_type_promotes_to) (type)) != NULL_TREE)
    {
      const char *name = "<anonymous type>", *pname = 0;
      static bool gave_help;

      if (TYPE_NAME (type))
	{
	  if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
	    name = IDENTIFIER_POINTER (TYPE_NAME (type));
	  else if (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
		   && DECL_NAME (TYPE_NAME (type)))
	    name = IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (type)));
	}
      if (TYPE_NAME (promoted_type))
	{
	  if (TREE_CODE (TYPE_NAME (promoted_type)) == IDENTIFIER_NODE)
	    pname = IDENTIFIER_POINTER (TYPE_NAME (promoted_type));
	  else if (TREE_CODE (TYPE_NAME (promoted_type)) == TYPE_DECL
		   && DECL_NAME (TYPE_NAME (promoted_type)))
	    pname = IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (promoted_type)));
	}

      /* Unfortunately, this is merely undefined, rather than a constraint
	 violation, so we cannot make this an error.  If this call is never
	 executed, the program is still strictly conforming.  */
      warning ("`%s' is promoted to `%s' when passed through `...'",
	       name, pname);
      if (! gave_help)
	{
	  gave_help = true;
	  warning ("(so you should pass `%s' not `%s' to `va_arg')",
		   pname, name);
	}

      /* We can, however, treat "undefined" any way we please.
	 Call abort to encourage the user to fix the program.  */
      expand_builtin_trap ();

      /* This is dead code, but go ahead and finish so that the
	 mode of the result comes out right.  */
      addr = const0_rtx;
    }
  else
    {
      /* Make it easier for the backends by protecting the valist argument
         from multiple evaluations.  */
      valist = stabilize_va_list (valist, 0);

#ifdef EXPAND_BUILTIN_VA_ARG
      addr = EXPAND_BUILTIN_VA_ARG (valist, type);
#else
      addr = std_expand_builtin_va_arg (valist, type);
#endif
    }

#ifdef POINTERS_EXTEND_UNSIGNED
  if (GET_MODE (addr) != Pmode)
    addr = convert_memory_address (Pmode, addr);
#endif

  result = gen_rtx_MEM (TYPE_MODE (type), addr);
  set_mem_alias_set (result, get_varargs_alias_set ());

  return result;
}

/* Expand ARGLIST, from a call to __builtin_va_end.  */

static rtx
expand_builtin_va_end (arglist)
     tree arglist;
{
  tree valist = TREE_VALUE (arglist);

#ifdef EXPAND_BUILTIN_VA_END
  valist = stabilize_va_list (valist, 0);
  EXPAND_BUILTIN_VA_END(arglist);
#else
  /* Evaluate for side effects, if needed.  I hate macros that don't
     do that.  */
  if (TREE_SIDE_EFFECTS (valist))
    expand_expr (valist, const0_rtx, VOIDmode, EXPAND_NORMAL);
#endif

  return const0_rtx;
}

/* Expand ARGLIST, from a call to __builtin_va_copy.  We do this as a
   builtin rather than just as an assignment in stdarg.h because of the
   nastiness of array-type va_list types.  */

static rtx
expand_builtin_va_copy (arglist)
     tree arglist;
{
  tree dst, src, t;

  dst = TREE_VALUE (arglist);
  src = TREE_VALUE (TREE_CHAIN (arglist));

  dst = stabilize_va_list (dst, 1);
  src = stabilize_va_list (src, 0);

  if (TREE_CODE (va_list_type_node) != ARRAY_TYPE)
    {
      t = build (MODIFY_EXPR, va_list_type_node, dst, src);
      TREE_SIDE_EFFECTS (t) = 1;
      expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);
    }
  else
    {
      rtx dstb, srcb, size;

      /* Evaluate to pointers.  */
      dstb = expand_expr (dst, NULL_RTX, Pmode, EXPAND_NORMAL);
      srcb = expand_expr (src, NULL_RTX, Pmode, EXPAND_NORMAL);
      size = expand_expr (TYPE_SIZE_UNIT (va_list_type_node), NULL_RTX,
			  VOIDmode, EXPAND_NORMAL);

#ifdef POINTERS_EXTEND_UNSIGNED
      if (GET_MODE (dstb) != Pmode)
	dstb = convert_memory_address (Pmode, dstb);

      if (GET_MODE (srcb) != Pmode)
	srcb = convert_memory_address (Pmode, srcb);
#endif

      /* "Dereference" to BLKmode memories.  */
      dstb = gen_rtx_MEM (BLKmode, dstb);
      set_mem_alias_set (dstb, get_alias_set (TREE_TYPE (TREE_TYPE (dst))));
      set_mem_align (dstb, TYPE_ALIGN (va_list_type_node));
      srcb = gen_rtx_MEM (BLKmode, srcb);
      set_mem_alias_set (srcb, get_alias_set (TREE_TYPE (TREE_TYPE (src))));
      set_mem_align (srcb, TYPE_ALIGN (va_list_type_node));

      /* Copy.  */
      emit_block_move (dstb, srcb, size);
    }

  return const0_rtx;
}

/* Expand a call to one of the builtin functions __builtin_frame_address or
   __builtin_return_address.  */

static rtx
expand_builtin_frame_address (exp)
     tree exp;
{
  tree fndecl = TREE_OPERAND (TREE_OPERAND (exp, 0), 0);
  tree arglist = TREE_OPERAND (exp, 1);

  /* The argument must be a nonnegative integer constant.
     It counts the number of frames to scan up the stack.
     The value is the return address saved in that frame.  */
  if (arglist == 0)
    /* Warning about missing arg was already issued.  */
    return const0_rtx;
  else if (! host_integerp (TREE_VALUE (arglist), 1))
    {
      if (DECL_FUNCTION_CODE (fndecl) == BUILT_IN_FRAME_ADDRESS)
	error ("invalid arg to `__builtin_frame_address'");
      else
	error ("invalid arg to `__builtin_return_address'");
      return const0_rtx;
    }
  else
    {
      rtx tem
	= expand_builtin_return_addr (DECL_FUNCTION_CODE (fndecl),
				      tree_low_cst (TREE_VALUE (arglist), 1),
				      hard_frame_pointer_rtx);

      /* Some ports cannot access arbitrary stack frames.  */
      if (tem == NULL)
	{
	  if (DECL_FUNCTION_CODE (fndecl) == BUILT_IN_FRAME_ADDRESS)
	    warning ("unsupported arg to `__builtin_frame_address'");
	  else
	    warning ("unsupported arg to `__builtin_return_address'");
	  return const0_rtx;
	}

      /* For __builtin_frame_address, return what we've got.  */
      if (DECL_FUNCTION_CODE (fndecl) == BUILT_IN_FRAME_ADDRESS)
	return tem;

      if (GET_CODE (tem) != REG
	  && ! CONSTANT_P (tem))
	tem = copy_to_mode_reg (Pmode, tem);
      return tem;
    }
}

/* Expand a call to the alloca builtin, with arguments ARGLIST.  Return 0 if
   we failed and the caller should emit a normal call, otherwise try to get
   the result in TARGET, if convenient.  */

static rtx
expand_builtin_alloca (arglist, target)
     tree arglist;
     rtx target;
{
  rtx op0;
  rtx result;

  if (!validate_arglist (arglist, INTEGER_TYPE, VOID_TYPE))
    return 0;

  /* Compute the argument.  */
  op0 = expand_expr (TREE_VALUE (arglist), NULL_RTX, VOIDmode, 0);

  /* Allocate the desired space.  */
  result = allocate_dynamic_stack_space (op0, target, BITS_PER_UNIT);

#ifdef POINTERS_EXTEND_UNSIGNED
  if (GET_MODE (result) != ptr_mode)
    result = convert_memory_address (ptr_mode, result);
#endif

  return result;
}

/* Expand a call to the ffs builtin.  The arguments are in ARGLIST.
   Return 0 if a normal call should be emitted rather than expanding the
   function in-line.  If convenient, the result should be placed in TARGET.
   SUBTARGET may be used as the target for computing one of EXP's operands.  */

static rtx
expand_builtin_ffs (arglist, target, subtarget)
     tree arglist;
     rtx target, subtarget;
{
  rtx op0;
  if (!validate_arglist (arglist, INTEGER_TYPE, VOID_TYPE))
    return 0;

  /* Compute the argument.  */
  op0 = expand_expr (TREE_VALUE (arglist), subtarget, VOIDmode, 0);
  /* Compute ffs, into TARGET if possible.
     Set TARGET to wherever the result comes back.  */
  target = expand_unop (TYPE_MODE (TREE_TYPE (TREE_VALUE (arglist))),
			ffs_optab, op0, target, 1);
  if (target == 0)
    abort ();
  return target;
}

/* If the string passed to fputs is a constant and is one character
   long, we attempt to transform this call into __builtin_fputc().  */

static rtx
expand_builtin_fputs (arglist, ignore, unlocked)
     tree arglist;
     int ignore;
     int unlocked;
{
  tree len, fn;
  tree fn_fputc = unlocked ? built_in_decls[BUILT_IN_FPUTC_UNLOCKED]
    : built_in_decls[BUILT_IN_FPUTC];
  tree fn_fwrite = unlocked ? built_in_decls[BUILT_IN_FWRITE_UNLOCKED]
    : built_in_decls[BUILT_IN_FWRITE];

  /* If the return value is used, or the replacement _DECL isn't
     initialized, don't do the transformation.  */
  if (!ignore || !fn_fputc || !fn_fwrite)
    return 0;

  /* Verify the arguments in the original call.  */
  if (!validate_arglist (arglist, POINTER_TYPE, POINTER_TYPE, VOID_TYPE))
    return 0;

  /* Get the length of the string passed to fputs.  If the length
     can't be determined, punt.  */
  if (!(len = c_strlen (TREE_VALUE (arglist)))
      || TREE_CODE (len) != INTEGER_CST)
    return 0;

  switch (compare_tree_int (len, 1))
    {
    case -1: /* length is 0, delete the call entirely .  */
      {
	/* Evaluate and ignore the argument in case it has
           side-effects.  */
	expand_expr (TREE_VALUE (TREE_CHAIN (arglist)), const0_rtx,
		     VOIDmode, EXPAND_NORMAL);
	return const0_rtx;
      }
    case 0: /* length is 1, call fputc.  */
      {
	const char *p = c_getstr (TREE_VALUE (arglist));

	if (p != NULL)
	  {
	    /* New argument list transforming fputs(string, stream) to
	       fputc(string[0], stream).  */
	    arglist =
	      build_tree_list (NULL_TREE, TREE_VALUE (TREE_CHAIN (arglist)));
	    arglist =
	      tree_cons (NULL_TREE, build_int_2 (p[0], 0), arglist);
	    fn = fn_fputc;
	    break;
	  }
      }
      /* FALLTHROUGH */
    case 1: /* length is greater than 1, call fwrite.  */
      {
	tree string_arg = TREE_VALUE (arglist);

	/* New argument list transforming fputs(string, stream) to
	   fwrite(string, 1, len, stream).  */
	arglist = build_tree_list (NULL_TREE, TREE_VALUE (TREE_CHAIN (arglist)));
	arglist = tree_cons (NULL_TREE, len, arglist);
	arglist = tree_cons (NULL_TREE, size_one_node, arglist);
	arglist = tree_cons (NULL_TREE, string_arg, arglist);
	fn = fn_fwrite;
	break;
      }
    default:
      abort ();
    }

  return expand_expr (build_function_call_expr (fn, arglist),
		      (ignore ? const0_rtx : NULL_RTX),
		      VOIDmode, EXPAND_NORMAL);
}

/* Expand a call to __builtin_expect.  We return our argument and emit a
   NOTE_INSN_EXPECTED_VALUE note.  This is the expansion of __builtin_expect in
   a non-jump context.  */

static rtx
expand_builtin_expect (arglist, target)
     tree arglist;
     rtx target;
{
  tree exp, c;
  rtx note, rtx_c;

  if (arglist == NULL_TREE
      || TREE_CHAIN (arglist) == NULL_TREE)
    return const0_rtx;
  exp = TREE_VALUE (arglist);
  c = TREE_VALUE (TREE_CHAIN (arglist));

  if (TREE_CODE (c) != INTEGER_CST)
    {
      error ("second arg to `__builtin_expect' must be a constant");
      c = integer_zero_node;
    }

  target = expand_expr (exp, target, VOIDmode, EXPAND_NORMAL);

  /* Don't bother with expected value notes for integral constants.  */
  if (GET_CODE (target) != CONST_INT)
    {
      /* We do need to force this into a register so that we can be
	 moderately sure to be able to correctly interpret the branch
	 condition later.  */
      target = force_reg (GET_MODE (target), target);

      rtx_c = expand_expr (c, NULL_RTX, GET_MODE (target), EXPAND_NORMAL);

      note = emit_note (NULL, NOTE_INSN_EXPECTED_VALUE);
      NOTE_EXPECTED_VALUE (note) = gen_rtx_EQ (VOIDmode, target, rtx_c);
    }

  return target;
}

/* Like expand_builtin_expect, except do this in a jump context.  This is
   called from do_jump if the conditional is a __builtin_expect.  Return either
   a SEQUENCE of insns to emit the jump or NULL if we cannot optimize
   __builtin_expect.  We need to optimize this at jump time so that machines
   like the PowerPC don't turn the test into a SCC operation, and then jump
   based on the test being 0/1.  */

rtx
expand_builtin_expect_jump (exp, if_false_label, if_true_label)
     tree exp;
     rtx if_false_label;
     rtx if_true_label;
{
  tree arglist = TREE_OPERAND (exp, 1);
  tree arg0 = TREE_VALUE (arglist);
  tree arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  rtx ret = NULL_RTX;

  /* Only handle __builtin_expect (test, 0) and
     __builtin_expect (test, 1).  */
  if (TREE_CODE (TREE_TYPE (arg1)) == INTEGER_TYPE
      && (integer_zerop (arg1) || integer_onep (arg1)))
    {
      int j;
      int num_jumps = 0;

      /* If we fail to locate an appropriate conditional jump, we'll
	 fall back to normal evaluation.  Ensure that the expression
	 can be re-evaluated.  */
      switch (unsafe_for_reeval (arg0))
	{
	case 0: /* Safe.  */
	  break;

	case 1: /* Mildly unsafe.  */
	  arg0 = unsave_expr (arg0);
	  break;

	case 2: /* Wildly unsafe.  */
	  return NULL_RTX;
	}

      /* Expand the jump insns.  */
      start_sequence ();
      do_jump (arg0, if_false_label, if_true_label);
      ret = gen_sequence ();
      end_sequence ();

      /* Now that the __builtin_expect has been validated, go through and add
	 the expect's to each of the conditional jumps.  If we run into an
	 error, just give up and generate the 'safe' code of doing a SCC
	 operation and then doing a branch on that.  */
      for (j = 0; j < XVECLEN (ret, 0); j++)
	{
	  rtx insn = XVECEXP (ret, 0, j);
	  rtx pattern;

	  if (GET_CODE (insn) == JUMP_INSN && any_condjump_p (insn)
	      && (pattern = pc_set (insn)) != NULL_RTX)
	    {
	      rtx ifelse = SET_SRC (pattern);
	      rtx label;
	      int taken;

	      if (GET_CODE (ifelse) != IF_THEN_ELSE)
		continue;

	      if (GET_CODE (XEXP (ifelse, 1)) == LABEL_REF)
		{
		  taken = 1;
		  label = XEXP (XEXP (ifelse, 1), 0);
		}
	      /* An inverted jump reverses the probabilities.  */
	      else if (GET_CODE (XEXP (ifelse, 2)) == LABEL_REF)
		{
		  taken = 0;
		  label = XEXP (XEXP (ifelse, 2), 0);
		}
	      /* We shouldn't have to worry about conditional returns during
		 the expansion stage, but handle it gracefully anyway.  */
	      else if (GET_CODE (XEXP (ifelse, 1)) == RETURN)
		{
		  taken = 1;
		  label = NULL_RTX;
		}
	      /* An inverted return reverses the probabilities.  */
	      else if (GET_CODE (XEXP (ifelse, 2)) == RETURN)
		{
		  taken = 0;
		  label = NULL_RTX;
		}
	      else
		continue;

	      /* If the test is expected to fail, reverse the
		 probabilities.  */
	      if (integer_zerop (arg1))
		taken = 1 - taken;

	      /* If we are jumping to the false label, reverse the
		 probabilities.  */
	      if (label == NULL_RTX)
		;		/* conditional return */
	      else if (label == if_false_label)
		taken = 1 - taken;
	      else if (label != if_true_label)
		continue;

	      num_jumps++;
	      predict_insn_def (insn, PRED_BUILTIN_EXPECT, taken);
	    }
	}

      /* If no jumps were modified, fail and do __builtin_expect the normal
	 way.  */
      if (num_jumps == 0)
	ret = NULL_RTX;
    }

  return ret;
}

void
expand_builtin_trap ()
{
#ifdef HAVE_trap
  if (HAVE_trap)
    emit_insn (gen_trap ());
  else
#endif
    emit_library_call (abort_libfunc, LCT_NORETURN, VOIDmode, 0);
  emit_barrier ();
}

/* Expand an expression EXP that calls a built-in function,
   with result going to TARGET if that's convenient
   (and in mode MODE if that's convenient).
   SUBTARGET may be used as the target for computing one of EXP's operands.
   IGNORE is nonzero if the value is to be ignored.  */

rtx
expand_builtin (exp, target, subtarget, mode, ignore)
     tree exp;
     rtx target;
     rtx subtarget;
     enum machine_mode mode;
     int ignore;
{
  tree fndecl = TREE_OPERAND (TREE_OPERAND (exp, 0), 0);
  tree arglist = TREE_OPERAND (exp, 1);
  enum built_in_function fcode = DECL_FUNCTION_CODE (fndecl);

  if (DECL_BUILT_IN_CLASS (fndecl) == BUILT_IN_MD)
    return (*targetm.expand_builtin) (exp, target, subtarget, mode, ignore);

  /* When not optimizing, generate calls to library functions for a certain
     set of builtins.  */
  if (!optimize && !CALLED_AS_BUILT_IN (fndecl))
    switch (fcode)
      {
      case BUILT_IN_SIN:
      case BUILT_IN_COS:
      case BUILT_IN_SQRT:
      case BUILT_IN_SQRTF:
      case BUILT_IN_SQRTL:
      case BUILT_IN_MEMSET:
      case BUILT_IN_MEMCPY:
      case BUILT_IN_MEMCMP:
      case BUILT_IN_BCMP:
      case BUILT_IN_BZERO:
      case BUILT_IN_INDEX:
      case BUILT_IN_RINDEX:
      case BUILT_IN_STRCHR:
      case BUILT_IN_STRRCHR:
      case BUILT_IN_STRLEN:
      case BUILT_IN_STRCPY:
      case BUILT_IN_STRNCPY:
      case BUILT_IN_STRNCMP:
      case BUILT_IN_STRSTR:
      case BUILT_IN_STRPBRK:
      case BUILT_IN_STRCAT:
      case BUILT_IN_STRNCAT:
      case BUILT_IN_STRSPN:
      case BUILT_IN_STRCSPN:
      case BUILT_IN_STRCMP:
      case BUILT_IN_FFS:
      case BUILT_IN_PUTCHAR:
      case BUILT_IN_PUTS:
      case BUILT_IN_PRINTF:
      case BUILT_IN_FPUTC:
      case BUILT_IN_FPUTS:
      case BUILT_IN_FWRITE:
      case BUILT_IN_PUTCHAR_UNLOCKED:
      case BUILT_IN_PUTS_UNLOCKED:
      case BUILT_IN_PRINTF_UNLOCKED:
      case BUILT_IN_FPUTC_UNLOCKED:
      case BUILT_IN_FPUTS_UNLOCKED:
      case BUILT_IN_FWRITE_UNLOCKED:
        return expand_call (exp, target, ignore);

      default:
        break;
    }

  switch (fcode)
    {
    case BUILT_IN_ABS:
    case BUILT_IN_LABS:
    case BUILT_IN_LLABS:
    case BUILT_IN_IMAXABS:
    case BUILT_IN_FABS:
    case BUILT_IN_FABSF:
    case BUILT_IN_FABSL:
      /* build_function_call changes these into ABS_EXPR.  */
      abort ();

    case BUILT_IN_CONJ:
    case BUILT_IN_CONJF:
    case BUILT_IN_CONJL:
    case BUILT_IN_CREAL:
    case BUILT_IN_CREALF:
    case BUILT_IN_CREALL:
    case BUILT_IN_CIMAG:
    case BUILT_IN_CIMAGF:
    case BUILT_IN_CIMAGL:
      /* expand_tree_builtin changes these into CONJ_EXPR, REALPART_EXPR
	 and IMAGPART_EXPR.  */
      abort ();

    case BUILT_IN_SIN:
    case BUILT_IN_SINF:
    case BUILT_IN_SINL:
    case BUILT_IN_COS:
    case BUILT_IN_COSF:
    case BUILT_IN_COSL:
      /* Treat these like sqrt only if unsafe math optimizations are allowed,
	 because of possible accuracy problems.  */
      if (! flag_unsafe_math_optimizations)
	break;
    case BUILT_IN_SQRT:
    case BUILT_IN_SQRTF:
    case BUILT_IN_SQRTL:
      target = expand_builtin_mathfn (exp, target, subtarget);
      if (target)
	return target;
      break;

    case BUILT_IN_FMOD:
      break;

    case BUILT_IN_APPLY_ARGS:
      return expand_builtin_apply_args ();

      /* __builtin_apply (FUNCTION, ARGUMENTS, ARGSIZE) invokes
	 FUNCTION with a copy of the parameters described by
	 ARGUMENTS, and ARGSIZE.  It returns a block of memory
	 allocated on the stack into which is stored all the registers
	 that might possibly be used for returning the result of a
	 function.  ARGUMENTS is the value returned by
	 __builtin_apply_args.  ARGSIZE is the number of bytes of
	 arguments that must be copied.  ??? How should this value be
	 computed?  We'll also need a safe worst case value for varargs
	 functions.  */
    case BUILT_IN_APPLY:
      if (!validate_arglist (arglist, POINTER_TYPE,
			     POINTER_TYPE, INTEGER_TYPE, VOID_TYPE)
	  && !validate_arglist (arglist, REFERENCE_TYPE,
				POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
	return const0_rtx;
      else
	{
	  int i;
	  tree t;
	  rtx ops[3];

	  for (t = arglist, i = 0; t; t = TREE_CHAIN (t), i++)
	    ops[i] = expand_expr (TREE_VALUE (t), NULL_RTX, VOIDmode, 0);

	  return expand_builtin_apply (ops[0], ops[1], ops[2]);
	}

      /* __builtin_return (RESULT) causes the function to return the
	 value described by RESULT.  RESULT is address of the block of
	 memory returned by __builtin_apply.  */
    case BUILT_IN_RETURN:
      if (validate_arglist (arglist, POINTER_TYPE, VOID_TYPE))
	expand_builtin_return (expand_expr (TREE_VALUE (arglist),
					    NULL_RTX, VOIDmode, 0));
      return const0_rtx;

    case BUILT_IN_SAVEREGS:
      return expand_builtin_saveregs ();

    case BUILT_IN_ARGS_INFO:
      return expand_builtin_args_info (exp);

      /* Return the address of the first anonymous stack arg.  */
    case BUILT_IN_NEXT_ARG:
      return expand_builtin_next_arg (arglist);

    case BUILT_IN_CLASSIFY_TYPE:
      return expand_builtin_classify_type (arglist);

    case BUILT_IN_CONSTANT_P:
      return expand_builtin_constant_p (exp);

    case BUILT_IN_FRAME_ADDRESS:
    case BUILT_IN_RETURN_ADDRESS:
      return expand_builtin_frame_address (exp);

    /* Returns the address of the area where the structure is returned.
       0 otherwise.  */
    case BUILT_IN_AGGREGATE_INCOMING_ADDRESS:
      if (arglist != 0
          || ! AGGREGATE_TYPE_P (TREE_TYPE (TREE_TYPE (current_function_decl)))
          || GET_CODE (DECL_RTL (DECL_RESULT (current_function_decl))) != MEM)
        return const0_rtx;
      else
        return XEXP (DECL_RTL (DECL_RESULT (current_function_decl)), 0);

    case BUILT_IN_ALLOCA:
      target = expand_builtin_alloca (arglist, target);
      if (target)
	return target;
      break;

    case BUILT_IN_FFS:
      target = expand_builtin_ffs (arglist, target, subtarget);
      if (target)
	return target;
      break;

    case BUILT_IN_STRLEN:
      target = expand_builtin_strlen (exp, target);
      if (target)
	return target;
      break;

    case BUILT_IN_STRCPY:
      target = expand_builtin_strcpy (exp, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_STRNCPY:
      target = expand_builtin_strncpy (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_STRCAT:
      target = expand_builtin_strcat (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_STRNCAT:
      target = expand_builtin_strncat (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_STRSPN:
      target = expand_builtin_strspn (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_STRCSPN:
      target = expand_builtin_strcspn (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_STRSTR:
      target = expand_builtin_strstr (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_STRPBRK:
      target = expand_builtin_strpbrk (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_INDEX:
    case BUILT_IN_STRCHR:
      target = expand_builtin_strchr (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_RINDEX:
    case BUILT_IN_STRRCHR:
      target = expand_builtin_strrchr (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_MEMCPY:
      target = expand_builtin_memcpy (arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_MEMSET:
      target = expand_builtin_memset (exp, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_BZERO:
      target = expand_builtin_bzero (exp);
      if (target)
	return target;
      break;

    case BUILT_IN_STRCMP:
      target = expand_builtin_strcmp (exp, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_STRNCMP:
      target = expand_builtin_strncmp (exp, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_BCMP:
    case BUILT_IN_MEMCMP:
      target = expand_builtin_memcmp (exp, arglist, target, mode);
      if (target)
	return target;
      break;

    case BUILT_IN_SETJMP:
      target = expand_builtin_setjmp (arglist, target);
      if (target)
	return target;
      break;

      /* __builtin_longjmp is passed a pointer to an array of five words.
	 It's similar to the C library longjmp function but works with
	 __builtin_setjmp above.  */
    case BUILT_IN_LONGJMP:
      if (!validate_arglist (arglist, POINTER_TYPE, INTEGER_TYPE, VOID_TYPE))
	break;
      else
	{
	  rtx buf_addr = expand_expr (TREE_VALUE (arglist), subtarget,
				      VOIDmode, 0);
	  rtx value = expand_expr (TREE_VALUE (TREE_CHAIN (arglist)),
				   NULL_RTX, VOIDmode, 0);

	  if (value != const1_rtx)
	    {
	      error ("__builtin_longjmp second argument must be 1");
	      return const0_rtx;
	    }

	  expand_builtin_longjmp (buf_addr, value);
	  return const0_rtx;
	}

    case BUILT_IN_TRAP:
      expand_builtin_trap ();
      return const0_rtx;

    case BUILT_IN_PUTCHAR:
    case BUILT_IN_PUTS:
    case BUILT_IN_FPUTC:
    case BUILT_IN_FWRITE:
    case BUILT_IN_PUTCHAR_UNLOCKED:
    case BUILT_IN_PUTS_UNLOCKED:
    case BUILT_IN_FPUTC_UNLOCKED:
    case BUILT_IN_FWRITE_UNLOCKED:
      break;
    case BUILT_IN_FPUTS:
      target = expand_builtin_fputs (arglist, ignore,/*unlocked=*/ 0);
      if (target)
	return target;
      break;
    case BUILT_IN_FPUTS_UNLOCKED:
      target = expand_builtin_fputs (arglist, ignore,/*unlocked=*/ 1);
      if (target)
	return target;
      break;

      /* Various hooks for the DWARF 2 __throw routine.  */
    case BUILT_IN_UNWIND_INIT:
      expand_builtin_unwind_init ();
      return const0_rtx;
    case BUILT_IN_DWARF_CFA:
      return virtual_cfa_rtx;
#ifdef DWARF2_UNWIND_INFO
    case BUILT_IN_DWARF_FP_REGNUM:
      return expand_builtin_dwarf_fp_regnum ();
    case BUILT_IN_INIT_DWARF_REG_SIZES:
      expand_builtin_init_dwarf_reg_sizes (TREE_VALUE (arglist));
      return const0_rtx;
#endif
    case BUILT_IN_FROB_RETURN_ADDR:
      return expand_builtin_frob_return_addr (TREE_VALUE (arglist));
    case BUILT_IN_EXTRACT_RETURN_ADDR:
      return expand_builtin_extract_return_addr (TREE_VALUE (arglist));
    case BUILT_IN_EH_RETURN:
      expand_builtin_eh_return (TREE_VALUE (arglist),
				TREE_VALUE (TREE_CHAIN (arglist)));
      return const0_rtx;
#ifdef EH_RETURN_DATA_REGNO
    case BUILT_IN_EH_RETURN_DATA_REGNO:
      return expand_builtin_eh_return_data_regno (arglist);
#endif
    case BUILT_IN_VARARGS_START:
      return expand_builtin_va_start (0, arglist);
    case BUILT_IN_STDARG_START:
      return expand_builtin_va_start (1, arglist);
    case BUILT_IN_VA_END:
      return expand_builtin_va_end (arglist);
    case BUILT_IN_VA_COPY:
      return expand_builtin_va_copy (arglist);
    case BUILT_IN_EXPECT:
      return expand_builtin_expect (arglist, target);
    case BUILT_IN_PREFETCH:
      expand_builtin_prefetch (arglist);
      return const0_rtx;


    default:			/* just do library call, if unknown builtin */
      error ("built-in function `%s' not currently supported",
	     IDENTIFIER_POINTER (DECL_NAME (fndecl)));
    }

  /* The switch statement above can drop through to cause the function
     to be called normally.  */
  return expand_call (exp, target, ignore);
}

/* Fold a call to __builtin_constant_p, if we know it will evaluate to a
   constant.  ARGLIST is the argument list of the call.  */

static tree
fold_builtin_constant_p (arglist)
     tree arglist;
{
  if (arglist == 0)
    return 0;

  arglist = TREE_VALUE (arglist);

  /* We return 1 for a numeric type that's known to be a constant
     value at compile-time or for an aggregate type that's a
     literal constant.  */
  STRIP_NOPS (arglist);

  /* If we know this is a constant, emit the constant of one.  */
  if (TREE_CODE_CLASS (TREE_CODE (arglist)) == 'c'
      || (TREE_CODE (arglist) == CONSTRUCTOR
	  && TREE_CONSTANT (arglist))
      || (TREE_CODE (arglist) == ADDR_EXPR
	  && TREE_CODE (TREE_OPERAND (arglist, 0)) == STRING_CST))
    return integer_one_node;

  /* If we aren't going to be running CSE or this expression
     has side effects, show we don't know it to be a constant.
     Likewise if it's a pointer or aggregate type since in those
     case we only want literals, since those are only optimized
     when generating RTL, not later.
     And finally, if we are compiling an initializer, not code, we
     need to return a definite result now; there's not going to be any
     more optimization done.  */
  if (TREE_SIDE_EFFECTS (arglist) || cse_not_expected
      || AGGREGATE_TYPE_P (TREE_TYPE (arglist))
      || POINTER_TYPE_P (TREE_TYPE (arglist))
      || cfun == 0)
    return integer_zero_node;

  return 0;
}

/* Fold a call to __builtin_classify_type.  */

static tree
fold_builtin_classify_type (arglist)
     tree arglist;
{
  if (arglist == 0)
    return build_int_2 (no_type_class, 0);

  return build_int_2 (type_to_class (TREE_TYPE (TREE_VALUE (arglist))), 0);
}

/* Used by constant folding to eliminate some builtin calls early.  EXP is
   the CALL_EXPR of a call to a builtin function.  */

tree
fold_builtin (exp)
     tree exp;
{
  tree fndecl = TREE_OPERAND (TREE_OPERAND (exp, 0), 0);
  tree arglist = TREE_OPERAND (exp, 1);
  enum built_in_function fcode = DECL_FUNCTION_CODE (fndecl);

  if (DECL_BUILT_IN_CLASS (fndecl) == BUILT_IN_MD)
    return 0;

  switch (fcode)
    {
    case BUILT_IN_CONSTANT_P:
      return fold_builtin_constant_p (arglist);

    case BUILT_IN_CLASSIFY_TYPE:
      return fold_builtin_classify_type (arglist);

    case BUILT_IN_STRLEN:
      if (validate_arglist (arglist, POINTER_TYPE, VOID_TYPE))
	{
	  tree len = c_strlen (TREE_VALUE (arglist));
	  if (len != 0)
	    return len;
	}
      break;

    default:
      break;
    }

  return 0;
}

static tree
build_function_call_expr (fn, arglist)
     tree fn, arglist;
{
  tree call_expr;

  call_expr = build1 (ADDR_EXPR, build_pointer_type (TREE_TYPE (fn)), fn);
  call_expr = build (CALL_EXPR, TREE_TYPE (TREE_TYPE (fn)),
		     call_expr, arglist);
  TREE_SIDE_EFFECTS (call_expr) = 1;
  return fold (call_expr);
}

/* This function validates the types of a function call argument list
   represented as a tree chain of parameters against a specified list
   of tree_codes.  If the last specifier is a 0, that represents an
   ellipses, otherwise the last specifier must be a VOID_TYPE.  */

static int
validate_arglist VPARAMS ((tree arglist, ...))
{
  enum tree_code code;
  int res = 0;

  VA_OPEN (ap, arglist);
  VA_FIXEDARG (ap, tree, arglist);

  do {
    code = va_arg (ap, enum tree_code);
    switch (code)
    {
    case 0:
      /* This signifies an ellipses, any further arguments are all ok.  */
      res = 1;
      goto end;
    case VOID_TYPE:
      /* This signifies an endlink, if no arguments remain, return
         true, otherwise return false.  */
      res = arglist == 0;
      goto end;
    default:
      /* If no parameters remain or the parameter's code does not
         match the specified code, return false.  Otherwise continue
         checking any remaining arguments.  */
      if (arglist == 0 || code != TREE_CODE (TREE_TYPE (TREE_VALUE (arglist))))
	goto end;
      break;
    }
    arglist = TREE_CHAIN (arglist);
  } while (1);

  /* We need gotos here since we can only have one VA_CLOSE in a
     function.  */
 end: ;
  VA_CLOSE (ap);

  return res;
}

/* Default version of target-specific builtin setup that does nothing.  */

void
default_init_builtins ()
{
}

/* Default target-specific builtin expander that does nothing.  */

rtx
default_expand_builtin (exp, target, subtarget, mode, ignore)
     tree exp ATTRIBUTE_UNUSED;
     rtx target ATTRIBUTE_UNUSED;
     rtx subtarget ATTRIBUTE_UNUSED;
     enum machine_mode mode ATTRIBUTE_UNUSED;
     int ignore ATTRIBUTE_UNUSED;
{
  return NULL_RTX;
}
