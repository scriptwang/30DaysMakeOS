/* Handle exceptional things in C++.
   Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001  Free Software Foundation, Inc.
   Contributed by Michael Tiemann <tiemann@cygnus.com>
   Rewritten by Mike Stump <mrs@cygnus.com>, based upon an
   initial re-implementation courtesy Tad Hunt.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "../gcc/libfuncs.h"
#include "cp-tree.h"
#include "../gcc/flags.h"
#include "../include/obstack.h"
#include "../gcc/output.h"
#include "../gcc/except.h"
#include "../gcc/toplev.h"
/* end of !kawai! */

static void push_eh_cleanup PARAMS ((tree));
static tree prepare_eh_type PARAMS ((tree));
static tree build_eh_type_type PARAMS ((tree));
static tree do_begin_catch PARAMS ((void));
static int dtor_nothrow PARAMS ((tree));
static tree do_end_catch PARAMS ((tree));
static void push_eh_cleanup PARAMS ((tree));
static bool decl_is_java_type PARAMS ((tree decl, int err));
static void initialize_handler_parm PARAMS ((tree, tree));
static tree do_allocate_exception PARAMS ((tree));
static int complete_ptr_ref_or_void_ptr_p PARAMS ((tree, tree));
static bool is_admissible_throw_operand PARAMS ((tree));
static int can_convert_eh PARAMS ((tree, tree));
static void check_handlers_1 PARAMS ((tree, tree));
static tree cp_protect_cleanup_actions PARAMS ((void));

/* !kawai */
#include "decl.h"
/* #include "obstack.h" */
/* end of !kawai! */

/* Sets up all the global eh stuff that needs to be initialized at the
   start of compilation.  */

void
init_exception_processing ()
{
  tree tmp;

  /* void std::terminate (); */
  push_namespace (std_identifier);
  tmp = build_function_type (void_type_node, void_list_node);
  terminate_node = build_cp_library_fn_ptr ("terminate", tmp);
  TREE_THIS_VOLATILE (terminate_node) = 1;
  TREE_NOTHROW (terminate_node) = 1;
  pop_namespace ();

  /* void __cxa_call_unexpected(void *); */
  tmp = tree_cons (NULL_TREE, ptr_type_node, void_list_node);
  tmp = build_function_type (void_type_node, tmp);
  call_unexpected_node
    = push_throw_library_fn (get_identifier ("__cxa_call_unexpected"), tmp);

  eh_personality_libfunc = init_one_libfunc (USING_SJLJ_EXCEPTIONS
					     ? "__gxx_personality_sj0"
					     : "__gxx_personality_v0");

  lang_eh_runtime_type = build_eh_type_type;
  lang_protect_cleanup_actions = &cp_protect_cleanup_actions;
}

/* Returns an expression to be executed if an unhandled exception is
   propagated out of a cleanup region.  */

static tree
cp_protect_cleanup_actions ()
{
  /* [except.terminate]

     When the destruction of an object during stack unwinding exits
     using an exception ... void terminate(); is called.  */
  return build_call (terminate_node, NULL_TREE);
}     

static tree
prepare_eh_type (type)
     tree type;
{
  if (type == NULL_TREE)
    return type;
  if (type == error_mark_node)
    return error_mark_node;

  /* peel back references, so they match.  */
  if (TREE_CODE (type) == REFERENCE_TYPE)
    type = TREE_TYPE (type);

  /* Peel off cv qualifiers.  */
  type = TYPE_MAIN_VARIANT (type);

  return type;
}

/* Build the address of a typeinfo decl for use in the runtime
   matching field of the exception model.   */

static tree
build_eh_type_type (type)
     tree type;
{
  tree exp;

  if (type == NULL_TREE || type == error_mark_node)
    return type;

  if (decl_is_java_type (type, 0))
    exp = build_java_class_ref (TREE_TYPE (type));
  else
    exp = get_tinfo_decl (type);

  mark_used (exp);
  exp = build1 (ADDR_EXPR, ptr_type_node, exp);

  return exp;
}

tree
build_exc_ptr ()
{
  return build (EXC_PTR_EXPR, ptr_type_node);
}

/* Build up a call to __cxa_begin_catch, to tell the runtime that the
   exception has been handled.  */

static tree
do_begin_catch ()
{
  tree fn;

  fn = get_identifier ("__cxa_begin_catch");
  if (IDENTIFIER_GLOBAL_VALUE (fn))
    fn = IDENTIFIER_GLOBAL_VALUE (fn);
  else
    {
      /* Declare void* __cxa_begin_catch (void *).  */
      tree tmp = tree_cons (NULL_TREE, ptr_type_node, void_list_node);
      fn = push_library_fn (fn, build_function_type (ptr_type_node, tmp));
    }

  return build_function_call (fn, tree_cons (NULL_TREE, build_exc_ptr (),
					     NULL_TREE));
}

/* Returns nonzero if cleaning up an exception of type TYPE (which can be
   NULL_TREE for a ... handler) will not throw an exception.  */

static int
dtor_nothrow (type)
     tree type;
{
  tree fn;

  if (type == NULL_TREE)
    return 0;

  if (! TYPE_HAS_DESTRUCTOR (type))
    return 1;

  fn = lookup_member (type, dtor_identifier, 0, 0);
  fn = TREE_VALUE (fn);
  return TREE_NOTHROW (fn);
}

/* Build up a call to __cxa_end_catch, to destroy the exception object
   for the current catch block if no others are currently using it.  */

static tree
do_end_catch (type)
     tree type;
{
  tree fn, cleanup;

  fn = get_identifier ("__cxa_end_catch");
  if (IDENTIFIER_GLOBAL_VALUE (fn))
    fn = IDENTIFIER_GLOBAL_VALUE (fn);
  else
    {
      /* Declare void __cxa_end_catch ().  */
      fn = push_void_library_fn (fn, void_list_node);
      /* This can throw if the destructor for the exception throws.  */
      TREE_NOTHROW (fn) = 0;
    }

  cleanup = build_function_call (fn, NULL_TREE);
  TREE_NOTHROW (cleanup) = dtor_nothrow (type);

  return cleanup;
}

/* This routine creates the cleanup for the current exception.  */

static void
push_eh_cleanup (type)
     tree type;
{
  finish_decl_cleanup (NULL_TREE, do_end_catch (type));
}

/* Return nonzero value if DECL is a Java type suitable for catch or
   throw.  */

static bool
decl_is_java_type (decl, err)
     tree decl;
     int err;
{
  bool r = (TREE_CODE (decl) == POINTER_TYPE
	    && TREE_CODE (TREE_TYPE (decl)) == RECORD_TYPE
	    && TYPE_FOR_JAVA (TREE_TYPE (decl)));

  if (err)
    {
      if (TREE_CODE (decl) == REFERENCE_TYPE
	  && TREE_CODE (TREE_TYPE (decl)) == RECORD_TYPE
	  && TYPE_FOR_JAVA (TREE_TYPE (decl)))
	{
	  /* Can't throw a reference.  */
	  error ("type `%T' is disallowed in Java `throw' or `catch'",
		    decl);
	}

      if (r)
	{
	  tree jthrow_node
	    = IDENTIFIER_GLOBAL_VALUE (get_identifier ("jthrowable"));

	  if (jthrow_node == NULL_TREE)
	    fatal_error
	      ("call to Java `catch' or `throw' with `jthrowable' undefined");

	  jthrow_node = TREE_TYPE (TREE_TYPE (jthrow_node));

	  if (! DERIVED_FROM_P (jthrow_node, TREE_TYPE (decl)))
	    {
	      /* Thrown object must be a Throwable.  */
	      error ("type `%T' is not derived from `java::lang::Throwable'",
			TREE_TYPE (decl));
	    }
	}
    }

  return r;
}

/* Select the personality routine to be used for exception handling,
   or issue an error if we need two different ones in the same
   translation unit.
   ??? At present eh_personality_libfunc is set to
   __gxx_personality_(sj|v)0 in init_exception_processing - should it
   be done here instead?  */
void
choose_personality_routine (lang)
     enum languages lang;
{
  static enum {
    chose_none,
    chose_cpp,
    chose_java,
    gave_error
  } state;

  switch (state)
    {
    case gave_error:
      return;

    case chose_cpp:
      if (lang != lang_cplusplus)
	goto give_error;
      return;

    case chose_java:
      if (lang != lang_java)
	goto give_error;
      return;

    case chose_none:
      ; /* proceed to language selection */
    }

  switch (lang)
    {
    case lang_cplusplus:
      state = chose_cpp;
      break;

    case lang_java:
      state = chose_java;
      eh_personality_libfunc = init_one_libfunc (USING_SJLJ_EXCEPTIONS
						 ? "__gcj_personality_sj0"
						 : "__gcj_personality_v0");
      break;

    default:
      abort ();
    }
  return;

 give_error:
  error ("mixing C++ and Java catches in a single translation unit");
  state = gave_error;
}

/* Initialize the catch parameter DECL.  */

static void 
initialize_handler_parm (decl, exp)
     tree decl;
     tree exp;
{
  tree init;
  tree init_type;

  /* Make sure we mark the catch param as used, otherwise we'll get a
     warning about an unused ((anonymous)).  */
  TREE_USED (decl) = 1;

  /* Figure out the type that the initializer is.  Pointers are returned
     adjusted by value from __cxa_begin_catch.  Others are returned by 
     reference.  */
  init_type = TREE_TYPE (decl);
  if (! TYPE_PTR_P (init_type)
      && TREE_CODE (init_type) != REFERENCE_TYPE)
    init_type = build_reference_type (init_type);

  choose_personality_routine (decl_is_java_type (init_type, 0)
			      ? lang_java : lang_cplusplus);

  /* Since pointers are passed by value, initialize a reference to
     pointer catch parm with the address of the temporary.  */
  if (TREE_CODE (init_type) == REFERENCE_TYPE
      && TYPE_PTR_P (TREE_TYPE (init_type)))
    exp = build_unary_op (ADDR_EXPR, exp, 1);

  exp = ocp_convert (init_type, exp, CONV_IMPLICIT|CONV_FORCE_TEMP, 0);

  init = convert_from_reference (exp);

  /* If the constructor for the catch parm exits via an exception, we
     must call terminate.  See eh23.C.  */
  if (TYPE_NEEDS_CONSTRUCTING (TREE_TYPE (decl)))
    {
      /* Generate the copy constructor call directly so we can wrap it.
	 See also expand_default_init.  */
      init = ocp_convert (TREE_TYPE (decl), init,
			  CONV_IMPLICIT|CONV_FORCE_TEMP, 0);
      init = build1 (MUST_NOT_THROW_EXPR, TREE_TYPE (init), init);
    }

  /* Let `cp_finish_decl' know that this initializer is ok.  */
  DECL_INITIAL (decl) = error_mark_node;
  decl = pushdecl (decl);

  start_decl_1 (decl);
  cp_finish_decl (decl, init, NULL_TREE,
		  LOOKUP_ONLYCONVERTING|DIRECT_BIND);
}

/* Call this to start a catch block.  DECL is the catch parameter.  */

tree
expand_start_catch_block (decl)
     tree decl;
{
  tree exp = NULL_TREE;
  tree type;
  bool is_java;

  if (! doing_eh (1))
    return NULL_TREE;

  /* Make sure this declaration is reasonable.  */
  if (decl && !complete_ptr_ref_or_void_ptr_p (TREE_TYPE (decl), NULL_TREE))
    decl = NULL_TREE;

  if (decl)
    type = prepare_eh_type (TREE_TYPE (decl));
  else
    type = NULL_TREE;

  is_java = false;
  if (decl)
    {
      tree init;

      if (decl_is_java_type (type, 1))
	{
	  /* Java only passes object via pointer and doesn't require
	     adjusting.  The java object is immediately before the
	     generic exception header.  */
	  init = build_exc_ptr ();
	  init = build1 (NOP_EXPR, build_pointer_type (type), init);
	  init = build (MINUS_EXPR, TREE_TYPE (init), init,
			TYPE_SIZE_UNIT (TREE_TYPE (init)));
	  init = build_indirect_ref (init, NULL);
	  is_java = true;
	}
      else
	{
	  /* C++ requires that we call __cxa_begin_catch to get the
	     pointer to the actual object.  */
	  init = do_begin_catch ();
	}
	  
      exp = create_temporary_var (ptr_type_node);
      DECL_REGISTER (exp) = 1;
      cp_finish_decl (exp, init, NULL_TREE, LOOKUP_ONLYCONVERTING);
      finish_expr_stmt (build_modify_expr (exp, INIT_EXPR, init));
    }
  else
    finish_expr_stmt (do_begin_catch ());

  /* C++ requires that we call __cxa_end_catch at the end of
     processing the exception.  */
  if (! is_java)
    push_eh_cleanup (type);

  if (decl)
    initialize_handler_parm (decl, exp);

  return type;
}


/* Call this to end a catch block.  Its responsible for emitting the
   code to handle jumping back to the correct place, and for emitting
   the label to jump to if this catch block didn't match.  */

void
expand_end_catch_block ()
{
  if (! doing_eh (1))
    return;

  /* The exception being handled is rethrown if control reaches the end of
     a handler of the function-try-block of a constructor or destructor.  */
  if (in_function_try_handler
      && (DECL_CONSTRUCTOR_P (current_function_decl)
	  || DECL_DESTRUCTOR_P (current_function_decl)))
    finish_expr_stmt (build_throw (NULL_TREE));
}

tree
begin_eh_spec_block ()
{
  tree r = build_stmt (EH_SPEC_BLOCK, NULL_TREE, NULL_TREE);
  add_stmt (r);
  return r;
}

void
finish_eh_spec_block (raw_raises, eh_spec_block)
     tree raw_raises;
     tree eh_spec_block;
{
  tree raises;

  RECHAIN_STMTS (eh_spec_block, EH_SPEC_STMTS (eh_spec_block));

  /* Strip cv quals, etc, from the specification types.  */
  for (raises = NULL_TREE;
       raw_raises && TREE_VALUE (raw_raises);
       raw_raises = TREE_CHAIN (raw_raises))
    raises = tree_cons (NULL_TREE, prepare_eh_type (TREE_VALUE (raw_raises)),
			raises);

  EH_SPEC_RAISES (eh_spec_block) = raises;
}

/* Return a pointer to a buffer for an exception object of type TYPE.  */

static tree
do_allocate_exception (type)
     tree type;
{
  tree fn;

  fn = get_identifier ("__cxa_allocate_exception");
  if (IDENTIFIER_GLOBAL_VALUE (fn))
    fn = IDENTIFIER_GLOBAL_VALUE (fn);
  else
    {
      /* Declare void *__cxa_allocate_exception(size_t).  */
      tree tmp = tree_cons (NULL_TREE, c_size_type_node, void_list_node);
      fn = push_library_fn (fn, build_function_type (ptr_type_node, tmp));
    }
  
  return build_function_call (fn, tree_cons (NULL_TREE, size_in_bytes (type),
					     NULL_TREE));
}

#if 0
/* Call __cxa_free_exception from a cleanup.  This is never invoked
   directly.  */

static tree
do_free_exception (ptr)
     tree ptr;
{
  tree fn;

  fn = get_identifier ("__cxa_free_exception");
  if (IDENTIFIER_GLOBAL_VALUE (fn))
    fn = IDENTIFIER_GLOBAL_VALUE (fn);
  else
    {
      /* Declare void __cxa_free_exception (void *).  */
      fn = push_void_library_fn (fn, tree_cons (NULL_TREE, ptr_type_node,
						void_list_node));
    }

  return build_function_call (fn, tree_cons (NULL_TREE, ptr, NULL_TREE));
}
#endif

/* Build a throw expression.  */

tree
build_throw (exp)
     tree exp;
{
  tree fn;

  if (exp == error_mark_node)
    return exp;

  if (processing_template_decl)
    return build_min (THROW_EXPR, void_type_node, exp);

  if (exp == null_node)
    warning ("throwing NULL, which has integral, not pointer type");
  
  if (exp != NULL_TREE)
    {
      if (!is_admissible_throw_operand (exp))
        return error_mark_node;
    }

  if (! doing_eh (1))
    return error_mark_node;

  if (exp && decl_is_java_type (TREE_TYPE (exp), 1))
    {
      tree fn = get_identifier ("_Jv_Throw");
      if (IDENTIFIER_GLOBAL_VALUE (fn))
	fn = IDENTIFIER_GLOBAL_VALUE (fn);
      else
	{
	  /* Declare void _Jv_Throw (void *).  */
	  tree tmp = tree_cons (NULL_TREE, ptr_type_node, void_list_node);
	  tmp = build_function_type (ptr_type_node, tmp);
	  fn = push_throw_library_fn (fn, tmp);
	}

      exp = build_function_call (fn, tree_cons (NULL_TREE, exp, NULL_TREE));
    }
  else if (exp)
    {
      tree throw_type;
      tree cleanup;
      tree stmt_expr;
      tree compound_stmt;
      tree object, ptr;
      tree tmp;

      fn = get_identifier ("__cxa_throw");
      if (IDENTIFIER_GLOBAL_VALUE (fn))
	fn = IDENTIFIER_GLOBAL_VALUE (fn);
      else
	{
	  /* The CLEANUP_TYPE is the internal type of a destructor.  */
	  if (cleanup_type == NULL_TREE)
	    {
	      tmp = void_list_node;
	      tmp = tree_cons (NULL_TREE, ptr_type_node, tmp);
	      tmp = build_function_type (void_type_node, tmp);
	      cleanup_type = build_pointer_type (tmp);
	    }

	  /* Declare void __cxa_throw (void*, void*, void (*)(void*)).  */
	  /* ??? Second argument is supposed to be "std::type_info*".  */
	  tmp = void_list_node;
	  tmp = tree_cons (NULL_TREE, cleanup_type, tmp);
	  tmp = tree_cons (NULL_TREE, ptr_type_node, tmp);
	  tmp = tree_cons (NULL_TREE, ptr_type_node, tmp);
	  tmp = build_function_type (void_type_node, tmp);
	  fn = push_throw_library_fn (fn, tmp);
	}

      begin_init_stmts (&stmt_expr, &compound_stmt);

      /* throw expression */
      /* First, decay it.  */
      exp = decay_conversion (exp);

      /* OK, this is kind of wacky.  The standard says that we call
	 terminate when the exception handling mechanism, after
	 completing evaluation of the expression to be thrown but
	 before the exception is caught (_except.throw_), calls a
	 user function that exits via an uncaught exception.

	 So we have to protect the actual initialization of the
	 exception object with terminate(), but evaluate the
	 expression first.  Since there could be temps in the
	 expression, we need to handle that, too.  We also expand
	 the call to __cxa_allocate_exception first (which doesn't
	 matter, since it can't throw).  */

      my_friendly_assert (stmts_are_full_exprs_p () == 1, 19990926);

      /* Store the throw expression into a temp.  This can be less
	 efficient than storing it into the allocated space directly, but
	 if we allocated the space first we would have to deal with
	 cleaning it up if evaluating this expression throws.  */
      if (TREE_SIDE_EFFECTS (exp))
	{
	  tmp = create_temporary_var (TREE_TYPE (exp));
	  DECL_INITIAL (tmp) = exp;
	  cp_finish_decl (tmp, exp, NULL_TREE, LOOKUP_ONLYCONVERTING);
	  exp = tmp;
	}

      /* Allocate the space for the exception.  */
      ptr = create_temporary_var (ptr_type_node);
      DECL_REGISTER (ptr) = 1;
      cp_finish_decl (ptr, NULL_TREE, NULL_TREE, LOOKUP_ONLYCONVERTING);
      tmp = do_allocate_exception (TREE_TYPE (exp));
      tmp = build_modify_expr (ptr, INIT_EXPR, tmp);
      finish_expr_stmt (tmp);

      object = build1 (NOP_EXPR, build_pointer_type (TREE_TYPE (exp)), ptr);
      object = build_indirect_ref (object, NULL);

      exp = build_modify_expr (object, INIT_EXPR, exp);
      if (exp == error_mark_node)
	error ("  in thrown expression");

      exp = build1 (MUST_NOT_THROW_EXPR, TREE_TYPE (exp), exp);
      finish_expr_stmt (exp);

      throw_type = build_eh_type_type (prepare_eh_type (TREE_TYPE (object)));

      if (TYPE_HAS_DESTRUCTOR (TREE_TYPE (object)))
	{
	  cleanup = lookup_fnfields (TYPE_BINFO (TREE_TYPE (object)),
				     complete_dtor_identifier, 0);
	  cleanup = TREE_VALUE (cleanup);
	  mark_used (cleanup);
	  mark_addressable (cleanup);
	  /* Pretend it's a normal function.  */
	  cleanup = build1 (ADDR_EXPR, cleanup_type, cleanup);
	}
      else
	{
	  cleanup = build_int_2 (0, 0);
	  TREE_TYPE (cleanup) = cleanup_type;
	}

      tmp = tree_cons (NULL_TREE, cleanup, NULL_TREE);
      tmp = tree_cons (NULL_TREE, throw_type, tmp);
      tmp = tree_cons (NULL_TREE, ptr, tmp);
      tmp = build_function_call (fn, tmp);

      /* ??? Indicate that this function call throws throw_type.  */

      finish_expr_stmt (tmp);

      exp = finish_init_stmts (stmt_expr, compound_stmt);
    }
  else
    {
      /* Rethrow current exception.  */

      tree fn = get_identifier ("__cxa_rethrow");
      if (IDENTIFIER_GLOBAL_VALUE (fn))
	fn = IDENTIFIER_GLOBAL_VALUE (fn);
      else
	{
	  /* Declare void __cxa_rethrow (void).  */
	  fn = push_throw_library_fn
	    (fn, build_function_type (void_type_node, void_list_node));
	}

      exp = build_function_call (fn, NULL_TREE);
    }

  exp = build1 (THROW_EXPR, void_type_node, exp);

  return exp;
}

/* Make sure TYPE is complete, pointer to complete, reference to
   complete, or pointer to cv void. Issue diagnostic on failure.
   Return the zero on failure and non-zero on success. FROM can be
   the expr or decl from whence TYPE came, if available.  */

static int
complete_ptr_ref_or_void_ptr_p (type, from)
     tree type;
     tree from;
{
  int is_ptr;
  
  /* Check complete.  */
  type = complete_type_or_else (type, from);
  if (!type)
    return 0;
  
  /* Or a pointer or ref to one, or cv void *.  */
  is_ptr = TREE_CODE (type) == POINTER_TYPE;
  if (is_ptr || TREE_CODE (type) == REFERENCE_TYPE)
    {
      tree core = TREE_TYPE (type);
  
      if (is_ptr && VOID_TYPE_P (core))
        /* OK */;
      else if (!complete_type_or_else (core, from))
        return 0;
    }
  return 1;
}

/* Return truth-value if EXPRESSION is admissible in throw-expression,
   i.e. if it is not of incomplete type or a pointer/reference to such
   a type or of an abstract class type.  */

static bool
is_admissible_throw_operand (expr)
     tree expr;
{
  tree type = TREE_TYPE (expr);

  /* 15.1/4 [...] The type of the throw-expression shall not be an
            incomplete type, or a pointer or a reference to an incomplete
            type, other than void*, const void*, volatile void*, or
            const volatile void*.  Except for these restriction and the
            restrictions on type matching mentioned in 15.3, the operand
            of throw is treated exactly as a function argument in a call
            (5.2.2) or the operand of a return statement.  */
  if (!complete_ptr_ref_or_void_ptr_p (type, expr))
    return false;

  /* 10.4/3 An abstract class shall not be used as a parameter type,
            as a function return type or as type of an explicit
            conversion.  */
  else if (CLASS_TYPE_P (type) && CLASSTYPE_PURE_VIRTUALS (type))
    {
      error ("expression '%E' of abstract class type '%T' cannot be used in throw-expression", expr, type);
      return false;
    }

  return true;
}

/* Returns nonzero if FN is a declaration of a standard C library
   function which is known not to throw.

   [lib.res.on.exception.handling]: None of the functions from the
   Standard C library shall report an error by throwing an
   exception, unless it calls a program-supplied function that
   throws an exception.  */

#include "cfns.h"

int
nothrow_libfn_p (fn)
     tree fn;
{
  tree id;

  if (TREE_PUBLIC (fn)
      && DECL_EXTERNAL (fn)
      && DECL_NAMESPACE_SCOPE_P (fn)
      && DECL_EXTERN_C_P (fn))
    /* OK */;
  else
    /* Can't be a C library function.  */
    return 0;

  id = DECL_ASSEMBLER_NAME (fn);
  return !!libc_name_p (IDENTIFIER_POINTER (id), IDENTIFIER_LENGTH (id));
}

/* Returns nonzero if an exception of type FROM will be caught by a
   handler for type TO, as per [except.handle].  */

static int
can_convert_eh (to, from)
     tree to, from;
{
  if (TREE_CODE (to) == REFERENCE_TYPE)
    to = TREE_TYPE (to);
  if (TREE_CODE (from) == REFERENCE_TYPE)
    from = TREE_TYPE (from);

  if (TREE_CODE (to) == POINTER_TYPE && TREE_CODE (from) == POINTER_TYPE)
    {
      to = TREE_TYPE (to);
      from = TREE_TYPE (from);

      if (! at_least_as_qualified_p (to, from))
	return 0;

      if (TREE_CODE (to) == VOID_TYPE)
	return 1;

      /* else fall through */
    }

  if (CLASS_TYPE_P (to) && CLASS_TYPE_P (from)
      && PUBLICLY_UNIQUELY_DERIVED_P (to, from))
    return 1;

  return 0;
}

/* Check whether any of HANDLERS are shadowed by another handler accepting
   TYPE.  Note that the shadowing may not be complete; even if an exception
   of type B would be caught by a handler for A, there could be a derived
   class C for which A is an ambiguous base but B is not, so the handler
   for B would catch an exception of type C.  */

static void
check_handlers_1 (master, handlers)
     tree master;
     tree handlers;
{
  tree type = TREE_TYPE (master);
  tree handler;

  for (handler = handlers; handler; handler = TREE_CHAIN (handler))
    if (TREE_TYPE (handler)
	&& can_convert_eh (type, TREE_TYPE (handler)))
      {
	lineno = STMT_LINENO (handler);
	warning ("exception of type `%T' will be caught",
		    TREE_TYPE (handler));
	lineno = STMT_LINENO (master);
	warning ("   by earlier handler for `%T'", type);
	break;
      }
}

/* Given a chain of HANDLERs, make sure that they're OK.  */

void
check_handlers (handlers)
     tree handlers;
{
  tree handler;
  int save_line = lineno;
  for (handler = handlers; handler; handler = TREE_CHAIN (handler))
    {
      if (TREE_CHAIN (handler) == NULL_TREE)
	/* No more handlers; nothing to shadow.  */;
      else if (TREE_TYPE (handler) == NULL_TREE)
	{
	  lineno = STMT_LINENO (handler);
	  pedwarn
	    ("`...' handler must be the last handler for its try block");
	}
      else
	check_handlers_1 (handler, TREE_CHAIN (handler));
    }
  lineno = save_line;
}
