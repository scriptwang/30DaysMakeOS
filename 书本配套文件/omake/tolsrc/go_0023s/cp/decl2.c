/* Process declarations and variables for C compiler.
   Copyright (C) 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Hacked by Michael Tiemann (tiemann@cygnus.com)

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


/* Process declarations and symbol lookup for C front end.
   Also constructs types; the standard scalar types at initialization,
   and structure, union, array and enum types when they are declared.  */

/* ??? not all decl nodes are given the most useful possible
   line numbers.  For example, the CONST_DECLs for enum values.  */

/* !kawai! */
#include "../gcc/config.h"
#include "../gcc/system.h"
#include "../gcc/tree.h"
#include "../gcc/rtl.h"
#include "../gcc/expr.h"
#include "../gcc/flags.h"
#include "cp-tree.h"
#include "decl.h"
#include "lex.h"
#include "../gcc/output.h"
#include "../gcc/except.h"
#include "../gcc/toplev.h"
#include "../gcc/ggc.h"
#include "../gcc/timevar.h"
#include "../gcc/cpplib.h"
#include "../gcc/target.h"
extern cpp_reader *parse_in;
/* end of !kawai! */

/* This structure contains information about the initializations
   and/or destructions required for a particular priority level.  */
typedef struct priority_info_s {
  /* Non-zero if there have been any initializations at this priority
     throughout the translation unit.  */
  int initializations_p;
  /* Non-zero if there have been any destructions at this priority
     throughout the translation unit.  */
  int destructions_p;
} *priority_info;

static void mark_vtable_entries PARAMS ((tree));
static void grok_function_init PARAMS ((tree, tree));
static int finish_vtable_vardecl PARAMS ((tree *, void *));
static int prune_vtable_vardecl PARAMS ((tree *, void *));
static int is_namespace_ancestor PARAMS ((tree, tree));
static void add_using_namespace PARAMS ((tree, tree, int));
static tree ambiguous_decl PARAMS ((tree, tree, tree,int));
static tree build_anon_union_vars PARAMS ((tree, tree*, int, int));
static int acceptable_java_type PARAMS ((tree));
static void output_vtable_inherit PARAMS ((tree));
static tree start_objects PARAMS ((int, int));
static void finish_objects PARAMS ((int, int, tree));
static tree merge_functions PARAMS ((tree, tree));
static tree decl_namespace PARAMS ((tree));
static tree validate_nonmember_using_decl PARAMS ((tree, tree *, tree *));
static void do_nonmember_using_decl PARAMS ((tree, tree, tree, tree,
					   tree *, tree *));
static tree start_static_storage_duration_function PARAMS ((void));
static void finish_static_storage_duration_function PARAMS ((tree));
static priority_info get_priority_info PARAMS ((int));
static void do_static_initialization PARAMS ((tree, tree));
static void do_static_destruction PARAMS ((tree));
static tree start_static_initialization_or_destruction PARAMS ((tree, int));
static void finish_static_initialization_or_destruction PARAMS ((tree));
static void generate_ctor_or_dtor_function PARAMS ((int, int));
static int generate_ctor_and_dtor_functions_for_priority
                                  PARAMS ((splay_tree_node, void *));
static tree prune_vars_needing_no_initialization PARAMS ((tree));
static void write_out_vars PARAMS ((tree));
static void import_export_class	PARAMS ((tree));
static tree key_method PARAMS ((tree));
static int compare_options PARAMS ((const PTR, const PTR));
static tree get_guard_bits PARAMS ((tree));

/* A list of static class variables.  This is needed, because a
   static class variable can be declared inside the class without
   an initializer, and then initialized, statically, outside the class.  */
static varray_type pending_statics;
#define pending_statics_used \
  (pending_statics ? pending_statics->elements_used : 0)

/* A list of functions which were declared inline, but which we
   may need to emit outline anyway.  */
static varray_type deferred_fns;
#define deferred_fns_used \
  (deferred_fns ? deferred_fns->elements_used : 0)

/* Flag used when debugging spew.c */

extern int spew_debug;

/* Nonzero if we're done parsing and into end-of-file activities.  */

int at_eof;

/* Functions called along with real static constructors and destructors.  */

tree static_ctors;
tree static_dtors;

/* The :: namespace. */

tree global_namespace;

/* C (and C++) language-specific option variables.  */

/* Nonzero means don't recognize the keyword `asm'.  */

int flag_no_asm;

/* Nonzero means don't recognize any extension keywords.  */

int flag_no_gnu_keywords;

/* Nonzero means do some things the same way PCC does.  Only provided so
   the compiler will link.  */

int flag_traditional;

/* Nonzero means to treat bitfields as unsigned unless they say `signed'.  */

int flag_signed_bitfields = 1;

/* Nonzero means enable obscure standard features and disable GNU
   extensions that might cause standard-compliant code to be
   miscompiled.  */

int flag_ansi;

/* Nonzero means do emit exported implementations of functions even if
   they can be inlined.  */

int flag_implement_inlines = 1;

/* Nonzero means do emit exported implementations of templates, instead of
   multiple static copies in each file that needs a definition.  */

int flag_external_templates;

/* Nonzero means that the decision to emit or not emit the implementation of a
   template depends on where the template is instantiated, rather than where
   it is defined.  */

int flag_alt_external_templates;

/* Nonzero means that implicit instantiations will be emitted if needed.  */

int flag_implicit_templates = 1;

/* Nonzero means that implicit instantiations of inline templates will be
   emitted if needed, even if instantiations of non-inline templates
   aren't.  */

int flag_implicit_inline_templates = 1;

/* Nonzero means warn about implicit declarations.  */

int warn_implicit = 1;

/* Nonzero means warn about usage of long long when `-pedantic'.  */

int warn_long_long = 1;

/* Nonzero means warn when all ctors or dtors are private, and the class
   has no friends.  */

int warn_ctor_dtor_privacy = 1;

/* Nonzero means generate separate instantiation control files and juggle
   them at link time.  */

int flag_use_repository;

/* Nonzero if we want to issue diagnostics that the standard says are not
   required.  */

int flag_optional_diags = 1;

/* Nonzero means give string constants the type `const char *', as mandated
   by the standard.  */

int flag_const_strings = 1;

/* Nonzero means warn about deprecated conversion from string constant to
   `char *'.  */

int warn_write_strings;

/* Nonzero means warn about pointer casts that can drop a type qualifier
   from the pointer target type.  */

int warn_cast_qual;

/* Nonzero means warn about sizeof(function) or addition/subtraction
   of function pointers.  */

int warn_pointer_arith = 1;

/* Nonzero means warn for any function def without prototype decl.  */

int warn_missing_prototypes;

/* Nonzero means warn about multiple (redundant) decls for the same single
   variable or function.  */

int warn_redundant_decls;

/* Warn if initializer is not completely bracketed.  */

int warn_missing_braces;

/* Warn about comparison of signed and unsigned values.  */

int warn_sign_compare;

/* Warn about testing equality of floating point numbers. */

int warn_float_equal = 0;

/* Warn about functions which might be candidates for format attributes.  */

int warn_missing_format_attribute;

/* Warn about a subscript that has type char.  */

int warn_char_subscripts;

/* Warn if a type conversion is done that might have confusing results.  */

int warn_conversion;

/* Warn if adding () is suggested.  */

int warn_parentheses;

/* Non-zero means warn in function declared in derived class has the
   same name as a virtual in the base class, but fails to match the
   type signature of any virtual function in the base class.  */

int warn_overloaded_virtual;

/* Non-zero means warn when declaring a class that has a non virtual
   destructor, when it really ought to have a virtual one.  */

int warn_nonvdtor;

/* Non-zero means warn when the compiler will reorder code.  */

int warn_reorder;

/* Non-zero means warn when synthesis behavior differs from Cfront's.  */

int warn_synth;

/* Non-zero means warn when we convert a pointer to member function
   into a pointer to (void or function).  */

int warn_pmf2ptr = 1;

/* Nonzero means warn about violation of some Effective C++ style rules.  */

int warn_ecpp;

/* Nonzero means warn where overload resolution chooses a promotion from
   unsigned to signed over a conversion to an unsigned of the same size.  */

int warn_sign_promo;

/* Nonzero means warn when an old-style cast is used.  */

int warn_old_style_cast;

/* Warn about #pragma directives that are not recognised.  */      

int warn_unknown_pragmas; /* Tri state variable.  */  

/* Nonzero means warn about use of multicharacter literals.  */

int warn_multichar = 1;

/* Nonzero means warn when non-templatized friend functions are
   declared within a template */

int warn_nontemplate_friend = 1;

/* Nonzero means complain about deprecated features.  */

int warn_deprecated = 1;

/* Nonzero means `$' can be in an identifier.  */

#ifndef DOLLARS_IN_IDENTIFIERS
#define DOLLARS_IN_IDENTIFIERS 1
#endif
int dollars_in_ident = DOLLARS_IN_IDENTIFIERS;

/* Nonzero means allow Microsoft extensions without a pedwarn.  */

int flag_ms_extensions;

/* C++ specific flags.  */   

/* Nonzero means we should attempt to elide constructors when possible.  */

int flag_elide_constructors = 1;

/* Nonzero means that member functions defined in class scope are
   inline by default.  */

int flag_default_inline = 1;

/* Controls whether compiler generates 'type descriptor' that give
   run-time type information.  */

int flag_rtti = 1;

/* Nonzero if we want to support huge (> 2^(sizeof(short)*8-1) bytes)
   objects.  */

int flag_huge_objects;

/* Nonzero if we want to conserve space in the .o files.  We do this
   by putting uninitialized data and runtime initialized data into
   .common instead of .data at the expense of not flagging multiple
   definitions.  */

int flag_conserve_space;

/* Nonzero if we want to obey access control semantics.  */

int flag_access_control = 1;

/* Nonzero if we want to understand the operator names, i.e. 'bitand'.  */

int flag_operator_names = 1;

/* Nonzero if we want to check the return value of new and avoid calling
   constructors if it is a null pointer.  */

int flag_check_new;

/* Nonzero if we want the new ISO rules for pushing a new scope for `for'
   initialization variables.
   0: Old rules, set by -fno-for-scope.
   2: New ISO rules, set by -ffor-scope.
   1: Try to implement new ISO rules, but with backup compatibility
   (and warnings).  This is the default, for now.  */

int flag_new_for_scope = 1;

/* Nonzero if we want to emit defined symbols with common-like linkage as
   weak symbols where possible, in order to conform to C++ semantics.
   Otherwise, emit them as local symbols.  */

int flag_weak = 1;

/* Nonzero to use __cxa_atexit, rather than atexit, to register
   destructors for local statics and global objects.  */

int flag_use_cxa_atexit = DEFAULT_USE_CXA_ATEXIT;

/* Maximum template instantiation depth.  This limit is rather
   arbitrary, but it exists to limit the time it takes to notice
   infinite template instantiations.  */

int max_tinst_depth = 500;

/* Nonzero means output .vtable_{entry,inherit} for use in doing vtable gc.  */

int flag_vtable_gc;

/* Nonzero means make the default pedwarns warnings instead of errors.
   The value of this flag is ignored if -pedantic is specified.  */

int flag_permissive;

/* Nonzero means to implement standard semantics for exception
   specifications, calling unexpected if an exception is thrown that
   doesn't match the specification.  Zero means to treat them as
   assertions and optimize accordingly, but not check them.  */

int flag_enforce_eh_specs = 1;

/* Table of language-dependent -f options.
   STRING is the option name.  VARIABLE is the address of the variable.
   ON_VALUE is the value to store in VARIABLE
    if `-fSTRING' is seen as an option.
   (If `-fno-STRING' is seen as an option, the opposite value is stored.)  */

static const struct { const char *const string; int *const variable; const int on_value;}
lang_f_options[] =
{
  /* C/C++ options.  */
  {"signed-char", &flag_signed_char, 1},
  {"unsigned-char", &flag_signed_char, 0},
  {"signed-bitfields", &flag_signed_bitfields, 1},
  {"unsigned-bitfields", &flag_signed_bitfields, 0},
  {"short-enums", &flag_short_enums, 1},
  {"short-double", &flag_short_double, 1},
  {"short-wchar", &flag_short_wchar, 1},
  {"asm", &flag_no_asm, 0},
  {"builtin", &flag_no_builtin, 0},

  /* C++-only options.  */
  {"access-control", &flag_access_control, 1},
  {"check-new", &flag_check_new, 1},
  {"conserve-space", &flag_conserve_space, 1},
  {"const-strings", &flag_const_strings, 1},
  {"default-inline", &flag_default_inline, 1},
  {"dollars-in-identifiers", &dollars_in_ident, 1},
  {"elide-constructors", &flag_elide_constructors, 1},
  {"enforce-eh-specs", &flag_enforce_eh_specs, 1},
  {"external-templates", &flag_external_templates, 1},
  {"for-scope", &flag_new_for_scope, 2},
  {"gnu-keywords", &flag_no_gnu_keywords, 0},
  {"handle-exceptions", &flag_exceptions, 1},
  {"implement-inlines", &flag_implement_inlines, 1},
  {"implicit-inline-templates", &flag_implicit_inline_templates, 1},
  {"implicit-templates", &flag_implicit_templates, 1},
  {"ms-extensions", &flag_ms_extensions, 1},
  {"nonansi-builtins", &flag_no_nonansi_builtin, 0},
  {"operator-names", &flag_operator_names, 1},
  {"optional-diags", &flag_optional_diags, 1},
  {"permissive", &flag_permissive, 1},
  {"repo", &flag_use_repository, 1},
  {"rtti", &flag_rtti, 1},
  {"stats", &flag_detailed_statistics, 1},
  {"vtable-gc", &flag_vtable_gc, 1},
  {"use-cxa-atexit", &flag_use_cxa_atexit, 1},
  {"weak", &flag_weak, 1}
};

/* The list of `-f' options that we no longer support.  The `-f'
   prefix is not given in this table.  The `-fno-' variants are not
   listed here.  This table must be kept in alphabetical order.  */
static const char * const unsupported_options[] = {
  "all-virtual",
  "cond-mismatch",
  "enum-int-equiv",
  "guiding-decls",
  "honor-std",
  "huge-objects",
  "labels-ok",
  "new-abi",
  "nonnull-objects",
  "squangle",
  "strict-prototype",
  "this-is-variable",
  "vtable-thunks",
  "xref"
};

/* Compare two option strings, pointed two by P1 and P2, for use with
   bsearch.  */

static int
compare_options (p1, p2)
     const PTR p1;
     const PTR p2;
{
  return strcmp (*((const char *const *) p1), *((const char *const *) p2));
}

/* Decode the string P as a language-specific option.
   Return the number of strings consumed for a valid option.
   Otherwise return 0.  Should not complain if it does not
   recognise the option.  */

int   
cxx_decode_option (argc, argv)
     int argc;
     char **argv;
{
  int strings_processed;
  const char *p = argv[0];

  strings_processed = cpp_handle_option (parse_in, argc, argv, 0);

  if (!strcmp (p, "-ftraditional") || !strcmp (p, "-traditional"))
    /* ignore */;
  else if (p[0] == '-' && p[1] == 'f')
    {
      /* Some kind of -f option.
	 P's value is the option sans `-f'.
	 Search for it in the table of options.  */
      const char *option_value = NULL;
      const char *positive_option;
      size_t j;

      p += 2;
      /* Try special -f options.  */

      /* See if this is one of the options no longer supported.  We
	 used to support these options, so we continue to accept them,
	 with a warning.  */
      if (strncmp (p, "no-", strlen ("no-")) == 0)
	positive_option = p + strlen ("no-");
      else
	positive_option = p;

      /* If the option is present, issue a warning.  Indicate to our
	 caller that the option was processed successfully.  */
      if (bsearch (&positive_option, 
		   unsupported_options, 
		   (sizeof (unsupported_options) 
		    / sizeof (unsupported_options[0])),
		   sizeof (unsupported_options[0]),
		   compare_options))
	{
	  warning ("-f%s is no longer supported", p);
	  return 1;
	}

      if (!strcmp (p, "handle-exceptions")
	  || !strcmp (p, "no-handle-exceptions"))
	warning ("-fhandle-exceptions has been renamed to -fexceptions (and is now on by default)");
      else if (! strcmp (p, "alt-external-templates"))
	{
	  flag_external_templates = 1;
	  flag_alt_external_templates = 1;
          cp_deprecated ("-falt-external-templates");
	}
      else if (! strcmp (p, "no-alt-external-templates"))
	flag_alt_external_templates = 0;
      else if (!strcmp (p, "repo"))
	{
	  flag_use_repository = 1;
	  flag_implicit_templates = 0;
	}
      else if (!strcmp (p, "external-templates"))
        {
          flag_external_templates = 1;
          cp_deprecated ("-fexternal-templates");
        }
      else if ((option_value
                = skip_leading_substring (p, "template-depth-")))
	max_tinst_depth
	  = read_integral_parameter (option_value, p - 2, max_tinst_depth);
      else if ((option_value
                = skip_leading_substring (p, "name-mangling-version-")))
	{
	  warning ("-fname-mangling-version is no longer supported");
	  return 1;
	}
      else if (dump_switch_p (p))
	;
      else 
	{
	  int found = 0;

	  for (j = 0;
	       !found && j < (sizeof (lang_f_options) 
			      / sizeof (lang_f_options[0]));
	       j++)
	    {
	      if (!strcmp (p, lang_f_options[j].string))
		{
		  *lang_f_options[j].variable = lang_f_options[j].on_value;
		  /* A goto here would be cleaner,
		     but breaks the VAX pcc.  */
		  found = 1;
		}
	      else if (p[0] == 'n' && p[1] == 'o' && p[2] == '-'
		       && ! strcmp (p+3, lang_f_options[j].string))
		{
		  *lang_f_options[j].variable = ! lang_f_options[j].on_value;
		  found = 1;
		}
	    }

	  return found;
	}
    }
  else if (p[0] == '-' && p[1] == 'W')
    {
      int setting = 1;

      /* The -W options control the warning behavior of the compiler.  */
      p += 2;

      if (p[0] == 'n' && p[1] == 'o' && p[2] == '-')
	setting = 0, p += 3;

      if (!strcmp (p, "implicit"))
	warn_implicit = setting;
      else if (!strcmp (p, "long-long"))
	warn_long_long = setting;
      else if (!strcmp (p, "return-type"))
	warn_return_type = setting;
      else if (!strcmp (p, "ctor-dtor-privacy"))
	warn_ctor_dtor_privacy = setting;
      else if (!strcmp (p, "write-strings"))
	warn_write_strings = setting;
      else if (!strcmp (p, "cast-qual"))
	warn_cast_qual = setting;
      else if (!strcmp (p, "char-subscripts"))
	warn_char_subscripts = setting;
      else if (!strcmp (p, "pointer-arith"))
	warn_pointer_arith = setting;
      else if (!strcmp (p, "missing-prototypes"))
	warn_missing_prototypes = setting;
      else if (!strcmp (p, "strict-prototypes"))
	{
	  if (setting == 0)
	    warning ("-Wno-strict-prototypes is not supported in C++");
	}
      else if (!strcmp (p, "redundant-decls"))
	warn_redundant_decls = setting;
      else if (!strcmp (p, "missing-braces"))
	warn_missing_braces = setting;
      else if (!strcmp (p, "sign-compare"))
	warn_sign_compare = setting;
      else if (!strcmp (p, "float-equal"))
	warn_float_equal = setting;
      else if (!strcmp (p, "format"))
	set_Wformat (setting);
      else if (!strcmp (p, "format=2"))
	set_Wformat (2);
      else if (!strcmp (p, "format-y2k"))
	warn_format_y2k = setting;
      else if (!strcmp (p, "format-extra-args"))
	warn_format_extra_args = setting;
      else if (!strcmp (p, "format-nonliteral"))
	warn_format_nonliteral = setting;
      else if (!strcmp (p, "format-security"))
	warn_format_security = setting;
      else if (!strcmp (p, "missing-format-attribute"))
	warn_missing_format_attribute = setting;
      else if (!strcmp (p, "conversion"))
	warn_conversion = setting;
      else if (!strcmp (p, "parentheses"))
	warn_parentheses = setting;
      else if (!strcmp (p, "non-virtual-dtor"))
	warn_nonvdtor = setting;
      else if (!strcmp (p, "reorder"))
	warn_reorder = setting;
      else if (!strcmp (p, "synth"))
	warn_synth = setting;
      else if (!strcmp (p, "pmf-conversions"))
	warn_pmf2ptr = setting;
      else if (!strcmp (p, "effc++"))
	warn_ecpp = setting;
      else if (!strcmp (p, "sign-promo"))
	warn_sign_promo = setting;
      else if (!strcmp (p, "old-style-cast"))
	warn_old_style_cast = setting;
      else if (!strcmp (p, "overloaded-virtual"))
	warn_overloaded_virtual = setting;
      else if (!strcmp (p, "multichar"))
	warn_multichar = setting;
      else if (!strcmp (p, "unknown-pragmas"))
	/* Set to greater than 1, so that even unknown pragmas in
	   system headers will be warned about.  */  
	warn_unknown_pragmas = setting * 2;
      else if (!strcmp (p, "non-template-friend"))
	warn_nontemplate_friend = setting;
      else if (!strcmp (p, "deprecated"))
        warn_deprecated = setting;
      else if (!strcmp (p, "comment"))
	;			/* cpp handles this one.  */
      else if (!strcmp (p, "comments"))
	;			/* cpp handles this one.  */
      else if (!strcmp (p, "trigraphs"))
	;			/* cpp handles this one.  */
      else if (!strcmp (p, "import"))
	;			/* cpp handles this one.  */
      else if (!strcmp (p, "all"))
	{
	  warn_return_type = setting;
	  set_Wunused (setting);
	  warn_implicit = setting;
	  warn_switch = setting;
	  set_Wformat (setting);
	  warn_parentheses = setting;
	  warn_missing_braces = setting;
	  warn_sign_compare = setting;
	  warn_multichar = setting;
	  /* We save the value of warn_uninitialized, since if they put
	     -Wuninitialized on the command line, we need to generate a
	     warning about not using it without also specifying -O.  */
	  if (warn_uninitialized != 1)
	    warn_uninitialized = (setting ? 2 : 0);
	  /* Only warn about unknown pragmas that are not in system
	     headers.  */                                        
	  warn_unknown_pragmas = 1;       

	  /* C++-specific warnings.  */
	  warn_ctor_dtor_privacy = setting;
	  warn_nonvdtor = setting;
	  warn_reorder = setting;
	  warn_nontemplate_friend = setting;           
	}
      else return strings_processed;
    }
  else if (!strcmp (p, "-ansi"))
    flag_no_nonansi_builtin = 1, flag_ansi = 1,
    flag_noniso_default_format_attributes = 0, flag_no_gnu_keywords = 1;
#ifdef SPEW_DEBUG
  /* Undocumented, only ever used when you're invoking cc1plus by hand, since
     it's probably safe to assume no sane person would ever want to use this
     under normal circumstances.  */
  else if (!strcmp (p, "-spew-debug"))
    spew_debug = 1;
#endif
  else
    return strings_processed;

  return 1;
}

/* Incorporate `const' and `volatile' qualifiers for member functions.
   FUNCTION is a TYPE_DECL or a FUNCTION_DECL.
   QUALS is a list of qualifiers.  Returns any explicit
   top-level qualifiers of the method's this pointer, anything other than
   TYPE_UNQUALIFIED will be an extension.  */

int
grok_method_quals (ctype, function, quals)
     tree ctype, function, quals;
{
  tree fntype = TREE_TYPE (function);
  tree raises = TYPE_RAISES_EXCEPTIONS (fntype);
  int type_quals = TYPE_UNQUALIFIED;
  int dup_quals = TYPE_UNQUALIFIED;
  int this_quals = TYPE_UNQUALIFIED;

  do
    {
      int tq = cp_type_qual_from_rid (TREE_VALUE (quals));
      
      if ((type_quals | this_quals) & tq)
	dup_quals |= tq;
      else if (tq & TYPE_QUAL_RESTRICT)
        this_quals |= tq;
      else
	type_quals |= tq;
      quals = TREE_CHAIN (quals);
    } 
  while (quals);

  if (dup_quals != TYPE_UNQUALIFIED)
    error ("duplicate type qualifiers in %s declaration",
	      TREE_CODE (function) == FUNCTION_DECL 
	      ? "member function" : "type");

  ctype = cp_build_qualified_type (ctype, type_quals);
  fntype = build_cplus_method_type (ctype, TREE_TYPE (fntype),
				    (TREE_CODE (fntype) == METHOD_TYPE
				     ? TREE_CHAIN (TYPE_ARG_TYPES (fntype))
				     : TYPE_ARG_TYPES (fntype)));
  if (raises)
    fntype = build_exception_variant (fntype, raises);

  TREE_TYPE (function) = fntype;
  return this_quals;
}

/* Warn when -fexternal-templates is used and #pragma
   interface/implementation is not used all the times it should be,
   inform the user.  */

void
warn_if_unknown_interface (decl)
     tree decl;
{
  static int already_warned = 0;
  if (already_warned++)
    return;

  if (flag_alt_external_templates)
    {
      tree til = tinst_for_decl ();
      int sl = lineno;
      const char *sf = input_filename;

      if (til)
	{
	  lineno = TINST_LINE (til);
	  input_filename = TINST_FILE (til);
	}
      warning ("template `%#D' instantiated in file without #pragma interface",
		  decl);
      lineno = sl;
      input_filename = sf;
    }
  else
    cp_warning_at ("template `%#D' defined in file without #pragma interface",
		   decl);
}

/* A subroutine of the parser, to handle a component list.  */

void
grok_x_components (specs)
     tree specs;
{
  tree t;

  specs = strip_attrs (specs);

  check_tag_decl (specs);
  t = groktypename (build_tree_list (specs, NULL_TREE)); 

  /* The only case where we need to do anything additional here is an
     anonymous union field, e.g.: `struct S { union { int i; }; };'.  */
  if (t == NULL_TREE || !ANON_AGGR_TYPE_P (t))
    return;

  fixup_anonymous_aggr (t);
  finish_member_declaration (build_decl (FIELD_DECL, NULL_TREE, t)); 
}

/* Returns a PARM_DECL for a parameter of the indicated TYPE, with the
   indicated NAME.  */

tree
build_artificial_parm (name, type)
     tree name;
     tree type;
{
  tree parm;

  parm = build_decl (PARM_DECL, name, type);
  DECL_ARTIFICIAL (parm) = 1;
  /* All our artificial parms are implicitly `const'; they cannot be
     assigned to.  */
  TREE_READONLY (parm) = 1;
  DECL_ARG_TYPE (parm) = type;
  return parm;
}

/* Constructors for types with virtual baseclasses need an "in-charge" flag
   saying whether this constructor is responsible for initialization of
   virtual baseclasses or not.  All destructors also need this "in-charge"
   flag, which additionally determines whether or not the destructor should
   free the memory for the object.

   This function adds the "in-charge" flag to member function FN if
   appropriate.  It is called from grokclassfn and tsubst.
   FN must be either a constructor or destructor.

   The in-charge flag follows the 'this' parameter, and is followed by the
   VTT parm (if any), then the user-written parms.  */

void
maybe_retrofit_in_chrg (fn)
     tree fn;
{
  tree basetype, arg_types, parms, parm, fntype;

  /* If we've already add the in-charge parameter don't do it again.  */
  if (DECL_HAS_IN_CHARGE_PARM_P (fn))
    return;

  /* When processing templates we can't know, in general, whether or
     not we're going to have virtual baseclasses.  */
  if (uses_template_parms (fn))
    return;

  /* We don't need an in-charge parameter for constructors that don't
     have virtual bases.  */
  if (DECL_CONSTRUCTOR_P (fn)
      && !TYPE_USES_VIRTUAL_BASECLASSES (DECL_CONTEXT (fn)))
    return;

  arg_types = TYPE_ARG_TYPES (TREE_TYPE (fn));
  basetype = TREE_TYPE (TREE_VALUE (arg_types));
  arg_types = TREE_CHAIN (arg_types);

  parms = TREE_CHAIN (DECL_ARGUMENTS (fn));

  /* If this is a subobject constructor or destructor, our caller will
     pass us a pointer to our VTT.  */
  if (TYPE_USES_VIRTUAL_BASECLASSES (DECL_CONTEXT (fn)))
    {
      parm = build_artificial_parm (vtt_parm_identifier, vtt_parm_type);

      /* First add it to DECL_ARGUMENTS between 'this' and the real args...  */
      TREE_CHAIN (parm) = parms;
      parms = parm;

      /* ...and then to TYPE_ARG_TYPES.  */
      arg_types = hash_tree_chain (vtt_parm_type, arg_types);

      DECL_HAS_VTT_PARM_P (fn) = 1;
    }

  /* Then add the in-charge parm (before the VTT parm).  */
  parm = build_artificial_parm (in_charge_identifier, integer_type_node);
  TREE_CHAIN (parm) = parms;
  parms = parm;
  arg_types = hash_tree_chain (integer_type_node, arg_types);

  /* Insert our new parameter(s) into the list.  */
  TREE_CHAIN (DECL_ARGUMENTS (fn)) = parms;

  /* And rebuild the function type.  */
  fntype = build_cplus_method_type (basetype, TREE_TYPE (TREE_TYPE (fn)),
				    arg_types);
  if (TYPE_RAISES_EXCEPTIONS (TREE_TYPE (fn)))
    fntype = build_exception_variant (fntype,
				      TYPE_RAISES_EXCEPTIONS (TREE_TYPE (fn)));
  TREE_TYPE (fn) = fntype;

  /* Now we've got the in-charge parameter.  */
  DECL_HAS_IN_CHARGE_PARM_P (fn) = 1;
}

/* Classes overload their constituent function names automatically.
   When a function name is declared in a record structure,
   its name is changed to it overloaded name.  Since names for
   constructors and destructors can conflict, we place a leading
   '$' for destructors.

   CNAME is the name of the class we are grokking for.

   FUNCTION is a FUNCTION_DECL.  It was created by `grokdeclarator'.

   FLAGS contains bits saying what's special about today's
   arguments.  1 == DESTRUCTOR.  2 == OPERATOR.

   If FUNCTION is a destructor, then we must add the `auto-delete' field
   as a second parameter.  There is some hair associated with the fact
   that we must "declare" this variable in the manner consistent with the
   way the rest of the arguments were declared.

   QUALS are the qualifiers for the this pointer.  */

void
grokclassfn (ctype, function, flags, quals)
     tree ctype, function;
     enum overload_flags flags;
     tree quals;
{
  tree fn_name = DECL_NAME (function);
  int this_quals = TYPE_UNQUALIFIED;

  /* Even within an `extern "C"' block, members get C++ linkage.  See
     [dcl.link] for details.  */
  SET_DECL_LANGUAGE (function, lang_cplusplus);

  if (fn_name == NULL_TREE)
    {
      error ("name missing for member function");
      fn_name = get_identifier ("<anonymous>");
      DECL_NAME (function) = fn_name;
    }

  if (quals)
    this_quals = grok_method_quals (ctype, function, quals);

  if (TREE_CODE (TREE_TYPE (function)) == METHOD_TYPE)
    {
      /* Must add the class instance variable up front.  */
      /* Right now we just make this a pointer.  But later
	 we may wish to make it special.  */
      tree type = TREE_VALUE (TYPE_ARG_TYPES (TREE_TYPE (function)));
      tree qual_type;
      tree parm;

      /* The `this' parameter is implicitly `const'; it cannot be
	 assigned to.  */
      this_quals |= TYPE_QUAL_CONST;
      qual_type = cp_build_qualified_type (type, this_quals);
      parm = build_artificial_parm (this_identifier, qual_type);
      c_apply_type_quals_to_decl (this_quals, parm);
      TREE_CHAIN (parm) = last_function_parms;
      last_function_parms = parm;
    }

  DECL_ARGUMENTS (function) = last_function_parms;
  DECL_CONTEXT (function) = ctype;

  if (flags == DTOR_FLAG)
    DECL_DESTRUCTOR_P (function) = 1;

  if (flags == DTOR_FLAG || DECL_CONSTRUCTOR_P (function))
    maybe_retrofit_in_chrg (function);

  if (flags == DTOR_FLAG)
    {
      DECL_DESTRUCTOR_P (function) = 1;
      TYPE_HAS_DESTRUCTOR (ctype) = 1;
    }
}

/* Create an ARRAY_REF, checking for the user doing things backwards
   along the way.  */

tree
grok_array_decl (array_expr, index_exp)
     tree array_expr, index_exp;
{
  tree type = TREE_TYPE (array_expr);
  tree p1, p2, i1, i2;

  if (type == error_mark_node || index_exp == error_mark_node)
    return error_mark_node;
  if (processing_template_decl)
    return build_min (ARRAY_REF, type ? TREE_TYPE (type) : NULL_TREE,
		      array_expr, index_exp);

  if (type == NULL_TREE)
    {
      /* Something has gone very wrong.  Assume we are mistakenly reducing
	 an expression instead of a declaration.  */
      error ("parser may be lost: is there a '{' missing somewhere?");
      return NULL_TREE;
    }

  if (TREE_CODE (type) == OFFSET_TYPE
      || TREE_CODE (type) == REFERENCE_TYPE)
    type = TREE_TYPE (type);

  /* If they have an `operator[]', use that.  */
  if (IS_AGGR_TYPE (type) || IS_AGGR_TYPE (TREE_TYPE (index_exp)))
    return build_opfncall (ARRAY_REF, LOOKUP_NORMAL,
			   array_expr, index_exp, NULL_TREE);

  /* Otherwise, create an ARRAY_REF for a pointer or array type.  It
     is a little-known fact that, if `a' is an array and `i' is an
     int, you can write `i[a]', which means the same thing as `a[i]'.  */

  if (TREE_CODE (type) == ARRAY_TYPE)
    p1 = array_expr;
  else
    p1 = build_expr_type_conversion (WANT_POINTER, array_expr, 0);

  if (TREE_CODE (TREE_TYPE (index_exp)) == ARRAY_TYPE)
    p2 = index_exp;
  else
    p2 = build_expr_type_conversion (WANT_POINTER, index_exp, 0);

  i1 = build_expr_type_conversion (WANT_INT | WANT_ENUM, array_expr, 0);
  i2 = build_expr_type_conversion (WANT_INT | WANT_ENUM, index_exp, 0);

  if ((p1 && i2) && (i1 && p2))
    error ("ambiguous conversion for array subscript");

  if (p1 && i2)
    array_expr = p1, index_exp = i2;
  else if (i1 && p2)
    array_expr = p2, index_exp = i1;
  else
    {
      error ("invalid types `%T[%T]' for array subscript",
		type, TREE_TYPE (index_exp));
      return error_mark_node;
    }

  if (array_expr == error_mark_node || index_exp == error_mark_node)
    error ("ambiguous conversion for array subscript");

  return build_array_ref (array_expr, index_exp);
}

/* Given the cast expression EXP, checking out its validity.   Either return
   an error_mark_node if there was an unavoidable error, return a cast to
   void for trying to delete a pointer w/ the value 0, or return the
   call to delete.  If DOING_VEC is 1, we handle things differently
   for doing an array delete.  If DOING_VEC is 2, they gave us the
   array size as an argument to delete.
   Implements ARM $5.3.4.  This is called from the parser.  */

tree
delete_sanity (exp, size, doing_vec, use_global_delete)
     tree exp, size;
     int doing_vec, use_global_delete;
{
  tree t, type;
  /* For a regular vector delete (aka, no size argument) we will pass
     this down as a NULL_TREE into build_vec_delete.  */
  tree maxindex = NULL_TREE;

  if (exp == error_mark_node)
    return exp;

  if (processing_template_decl)
    {
      t = build_min (DELETE_EXPR, void_type_node, exp, size);
      DELETE_EXPR_USE_GLOBAL (t) = use_global_delete;
      DELETE_EXPR_USE_VEC (t) = doing_vec;
      return t;
    }

  if (TREE_CODE (exp) == OFFSET_REF)
    exp = resolve_offset_ref (exp);
  exp = convert_from_reference (exp);
  t = stabilize_reference (exp);
  t = build_expr_type_conversion (WANT_POINTER, t, 1);

  if (t == NULL_TREE || t == error_mark_node)
    {
      error ("type `%#T' argument given to `delete', expected pointer",
		TREE_TYPE (exp));
      return error_mark_node;
    }

  if (doing_vec == 2)
    {
      maxindex = cp_build_binary_op (MINUS_EXPR, size, integer_one_node);
      pedwarn ("anachronistic use of array size in vector delete");
    }

  type = TREE_TYPE (t);

  /* As of Valley Forge, you can delete a pointer to const.  */

  /* You can't delete functions.  */
  if (TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE)
    {
      error ("cannot delete a function.  Only pointer-to-objects are valid arguments to `delete'");
      return error_mark_node;
    }

  /* Deleting ptr to void is undefined behaviour [expr.delete/3].  */
  if (TREE_CODE (TREE_TYPE (type)) == VOID_TYPE)
    {
      warning ("deleting `%T' is undefined", type);
      doing_vec = 0;
    }

  /* An array can't have been allocated by new, so complain.  */
  if (TREE_CODE (t) == ADDR_EXPR
      && TREE_CODE (TREE_OPERAND (t, 0)) == VAR_DECL
      && TREE_CODE (TREE_TYPE (TREE_OPERAND (t, 0))) == ARRAY_TYPE)
    warning ("deleting array `%#D'", TREE_OPERAND (t, 0));

  /* Deleting a pointer with the value zero is valid and has no effect.  */
  if (integer_zerop (t))
    return build1 (NOP_EXPR, void_type_node, t);

  if (doing_vec)
    return build_vec_delete (t, maxindex, sfk_deleting_destructor,
			     use_global_delete);
  else
    return build_delete (type, t, sfk_deleting_destructor,
			 LOOKUP_NORMAL, use_global_delete);
}

/* Report an error if the indicated template declaration is not the
   sort of thing that should be a member template.  */

void
check_member_template (tmpl)
     tree tmpl;
{
  tree decl;

  my_friendly_assert (TREE_CODE (tmpl) == TEMPLATE_DECL, 0);
  decl = DECL_TEMPLATE_RESULT (tmpl);

  if (TREE_CODE (decl) == FUNCTION_DECL
      || (TREE_CODE (decl) == TYPE_DECL
	  && IS_AGGR_TYPE (TREE_TYPE (decl))))
    {
      if (current_function_decl)
	/* 14.5.2.2 [temp.mem]
	   
	   A local class shall not have member templates. */
	error ("invalid declaration of member template `%#D' in local class",
		  decl);
      
      if (TREE_CODE (decl) == FUNCTION_DECL && DECL_VIRTUAL_P (decl))
	{
	  /* 14.5.2.3 [temp.mem]

	     A member function template shall not be virtual.  */
	  error 
	    ("invalid use of `virtual' in template declaration of `%#D'",
	     decl);
	  DECL_VIRTUAL_P (decl) = 0;
	}

      /* The debug-information generating code doesn't know what to do
	 with member templates.  */ 
      DECL_IGNORED_P (tmpl) = 1;
    } 
  else
    error ("template declaration of `%#D'", decl);
}

/* Return true iff TYPE is a valid Java parameter or return type. */

static int
acceptable_java_type (type)
     tree type;
{
  if (TREE_CODE (type) == VOID_TYPE || TYPE_FOR_JAVA (type))
    return 1;
  if (TREE_CODE (type) == POINTER_TYPE || TREE_CODE (type) == REFERENCE_TYPE)
    {
      type = TREE_TYPE (type);
      if (TREE_CODE (type) == RECORD_TYPE)
	{
	  tree args;  int i;
	  if (! TYPE_FOR_JAVA (type))
	    return 0;
	  if (! CLASSTYPE_TEMPLATE_INFO (type))
	    return 1;
	  args = CLASSTYPE_TI_ARGS (type);
	  i = TREE_VEC_LENGTH (args);
	  while (--i >= 0)
	    {
	      type = TREE_VEC_ELT (args, i);
	      if (TREE_CODE (type) == POINTER_TYPE)
		type = TREE_TYPE (type);
	      if (! TYPE_FOR_JAVA (type))
		return 0;
	    }
	  return 1;
	}
    }
  return 0;
}

/* For a METHOD in a Java class CTYPE, return 1 if
   the parameter and return types are valid Java types.
   Otherwise, print appropriate error messages, and return 0.  */

int
check_java_method (method)
     tree method;
{
  int jerr = 0;
  tree arg_types = TYPE_ARG_TYPES (TREE_TYPE (method));
  tree ret_type = TREE_TYPE (TREE_TYPE (method));
  if (! acceptable_java_type (ret_type))
    {
      error ("Java method '%D' has non-Java return type `%T'",
		method, ret_type);
      jerr++;
    }
  for (; arg_types != NULL_TREE; arg_types = TREE_CHAIN (arg_types))
    {
      tree type = TREE_VALUE (arg_types);
      if (! acceptable_java_type (type))
	{
	  error ("Java method '%D' has non-Java parameter type `%T'",
		    method, type);
	  jerr++;
	}
    }
  return jerr ? 0 : 1;
}

/* Sanity check: report error if this function FUNCTION is not
   really a member of the class (CTYPE) it is supposed to belong to.
   CNAME is the same here as it is for grokclassfn above.  */

tree
check_classfn (ctype, function)
     tree ctype, function;
{
  tree fn_name = DECL_NAME (function);
  tree fndecl, fndecls;
  tree method_vec = CLASSTYPE_METHOD_VEC (complete_type (ctype));
  tree *methods = 0;
  tree *end = 0;
  
  if (DECL_USE_TEMPLATE (function)
      && !(TREE_CODE (function) == TEMPLATE_DECL
	   && DECL_TEMPLATE_SPECIALIZATION (function))
      && is_member_template (DECL_TI_TEMPLATE (function)))
    /* Since this is a specialization of a member template,
       we're not going to find the declaration in the class.
       For example, in:
       
         struct S { template <typename T> void f(T); };
         template <> void S::f(int);
       
       we're not going to find `S::f(int)', but there's no
       reason we should, either.  We let our callers know we didn't
       find the method, but we don't complain.  */
    return NULL_TREE;
      
  if (method_vec != 0)
    {
      methods = &TREE_VEC_ELT (method_vec, 0);
      end = TREE_VEC_END (method_vec);

      /* First suss out ctors and dtors.  */
      if (*methods && fn_name == DECL_NAME (OVL_CURRENT (*methods))
	  && DECL_CONSTRUCTOR_P (function))
	goto got_it;
      if (*++methods && fn_name == DECL_NAME (OVL_CURRENT (*methods))
	  && DECL_DESTRUCTOR_P (function))
	goto got_it;

      while (++methods != end && *methods)
	{
	  fndecl = *methods;
	  if (fn_name == DECL_NAME (OVL_CURRENT (*methods)))
	    {
	    got_it:
	      for (fndecls = *methods; fndecls != NULL_TREE;
		   fndecls = OVL_NEXT (fndecls))
		{
		  fndecl = OVL_CURRENT (fndecls);

		  /* We cannot simply call decls_match because this
		     doesn't work for static member functions that are 
                     pretending to be methods, and because the name
		     may have been changed by asm("new_name").  */ 
		  if (DECL_NAME (function) == DECL_NAME (fndecl))
		    {
		      tree p1 = TYPE_ARG_TYPES (TREE_TYPE (function));
		      tree p2 = TYPE_ARG_TYPES (TREE_TYPE (fndecl));

		      /* Get rid of the this parameter on functions that become
			 static.  */
		      if (DECL_STATIC_FUNCTION_P (fndecl)
			  && TREE_CODE (TREE_TYPE (function)) == METHOD_TYPE)
			p1 = TREE_CHAIN (p1);

		      if (same_type_p (TREE_TYPE (TREE_TYPE (function)),
				       TREE_TYPE (TREE_TYPE (fndecl)))
			  && compparms (p1, p2)
			  && (DECL_TEMPLATE_SPECIALIZATION (function)
			      == DECL_TEMPLATE_SPECIALIZATION (fndecl))
			  && (!DECL_TEMPLATE_SPECIALIZATION (function)
			      || (DECL_TI_TEMPLATE (function) 
				  == DECL_TI_TEMPLATE (fndecl))))
			return fndecl;
		    }
		}
	      break;		/* loser */
	    }
	}
    }

  if (methods != end && *methods)
    {
      tree fndecl = *methods;
      error ("prototype for `%#D' does not match any in class `%T'",
		function, ctype);
      cp_error_at ("candidate%s: %+#D", OVL_NEXT (fndecl) ? "s are" : " is",
		   OVL_CURRENT (fndecl));
      while (fndecl = OVL_NEXT (fndecl), fndecl)
	cp_error_at ("                %#D", OVL_CURRENT(fndecl));
    }
  else
    {
      methods = 0;
      if (!COMPLETE_TYPE_P (ctype))
        incomplete_type_error (function, ctype);
      else
        error ("no `%#D' member function declared in class `%T'",
		  function, ctype);
    }

  /* If we did not find the method in the class, add it to avoid
     spurious errors (unless the CTYPE is not yet defined, in which
     case we'll only confuse ourselves when the function is declared
     properly within the class.  */
  if (COMPLETE_TYPE_P (ctype))
    add_method (ctype, function, /*error_p=*/1);
  return NULL_TREE;
}

/* We have just processed the DECL, which is a static data member.
   Its initializer, if present, is INIT.  The ASMSPEC_TREE, if
   present, is the assembly-language name for the data member.
   FLAGS is as for cp_finish_decl.  */

void
finish_static_data_member_decl (decl, init, asmspec_tree, flags)
     tree decl;
     tree init;
     tree asmspec_tree;
     int flags;
{
  my_friendly_assert (TREE_PUBLIC (decl), 0);

  DECL_CONTEXT (decl) = current_class_type;

  /* We cannot call pushdecl here, because that would fill in the
     TREE_CHAIN of our decl.  Instead, we modify cp_finish_decl to do
     the right thing, namely, to put this decl out straight away.  */
  /* current_class_type can be NULL_TREE in case of error.  */
  if (!asmspec_tree && current_class_type)
    DECL_INITIAL (decl) = error_mark_node;

  if (! processing_template_decl)
    {
      if (!pending_statics)
	VARRAY_TREE_INIT (pending_statics, 32, "pending_statics");
      VARRAY_PUSH_TREE (pending_statics, decl);
    }

  if (LOCAL_CLASS_P (current_class_type))
    pedwarn ("local class `%#T' shall not have static data member `%#D'",
	     current_class_type, decl);

  /* Static consts need not be initialized in the class definition.  */
  if (init != NULL_TREE && TYPE_NEEDS_CONSTRUCTING (TREE_TYPE (decl)))
    {
      static int explained = 0;
	  
      error ("initializer invalid for static member with constructor");
      if (!explained)
        {
	  error ("(an out of class initialization is required)");
	  explained = 1;
	}
      init = NULL_TREE;
    }
  /* Force the compiler to know when an uninitialized static const
     member is being used.  */
  if (CP_TYPE_CONST_P (TREE_TYPE (decl)) && init == 0)
    TREE_USED (decl) = 1;
  DECL_INITIAL (decl) = init;
  DECL_IN_AGGR_P (decl) = 1;

  cp_finish_decl (decl, init, asmspec_tree, flags);
}

/* Process the specs, declarator (NULL if omitted) and width (NULL if omitted)
   of a structure component, returning a _DECL node.
   QUALS is a list of type qualifiers for this decl (such as for declaring
   const member functions).

   This is done during the parsing of the struct declaration.
   The _DECL nodes are chained together and the lot of them
   are ultimately passed to `build_struct' to make the RECORD_TYPE node.

   If class A defines that certain functions in class B are friends, then
   the way I have set things up, it is B who is interested in permission
   granted by A.  However, it is in A's context that these declarations
   are parsed.  By returning a void_type_node, class A does not attempt
   to incorporate the declarations of the friends within its structure.

   DO NOT MAKE ANY CHANGES TO THIS CODE WITHOUT MAKING CORRESPONDING
   CHANGES TO CODE IN `start_method'.  */

tree
grokfield (declarator, declspecs, init, asmspec_tree, attrlist)
     tree declarator, declspecs, init, asmspec_tree, attrlist;
{
  tree value;
  const char *asmspec = 0;
  int flags = LOOKUP_ONLYCONVERTING;

  /* Convert () initializers to = initializers.  */
  if (init == NULL_TREE && declarator != NULL_TREE
      && TREE_CODE (declarator) == CALL_EXPR
      && TREE_OPERAND (declarator, 0)
      && (TREE_CODE (TREE_OPERAND (declarator, 0)) == IDENTIFIER_NODE
	  || TREE_CODE (TREE_OPERAND (declarator, 0)) == SCOPE_REF)
      && parmlist_is_exprlist (CALL_DECLARATOR_PARMS (declarator)))
    {
      /* It's invalid to try to initialize a data member using a
	 functional notation, e.g.:
	 
            struct S {
	      static int i (3);
	    };
	    
	 Explain that to the user.  */
      static int explained;

      error ("invalid data member initialization");
      if (!explained)
	{
	  error ("(use `=' to initialize static data members)");
	  explained = 1;
	}

      declarator = TREE_OPERAND (declarator, 0);
      flags = 0;
    }

  if (declspecs == NULL_TREE
      && TREE_CODE (declarator) == SCOPE_REF
      && TREE_CODE (TREE_OPERAND (declarator, 1)) == IDENTIFIER_NODE)
    {
      /* Access declaration */
      if (! IS_AGGR_TYPE_CODE (TREE_CODE (TREE_OPERAND (declarator, 0))))
	;
      else if (TREE_COMPLEXITY (declarator) == current_class_depth)
	pop_nested_class ();
      return do_class_using_decl (declarator);
    }

  if (init
      && TREE_CODE (init) == TREE_LIST
      && TREE_VALUE (init) == error_mark_node
      && TREE_CHAIN (init) == NULL_TREE)
    init = NULL_TREE;

  value = grokdeclarator (declarator, declspecs, FIELD, init != 0, &attrlist);
  if (! value || value == error_mark_node)
    /* friend or constructor went bad.  */
    return value;
  if (TREE_TYPE (value) == error_mark_node)
    return error_mark_node;  

  /* Pass friendly classes back.  */
  if (TREE_CODE (value) == VOID_TYPE)
    return void_type_node;

  if (DECL_NAME (value) != NULL_TREE
      && IDENTIFIER_POINTER (DECL_NAME (value))[0] == '_'
      && ! strcmp (IDENTIFIER_POINTER (DECL_NAME (value)), "_vptr"))
    error ("member `%D' conflicts with virtual function table field name",
	      value);

  /* Stash away type declarations.  */
  if (TREE_CODE (value) == TYPE_DECL)
    {
      DECL_NONLOCAL (value) = 1;
      DECL_CONTEXT (value) = current_class_type;

      if (CLASS_TYPE_P (TREE_TYPE (value)))
        CLASSTYPE_GOT_SEMICOLON (TREE_TYPE (value)) = 1;
      
      if (processing_template_decl)
	value = push_template_decl (value);

      return value;
    }

  if (DECL_IN_AGGR_P (value))
    {
      error ("`%D' is already defined in `%T'", value,
		DECL_CONTEXT (value));
      return void_type_node;
    }

  if (asmspec_tree)
    asmspec = TREE_STRING_POINTER (asmspec_tree);

  if (init)
    {
      if (TREE_CODE (value) == FUNCTION_DECL)
	{
	  grok_function_init (value, init);
	  init = NULL_TREE;
	}
      else if (pedantic && TREE_CODE (value) != VAR_DECL)
	/* Already complained in grokdeclarator.  */
	init = NULL_TREE;
      else
	{
	  /* We allow initializers to become parameters to base
             initializers.  */
	  if (TREE_CODE (init) == TREE_LIST)
	    {
	      if (TREE_CHAIN (init) == NULL_TREE)
		init = TREE_VALUE (init);
	      else
		init = digest_init (TREE_TYPE (value), init, (tree *)0);
	    }
	  
	  if (TREE_CODE (init) == CONST_DECL)
	    init = DECL_INITIAL (init);
	  else if (TREE_READONLY_DECL_P (init))
	    init = decl_constant_value (init);
	  else if (TREE_CODE (init) == CONSTRUCTOR)
	    init = digest_init (TREE_TYPE (value), init, (tree *)0);
	  if (init == error_mark_node)
	    /* We must make this look different than `error_mark_node'
	       because `decl_const_value' would mis-interpret it
	       as only meaning that this VAR_DECL is defined.  */
	    init = build1 (NOP_EXPR, TREE_TYPE (value), init);
	  else if (processing_template_decl)
	    ;
	  else if (! TREE_CONSTANT (init))
	    {
	      /* We can allow references to things that are effectively
		 static, since references are initialized with the address.  */
	      if (TREE_CODE (TREE_TYPE (value)) != REFERENCE_TYPE
		  || (TREE_STATIC (init) == 0
		      && (!DECL_P (init) || DECL_EXTERNAL (init) == 0)))
		{
		  error ("field initializer is not constant");
		  init = error_mark_node;
		}
	    }
	}
    }

  if (processing_template_decl && ! current_function_decl
      && (TREE_CODE (value) == VAR_DECL || TREE_CODE (value) == FUNCTION_DECL))
    value = push_template_decl (value);

  if (attrlist)
    cplus_decl_attributes (&value, attrlist, 0);

  if (TREE_CODE (value) == VAR_DECL)
    {
      finish_static_data_member_decl (value, init, asmspec_tree, 
				      flags);
      return value;
    }
  if (TREE_CODE (value) == FIELD_DECL)
    {
      if (asmspec)
	error ("`asm' specifiers are not permitted on non-static data members");
      if (DECL_INITIAL (value) == error_mark_node)
	init = error_mark_node;
      cp_finish_decl (value, init, NULL_TREE, flags);
      DECL_INITIAL (value) = init;
      DECL_IN_AGGR_P (value) = 1;
      return value;
    }
  if (TREE_CODE (value) == FUNCTION_DECL)
    {
      if (asmspec)
	{
	  /* This must override the asm specifier which was placed
	     by grokclassfn.  Lay this out fresh.  */
	  SET_DECL_RTL (value, NULL_RTX);
	  SET_DECL_ASSEMBLER_NAME (value, get_identifier (asmspec));
	}
      if (!DECL_FRIEND_P (value))
	grok_special_member_properties (value);
      
      cp_finish_decl (value, init, asmspec_tree, flags);

      /* Pass friends back this way.  */
      if (DECL_FRIEND_P (value))
	return void_type_node;

      DECL_IN_AGGR_P (value) = 1;
      return value;
    }
  abort ();
  /* NOTREACHED */
  return NULL_TREE;
}

/* Like `grokfield', but for bitfields.
   WIDTH is non-NULL for bit fields only, and is an INTEGER_CST node.  */

tree
grokbitfield (declarator, declspecs, width)
     tree declarator, declspecs, width;
{
  register tree value = grokdeclarator (declarator, declspecs, BITFIELD,
					0, NULL);

  if (! value) return NULL_TREE; /* friends went bad.  */

  /* Pass friendly classes back.  */
  if (TREE_CODE (value) == VOID_TYPE)
    return void_type_node;

  if (TREE_CODE (value) == TYPE_DECL)
    {
      error ("cannot declare `%D' to be a bit-field type", value);
      return NULL_TREE;
    }

  /* Usually, finish_struct_1 catches bitfields with invalid types.
     But, in the case of bitfields with function type, we confuse
     ourselves into thinking they are member functions, so we must
     check here.  */
  if (TREE_CODE (value) == FUNCTION_DECL)
    {
      error ("cannot declare bit-field `%D' with function type",
	     DECL_NAME (value));
      return NULL_TREE;
    }

  if (DECL_IN_AGGR_P (value))
    {
      error ("`%D' is already defined in the class %T", value,
		  DECL_CONTEXT (value));
      return void_type_node;
    }

  if (TREE_STATIC (value))
    {
      error ("static member `%D' cannot be a bit-field", value);
      return NULL_TREE;
    }
  cp_finish_decl (value, NULL_TREE, NULL_TREE, 0);

  if (width != error_mark_node)
    {
      constant_expression_warning (width);
      DECL_INITIAL (value) = width;
      SET_DECL_C_BIT_FIELD (value);
    }

  DECL_IN_AGGR_P (value) = 1;
  return value;
}

tree
grokoptypename (declspecs, declarator)
     tree declspecs, declarator;
{
  tree t = grokdeclarator (declarator, declspecs, TYPENAME, 0, NULL);
  return mangle_conv_op_name_for_type (t);
}

/* When a function is declared with an initializer,
   do the right thing.  Currently, there are two possibilities:

   class B
   {
    public:
     // initialization possibility #1.
     virtual void f () = 0;
     int g ();
   };
   
   class D1 : B
   {
    public:
     int d1;
     // error, no f ();
   };
   
   class D2 : B
   {
    public:
     int d2;
     void f ();
   };
   
   class D3 : B
   {
    public:
     int d3;
     // initialization possibility #2
     void f () = B::f;
   };

*/

static void
grok_function_init (decl, init)
     tree decl;
     tree init;
{
  /* An initializer for a function tells how this function should
     be inherited.  */
  tree type = TREE_TYPE (decl);

  if (TREE_CODE (type) == FUNCTION_TYPE)
    error ("initializer specified for non-member function `%D'", decl);
  else if (integer_zerop (init))
    DECL_PURE_VIRTUAL_P (decl) = 1;
  else
    error ("invalid initializer for virtual method `%D'", decl);
}

void
cplus_decl_attributes (decl, attributes, flags)
     tree *decl, attributes;
     int flags;
{
  if (*decl == NULL_TREE || *decl == void_type_node)
    return;

  if (TREE_CODE (*decl) == TEMPLATE_DECL)
    decl = &DECL_TEMPLATE_RESULT (*decl);

  decl_attributes (decl, attributes, flags);

  if (TREE_CODE (*decl) == TYPE_DECL)
    SET_IDENTIFIER_TYPE_VALUE (DECL_NAME (*decl), TREE_TYPE (*decl));
}

/* CONSTRUCTOR_NAME:
   Return the name for the constructor (or destructor) for the
   specified class.  Argument can be RECORD_TYPE, TYPE_DECL, or
   IDENTIFIER_NODE.  When given a template, this routine doesn't
   lose the specialization.  */

tree
constructor_name_full (thing)
     tree thing;
{
  if (TREE_CODE (thing) == TEMPLATE_TYPE_PARM
      || TREE_CODE (thing) == BOUND_TEMPLATE_TEMPLATE_PARM
      || TREE_CODE (thing) == TYPENAME_TYPE)
    thing = TYPE_NAME (thing);
  else if (IS_AGGR_TYPE_CODE (TREE_CODE (thing)))
    {
      if (TYPE_WAS_ANONYMOUS (thing) && TYPE_HAS_CONSTRUCTOR (thing))
	thing = DECL_NAME (OVL_CURRENT (TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (thing), 0)));
      else
	thing = TYPE_NAME (thing);
    }
  if (TREE_CODE (thing) == TYPE_DECL
      || (TREE_CODE (thing) == TEMPLATE_DECL
	  && TREE_CODE (DECL_TEMPLATE_RESULT (thing)) == TYPE_DECL))
    thing = DECL_NAME (thing);
  my_friendly_assert (TREE_CODE (thing) == IDENTIFIER_NODE, 197);
  return thing;
}

/* CONSTRUCTOR_NAME:
   Return the name for the constructor (or destructor) for the
   specified class.  Argument can be RECORD_TYPE, TYPE_DECL, or
   IDENTIFIER_NODE.  When given a template, return the plain
   unspecialized name.  */

tree
constructor_name (thing)
     tree thing;
{
  tree t;
  thing = constructor_name_full (thing);
  t = IDENTIFIER_TEMPLATE (thing);
  if (!t)
    return thing;
  return t;
}

/* Defer the compilation of the FN until the end of compilation.  */

void
defer_fn (fn)
     tree fn;
{
  if (DECL_DEFERRED_FN (fn))
    return;
  DECL_DEFERRED_FN (fn) = 1;
  if (!deferred_fns)
    VARRAY_TREE_INIT (deferred_fns, 32, "deferred_fns");

  VARRAY_PUSH_TREE (deferred_fns, fn);
}

/* Hunts through the global anonymous union ANON_DECL, building
   appropriate VAR_DECLs.  Stores cleanups on the list of ELEMS, and
   returns a VAR_DECL whose size is the same as the size of the
   ANON_DECL, if one is available.

   FIXME: we should really handle anonymous unions by binding the names
   of the members to COMPONENT_REFs rather than this kludge.  */

static tree 
build_anon_union_vars (anon_decl, elems, static_p, external_p)
     tree anon_decl;
     tree* elems;
     int static_p;
     int external_p;
{
  tree type = TREE_TYPE (anon_decl);
  tree main_decl = NULL_TREE;
  tree field;

  /* Rather than write the code to handle the non-union case,
     just give an error.  */
  if (TREE_CODE (type) != UNION_TYPE)
    error ("anonymous struct not inside named type");

  for (field = TYPE_FIELDS (type); 
       field != NULL_TREE; 
       field = TREE_CHAIN (field))
    {
      tree decl;

      if (DECL_ARTIFICIAL (field))
	continue;
      if (TREE_CODE (field) != FIELD_DECL)
	{
	  cp_pedwarn_at ("`%#D' invalid; an anonymous union can only have non-static data members",
			 field);
	  continue;
	}

      if (TREE_PRIVATE (field))
	cp_pedwarn_at ("private member `%#D' in anonymous union", field);
      else if (TREE_PROTECTED (field))
	cp_pedwarn_at ("protected member `%#D' in anonymous union", field);

      if (DECL_NAME (field) == NULL_TREE
	  && ANON_AGGR_TYPE_P (TREE_TYPE (field)))
	{
	  decl = build_anon_union_vars (field, elems, static_p, external_p);
	  if (!decl)
	    continue;
	}
      else if (DECL_NAME (field) == NULL_TREE)
	continue;
      else
	{
	  decl = build_decl (VAR_DECL, DECL_NAME (field), TREE_TYPE (field));
	  /* tell `pushdecl' that this is not tentative.  */
	  DECL_INITIAL (decl) = error_mark_node;
	  TREE_PUBLIC (decl) = 0;
	  TREE_STATIC (decl) = static_p;
	  DECL_EXTERNAL (decl) = external_p;
	  decl = pushdecl (decl);
	  DECL_INITIAL (decl) = NULL_TREE;
	}

      /* Only write out one anon union element--choose the largest
	 one.  We used to try to find one the same size as the union,
	 but that fails if the ABI forces us to align the union more
	 strictly.  */
      if (main_decl == NULL_TREE
	  || tree_int_cst_lt (DECL_SIZE (main_decl), DECL_SIZE (decl)))
	{
	  if (main_decl)
	    TREE_ASM_WRITTEN (main_decl) = 1;
	  main_decl = decl;
	}
      else 
	/* ??? This causes there to be no debug info written out
	   about this decl.  */
	TREE_ASM_WRITTEN (decl) = 1;
      
      if (DECL_NAME (field) == NULL_TREE
	  && ANON_AGGR_TYPE_P (TREE_TYPE (field)))
	/* The remainder of the processing was already done in the
	   recursive call.  */
	continue;

      /* If there's a cleanup to do, it belongs in the
	 TREE_PURPOSE of the following TREE_LIST.  */
      *elems = tree_cons (NULL_TREE, decl, *elems);
      TREE_TYPE (*elems) = type;
    }
  
  return main_decl;
}

/* Finish off the processing of a UNION_TYPE structure.  If the union is an
   anonymous union, then all members must be laid out together.  PUBLIC_P
   is nonzero if this union is not declared static.  */

void
finish_anon_union (anon_union_decl)
     tree anon_union_decl;
{
  tree type = TREE_TYPE (anon_union_decl);
  tree main_decl;
  int public_p = TREE_PUBLIC (anon_union_decl);
  int static_p = TREE_STATIC (anon_union_decl);
  int external_p = DECL_EXTERNAL (anon_union_decl);

  /* The VAR_DECL's context is the same as the TYPE's context. */
  DECL_CONTEXT (anon_union_decl) = DECL_CONTEXT (TYPE_NAME (type));
  
  if (TYPE_FIELDS (type) == NULL_TREE)
    return;

  if (public_p)
    {
      error ("namespace-scope anonymous aggregates must be static");
      return;
    }

  main_decl = build_anon_union_vars (anon_union_decl,
				     &DECL_ANON_UNION_ELEMS (anon_union_decl),
				     static_p, external_p);

  if (main_decl == NULL_TREE)
    {
      warning ("anonymous aggregate with no members");
      return;
    }

  if (static_p)
    {
      make_decl_rtl (main_decl, 0);
      COPY_DECL_RTL (main_decl, anon_union_decl);
      expand_anon_union_decl (anon_union_decl, 
			      NULL_TREE,
			      DECL_ANON_UNION_ELEMS (anon_union_decl));
    }
  else
    add_decl_stmt (anon_union_decl);
}

/* Finish processing a builtin type TYPE.  It's name is NAME,
   its fields are in the array FIELDS.  LEN is the number of elements
   in FIELDS minus one, or put another way, it is the maximum subscript
   used in FIELDS.

   It is given the same alignment as ALIGN_TYPE.  */

void
finish_builtin_type (type, name, fields, len, align_type)
     tree type;
     const char *name;
     tree fields[];
     int len;
     tree align_type;
{
  register int i;

  TYPE_FIELDS (type) = fields[0];
  for (i = 0; i < len; i++)
    {
      layout_type (TREE_TYPE (fields[i]));
      DECL_FIELD_CONTEXT (fields[i]) = type;
      TREE_CHAIN (fields[i]) = fields[i+1];
    }
  DECL_FIELD_CONTEXT (fields[i]) = type;
  TYPE_ALIGN (type) = TYPE_ALIGN (align_type);
  TYPE_USER_ALIGN (type) = TYPE_USER_ALIGN (align_type);
  layout_type (type);
#if 0 /* not yet, should get fixed properly later */
  TYPE_NAME (type) = make_type_decl (get_identifier (name), type);
#else
  TYPE_NAME (type) = build_decl (TYPE_DECL, get_identifier (name), type);
#endif
  TYPE_STUB_DECL (type) = TYPE_NAME (type);
  layout_decl (TYPE_NAME (type), 0);
}

/* Auxiliary functions to make type signatures for
   `operator new' and `operator delete' correspond to
   what compiler will be expecting.  */

tree
coerce_new_type (type)
     tree type;
{
  int e = 0;
  tree args = TYPE_ARG_TYPES (type);

  my_friendly_assert (TREE_CODE (type) == FUNCTION_TYPE, 20001107);
  
  if (!same_type_p (TREE_TYPE (type), ptr_type_node))
    e = 1, error ("`operator new' must return type `%T'", ptr_type_node);

  if (!args || args == void_list_node
      || !same_type_p (TREE_VALUE (args), c_size_type_node))
    {
      e = 2;
      if (args && args != void_list_node)
        args = TREE_CHAIN (args);
      pedwarn ("`operator new' takes type `size_t' (`%T') as first parameter", c_size_type_node);
    }
  switch (e)
  {
    case 2:
      args = tree_cons (NULL_TREE, c_size_type_node, args);
      /* FALLTHROUGH */
    case 1:
      type = build_exception_variant
              (build_function_type (ptr_type_node, args),
               TYPE_RAISES_EXCEPTIONS (type));
      /* FALLTHROUGH */
    default:;
  }
  return type;
}

tree
coerce_delete_type (type)
     tree type;
{
  int e = 0;
  tree args = TYPE_ARG_TYPES (type);
  
  my_friendly_assert (TREE_CODE (type) == FUNCTION_TYPE, 20001107);

  if (!same_type_p (TREE_TYPE (type), void_type_node))
    e = 1, error ("`operator delete' must return type `%T'", void_type_node);

  if (!args || args == void_list_node
      || !same_type_p (TREE_VALUE (args), ptr_type_node))
    {
      e = 2;
      if (args && args != void_list_node)
        args = TREE_CHAIN (args);
      error ("`operator delete' takes type `%T' as first parameter", ptr_type_node);
    }
  switch (e)
  {
    case 2:
      args = tree_cons (NULL_TREE, ptr_type_node, args);
      /* FALLTHROUGH */
    case 1:
      type = build_exception_variant
              (build_function_type (void_type_node, args),
               TYPE_RAISES_EXCEPTIONS (type));
      /* FALLTHROUGH */
    default:;
  }

  return type;
}

static void
mark_vtable_entries (decl)
     tree decl;
{
  tree entries = CONSTRUCTOR_ELTS (DECL_INITIAL (decl));

  for (; entries; entries = TREE_CHAIN (entries))
    {
      tree fnaddr = TREE_VALUE (entries);
      tree fn;
      
      if (TREE_CODE (fnaddr) != ADDR_EXPR
	  && TREE_CODE (fnaddr) != FDESC_EXPR)
	/* This entry is an offset: a virtual base class offset, a
	   virtual call offset, an RTTI offset, etc.  */
	continue;

      fn = TREE_OPERAND (fnaddr, 0);
      TREE_ADDRESSABLE (fn) = 1;
      /* When we don't have vcall offsets, we output thunks whenever
	 we output the vtables that contain them.  With vcall offsets,
	 we know all the thunks we'll need when we emit a virtual
	 function, so we emit the thunks there instead.  */
      if (DECL_THUNK_P (fn)) 
	use_thunk (fn, /*emit_p=*/0);
      mark_used (fn);
    }
}

/* Set DECL up to have the closest approximation of "initialized common"
   linkage available.  */

void
comdat_linkage (decl)
     tree decl;
{
  if (flag_weak)
    make_decl_one_only (decl);
  else if (TREE_CODE (decl) == FUNCTION_DECL 
	   || (TREE_CODE (decl) == VAR_DECL && DECL_ARTIFICIAL (decl)))
    /* We can just emit function and compiler-generated variables
       statically; having multiple copies is (for the most part) only
       a waste of space.  

       There are two correctness issues, however: the address of a
       template instantiation with external linkage should be the
       same, independent of what translation unit asks for the
       address, and this will not hold when we emit multiple copies of
       the function.  However, there's little else we can do.  

       Also, by default, the typeinfo implementation assumes that
       there will be only one copy of the string used as the name for
       each type.  Therefore, if weak symbols are unavailable, the
       run-time library should perform a more conservative check; it
       should perform a string comparison, rather than an address
       comparison.  */
    TREE_PUBLIC (decl) = 0;
  else
    {
      /* Static data member template instantiations, however, cannot
	 have multiple copies.  */
      if (DECL_INITIAL (decl) == 0
	  || DECL_INITIAL (decl) == error_mark_node)
	DECL_COMMON (decl) = 1;
      else if (EMPTY_CONSTRUCTOR_P (DECL_INITIAL (decl)))
	{
	  DECL_COMMON (decl) = 1;
	  DECL_INITIAL (decl) = error_mark_node;
	}
      else
	{
	  /* We can't do anything useful; leave vars for explicit
	     instantiation.  */
	  DECL_EXTERNAL (decl) = 1;
	  DECL_NOT_REALLY_EXTERN (decl) = 0;
	}
    }

  if (DECL_LANG_SPECIFIC (decl))
    DECL_COMDAT (decl) = 1;
}

/* For win32 we also want to put explicit instantiations in
   linkonce sections, so that they will be merged with implicit
   instantiations; otherwise we get duplicate symbol errors.  */

void
maybe_make_one_only (decl)
     tree decl;
{
  /* We used to say that this was not necessary on targets that support weak
     symbols, because the implicit instantiations will defer to the explicit
     one.  However, that's not actually the case in SVR4; a strong definition
     after a weak one is an error.  Also, not making explicit
     instantiations one_only means that we can end up with two copies of
     some template instantiations. */
  if (! flag_weak)
    return;

  /* We can't set DECL_COMDAT on functions, or finish_file will think
     we can get away with not emitting them if they aren't used.  We need
     to for variables so that cp_finish_decl will update their linkage,
     because their DECL_INITIAL may not have been set properly yet.  */

  make_decl_one_only (decl);

  if (TREE_CODE (decl) == VAR_DECL)
    {
      DECL_COMDAT (decl) = 1;
      /* Mark it needed so we don't forget to emit it.  */
      TREE_SYMBOL_REFERENCED (DECL_ASSEMBLER_NAME (decl)) = 1;
    }
}

/* Returns the virtual function with which the vtable for TYPE is
   emitted, or NULL_TREE if that heuristic is not applicable to TYPE.  */

static tree
key_method (type)
     tree type;
{
  tree method;

  if (TYPE_FOR_JAVA (type)
      || CLASSTYPE_TEMPLATE_INSTANTIATION (type)
      || CLASSTYPE_INTERFACE_KNOWN (type))
    return NULL_TREE;

  for (method = TYPE_METHODS (type); method != NULL_TREE;
       method = TREE_CHAIN (method))
    if (DECL_VINDEX (method) != NULL_TREE
	&& ! DECL_DECLARED_INLINE_P (method)
	&& (! DECL_PURE_VIRTUAL_P (method)
#if 0
	    /* This would be nice, but we didn't think of it in time.  */
	    || DECL_DESTRUCTOR_P (method)
#endif
	    ))
      return method;

  return NULL_TREE;
}

/* Set TREE_PUBLIC and/or DECL_EXTERN on the vtable DECL,
   based on TYPE and other static flags.

   Note that anything public is tagged TREE_PUBLIC, whether
   it's public in this file or in another one.  */

void
import_export_vtable (decl, type, final)
     tree decl, type;
     int final;
{
  if (DECL_INTERFACE_KNOWN (decl))
    return;

  if (TYPE_FOR_JAVA (type))
    {
      TREE_PUBLIC (decl) = 1;
      DECL_EXTERNAL (decl) = 1;
      DECL_INTERFACE_KNOWN (decl) = 1;
    }
  else if (CLASSTYPE_INTERFACE_KNOWN (type))
    {
      TREE_PUBLIC (decl) = 1;
      DECL_EXTERNAL (decl) = CLASSTYPE_INTERFACE_ONLY (type);
      DECL_INTERFACE_KNOWN (decl) = 1;
    }
  else
    {
      /* We can only wait to decide if we have real non-inline virtual
	 functions in our class, or if we come from a template.  */

      int found = (CLASSTYPE_TEMPLATE_INSTANTIATION (type)
		   || key_method (type));

      if (final || ! found)
	{
	  comdat_linkage (decl);
	  DECL_EXTERNAL (decl) = 0;
	}
      else
	{
	  TREE_PUBLIC (decl) = 1;
	  DECL_EXTERNAL (decl) = 1;
	}
    }
}

/* Determine whether or not we want to specifically import or export CTYPE,
   using various heuristics.  */

static void
import_export_class (ctype)
     tree ctype;
{
  /* -1 for imported, 1 for exported.  */
  int import_export = 0;

  /* It only makes sense to call this function at EOF.  The reason is
     that this function looks at whether or not the first non-inline
     non-abstract virtual member function has been defined in this
     translation unit.  But, we can't possibly know that until we've
     seen the entire translation unit.  */
  my_friendly_assert (at_eof, 20000226);

  if (CLASSTYPE_INTERFACE_KNOWN (ctype))
    return;

  /* If MULTIPLE_SYMBOL_SPACES is defined and we saw a #pragma interface,
     we will have CLASSTYPE_INTERFACE_ONLY set but not
     CLASSTYPE_INTERFACE_KNOWN.  In that case, we don't want to use this
     heuristic because someone will supply a #pragma implementation
     elsewhere, and deducing it here would produce a conflict.  */
  if (CLASSTYPE_INTERFACE_ONLY (ctype))
    return;

  if (lookup_attribute ("dllimport", TYPE_ATTRIBUTES (ctype)))
    import_export = -1;
  else if (lookup_attribute ("dllexport", TYPE_ATTRIBUTES (ctype)))
    import_export = 1;

  /* If we got -fno-implicit-templates, we import template classes that
     weren't explicitly instantiated.  */
  if (import_export == 0
      && CLASSTYPE_IMPLICIT_INSTANTIATION (ctype)
      && ! flag_implicit_templates)
    import_export = -1;

  /* Base our import/export status on that of the first non-inline,
     non-pure virtual function, if any.  */
  if (import_export == 0
      && TYPE_POLYMORPHIC_P (ctype))
    {
      tree method = key_method (ctype);
      if (method)
	import_export = (DECL_REALLY_EXTERN (method) ? -1 : 1);
    }

#ifdef MULTIPLE_SYMBOL_SPACES
  if (import_export == -1)
    import_export = 0;
#endif

  if (import_export)
    {
      SET_CLASSTYPE_INTERFACE_KNOWN (ctype);
      CLASSTYPE_INTERFACE_ONLY (ctype) = (import_export < 0);
    }
}
    
/* We need to describe to the assembler the relationship between
   a vtable and the vtable of the parent class.  */

static void
output_vtable_inherit (vars)
     tree vars;
{
  tree parent;
  rtx child_rtx, parent_rtx;

  child_rtx = XEXP (DECL_RTL (vars), 0);	  /* strip the mem ref  */

  parent = binfo_for_vtable (vars);

  if (parent == TYPE_BINFO (DECL_CONTEXT (vars)))
    parent_rtx = const0_rtx;
  else if (parent)
    {
      parent = get_vtbl_decl_for_binfo (TYPE_BINFO (BINFO_TYPE (parent)));
      parent_rtx = XEXP (DECL_RTL (parent), 0);  /* strip the mem ref  */
    }
  else
    abort ();

  assemble_vtable_inherit (child_rtx, parent_rtx);
}

static int
finish_vtable_vardecl (t, data)
     tree *t;
     void *data ATTRIBUTE_UNUSED;
{
  tree vars = *t;
  tree ctype = DECL_CONTEXT (vars);
  import_export_class (ctype);
  import_export_vtable (vars, ctype, 1);

  if (! DECL_EXTERNAL (vars)
      && DECL_NEEDED_P (vars)
      && ! TREE_ASM_WRITTEN (vars))
    {
      if (TREE_TYPE (vars) == void_type_node)
        /* It is a dummy vtable made by get_vtable_decl. Ignore it.  */
        return 0;
      
      /* Write it out.  */
      mark_vtable_entries (vars);
      if (TREE_TYPE (DECL_INITIAL (vars)) == 0)
	store_init_value (vars, DECL_INITIAL (vars));

      if (write_symbols == DWARF_DEBUG || write_symbols == DWARF2_DEBUG)
	{
	  /* Mark the VAR_DECL node representing the vtable itself as a
	     "gratuitous" one, thereby forcing dwarfout.c to ignore it.
	     It is rather important that such things be ignored because
	     any effort to actually generate DWARF for them will run
	     into trouble when/if we encounter code like:

		#pragma interface
		struct S { virtual void member (); };

	      because the artificial declaration of the vtable itself (as
	      manufactured by the g++ front end) will say that the vtable
	      is a static member of `S' but only *after* the debug output
	      for the definition of `S' has already been output.  This causes
	      grief because the DWARF entry for the definition of the vtable
	      will try to refer back to an earlier *declaration* of the
	      vtable as a static member of `S' and there won't be one.
	      We might be able to arrange to have the "vtable static member"
	      attached to the member list for `S' before the debug info for
	      `S' get written (which would solve the problem) but that would
	      require more intrusive changes to the g++ front end.  */

	  DECL_IGNORED_P (vars) = 1;
	}

      /* Always make vtables weak.  */
      if (flag_weak)
	comdat_linkage (vars);

      rest_of_decl_compilation (vars, NULL, 1, 1);

      if (flag_vtable_gc)
	output_vtable_inherit (vars);

      /* Because we're only doing syntax-checking, we'll never end up
	 actually marking the variable as written.  */
      if (flag_syntax_only)
	TREE_ASM_WRITTEN (vars) = 1;

      /* Since we're writing out the vtable here, also write the debug 
	 info.  */
      note_debug_info_needed (ctype);

      return 1;
    }

  /* If the references to this class' vtables were optimized away, still
     emit the appropriate debugging information.  See dfs_debug_mark.  */
  if (DECL_COMDAT (vars)
      && CLASSTYPE_DEBUG_REQUESTED (ctype))
    note_debug_info_needed (ctype);

  return 0;
}

static int
prune_vtable_vardecl (t, data)
     tree *t;
     void *data ATTRIBUTE_UNUSED;
{
  *t = TREE_CHAIN (*t);
  return 1;
}

/* Determines the proper settings of TREE_PUBLIC and DECL_EXTERNAL for an
   inline function or template instantiation at end-of-file.  */

void
import_export_decl (decl)
     tree decl;
{
  if (DECL_INTERFACE_KNOWN (decl))
    return;

  if (DECL_TEMPLATE_INSTANTIATION (decl)
      || DECL_FRIEND_PSEUDO_TEMPLATE_INSTANTIATION (decl))
    {
      DECL_NOT_REALLY_EXTERN (decl) = 1;
      if ((DECL_IMPLICIT_INSTANTIATION (decl)
	   || DECL_FRIEND_PSEUDO_TEMPLATE_INSTANTIATION (decl))
	  && (flag_implicit_templates
	      || (flag_implicit_inline_templates 
		  && DECL_DECLARED_INLINE_P (decl))))
	{
	  if (!TREE_PUBLIC (decl))
	    /* Templates are allowed to have internal linkage.  See 
	       [basic.link].  */
	    ;
	  else
	    comdat_linkage (decl);
	}
      else
	{
	  DECL_EXTERNAL (decl) = 1;
	  DECL_NOT_REALLY_EXTERN (decl) = 0;
	}
    }
  else if (DECL_FUNCTION_MEMBER_P (decl))
    {
      if (!DECL_DECLARED_INLINE_P (decl))
	{
	  tree ctype = DECL_CONTEXT (decl);
	  import_export_class (ctype);
	  if (CLASSTYPE_INTERFACE_KNOWN (ctype))
	    {
	      DECL_NOT_REALLY_EXTERN (decl)
		= ! (CLASSTYPE_INTERFACE_ONLY (ctype)
		     || (DECL_DECLARED_INLINE_P (decl) 
			 && ! flag_implement_inlines
			 && !DECL_VINDEX (decl)));

	      if (!DECL_NOT_REALLY_EXTERN (decl))
		DECL_EXTERNAL (decl) = 1;

	      /* Always make artificials weak.  */
	      if (DECL_ARTIFICIAL (decl) && flag_weak)
		comdat_linkage (decl);
	      else
		maybe_make_one_only (decl);
	    }
	}
      else
	comdat_linkage (decl);
    }
  else
    comdat_linkage (decl);

  DECL_INTERFACE_KNOWN (decl) = 1;
}

/* Here, we only decide whether or not the tinfo node should be
   emitted with the vtable.  IS_IN_LIBRARY is non-zero iff the
   typeinfo for TYPE should be in the runtime library.  */

void
import_export_tinfo (decl, type, is_in_library)
     tree decl;
     tree type;
     int is_in_library;
{
  if (DECL_INTERFACE_KNOWN (decl))
    return;
  
  if (IS_AGGR_TYPE (type))
    import_export_class (type);
      
  if (IS_AGGR_TYPE (type) && CLASSTYPE_INTERFACE_KNOWN (type)
      && TYPE_POLYMORPHIC_P (type)
      /* If -fno-rtti, we're not necessarily emitting this stuff with
	 the class, so go ahead and emit it now.  This can happen when
	 a class is used in exception handling.  */
      && flag_rtti)
    {
      DECL_NOT_REALLY_EXTERN (decl) = !CLASSTYPE_INTERFACE_ONLY (type);
      DECL_COMDAT (decl) = 0;
    }
  else
    {
      DECL_NOT_REALLY_EXTERN (decl) = 1;
      DECL_COMDAT (decl) = 1;
    }

  /* Now override some cases. */
  if (flag_weak)
    DECL_COMDAT (decl) = 1;
  else if (is_in_library)
    DECL_COMDAT (decl) = 0;
  
  DECL_INTERFACE_KNOWN (decl) = 1;
}

tree
build_cleanup (decl)
     tree decl;
{
  tree temp;
  tree type = TREE_TYPE (decl);

  if (TREE_CODE (type) == ARRAY_TYPE)
    temp = decl;
  else
    {
      mark_addressable (decl);
      temp = build1 (ADDR_EXPR, build_pointer_type (type), decl);
    }
  temp = build_delete (TREE_TYPE (temp), temp,
		       sfk_complete_destructor,
		       LOOKUP_NORMAL|LOOKUP_NONVIRTUAL|LOOKUP_DESTRUCTOR, 0);
  return temp;
}

/* Returns the initialization guard variable for the variable DECL,
   which has static storage duration.  */

tree
get_guard (decl)
     tree decl;
{
  tree sname;
  tree guard;

  sname = mangle_guard_variable (decl);
  guard = IDENTIFIER_GLOBAL_VALUE (sname);
  if (! guard)
    {
      tree guard_type;

      /* We use a type that is big enough to contain a mutex as well
	 as an integer counter.  */
      guard_type = long_long_integer_type_node;
      guard = build_decl (VAR_DECL, sname, guard_type);
      
      /* The guard should have the same linkage as what it guards. */
      TREE_PUBLIC (guard) = TREE_PUBLIC (decl);
      TREE_STATIC (guard) = TREE_STATIC (decl);
      DECL_COMMON (guard) = DECL_COMMON (decl);
      DECL_ONE_ONLY (guard) = DECL_ONE_ONLY (decl);
      if (TREE_PUBLIC (decl))
        DECL_WEAK (guard) = DECL_WEAK (decl);
      
      DECL_ARTIFICIAL (guard) = 1;
      TREE_USED (guard) = 1;
      pushdecl_top_level (guard);
      cp_finish_decl (guard, NULL_TREE, NULL_TREE, 0);
    }
  return guard;
}

/* Return those bits of the GUARD variable that should be set when the
   guarded entity is actually initialized.  */

static tree
get_guard_bits (guard)
     tree guard;
{
  /* We only set the first byte of the guard, in order to leave room
     for a mutex in the high-order bits.  */
  guard = build1 (ADDR_EXPR, 
		  build_pointer_type (TREE_TYPE (guard)),
		  guard);
  guard = build1 (NOP_EXPR, 
		  build_pointer_type (char_type_node), 
		  guard);
  guard = build1 (INDIRECT_REF, char_type_node, guard);

  return guard;
}

/* Return an expression which determines whether or not the GUARD
   variable has already been initialized.  */

tree
get_guard_cond (guard)
     tree guard;
{
  tree guard_value;

  /* Check to see if the GUARD is zero.  */
  guard = get_guard_bits (guard);
  guard_value = integer_zero_node;
  if (!same_type_p (TREE_TYPE (guard_value), TREE_TYPE (guard)))
    guard_value = convert (TREE_TYPE (guard), guard_value);
  return cp_build_binary_op (EQ_EXPR, guard, guard_value);
}

/* Return an expression which sets the GUARD variable, indicating that
   the variable being guarded has been initialized.  */

tree
set_guard (guard)
     tree guard;
{
  tree guard_init;

  /* Set the GUARD to one.  */
  guard = get_guard_bits (guard);
  guard_init = integer_one_node;
  if (!same_type_p (TREE_TYPE (guard_init), TREE_TYPE (guard)))
    guard_init = convert (TREE_TYPE (guard), guard_init);
  return build_modify_expr (guard, NOP_EXPR, guard_init);
}

/* Start the process of running a particular set of global constructors
   or destructors.  Subroutine of do_[cd]tors.  */

static tree
start_objects (method_type, initp)
     int method_type, initp;
{
  tree fnname;
  tree body;
  char type[10];

  /* Make ctor or dtor function.  METHOD_TYPE may be 'I' or 'D'.  */

  if (initp != DEFAULT_INIT_PRIORITY)
    {
      char joiner;

#ifdef JOINER
      joiner = JOINER;
#else
      joiner = '_';
#endif

      sprintf (type, "%c%c%.5u", method_type, joiner, initp);
    }
  else
    sprintf (type, "%c", method_type);

  fnname = get_file_function_name_long (type);

  start_function (void_list_node,
		  make_call_declarator (fnname, void_list_node, NULL_TREE,
					NULL_TREE),
		  NULL_TREE, SF_DEFAULT);

  /* It can be a static function as long as collect2 does not have
     to scan the object file to find its ctor/dtor routine.  */
  TREE_PUBLIC (current_function_decl) = ! targetm.have_ctors_dtors;

  /* Mark this declaration as used to avoid spurious warnings.  */
  TREE_USED (current_function_decl) = 1;

  /* Mark this function as a global constructor or destructor.  */
  if (method_type == 'I')
    DECL_GLOBAL_CTOR_P (current_function_decl) = 1;
  else
    DECL_GLOBAL_DTOR_P (current_function_decl) = 1;
  GLOBAL_INIT_PRIORITY (current_function_decl) = initp;

  body = begin_compound_stmt (/*has_no_scope=*/0);

  /* We cannot allow these functions to be elided, even if they do not
     have external linkage.  And, there's no point in deferring
     copmilation of thes functions; they're all going to have to be
     out anyhow.  */
  current_function_cannot_inline
    = "static constructors and destructors cannot be inlined";

  return body;
}

/* Finish the process of running a particular set of global constructors
   or destructors.  Subroutine of do_[cd]tors.  */

static void
finish_objects (method_type, initp, body)
     int method_type, initp;
     tree body;
{
  tree fn;

  /* Finish up.  */
  finish_compound_stmt (/*has_no_scope=*/0, body);
  fn = finish_function (0);
  expand_body (fn);

  /* When only doing semantic analysis, and no RTL generation, we
     can't call functions that directly emit assembly code; there is
     no assembly file in which to put the code.  */
  if (flag_syntax_only)
    return;

  if (targetm.have_ctors_dtors)
    {
      rtx fnsym = XEXP (DECL_RTL (fn), 0);
      if (method_type == 'I')
	(* targetm.asm_out.constructor) (fnsym, initp);
      else
	(* targetm.asm_out.destructor) (fnsym, initp);
    }
}

/* The names of the parameters to the function created to handle
   initializations and destructions for objects with static storage
   duration.  */
#define INITIALIZE_P_IDENTIFIER "__initialize_p"
#define PRIORITY_IDENTIFIER "__priority"

/* The name of the function we create to handle initializations and
   destructions for objects with static storage duration.  */
#define SSDF_IDENTIFIER "__static_initialization_and_destruction"

/* The declaration for the __INITIALIZE_P argument.  */
static tree initialize_p_decl;

/* The declaration for the __PRIORITY argument.  */
static tree priority_decl;

/* The declaration for the static storage duration function.  */
static tree ssdf_decl;

/* All the static storage duration functions created in this
   translation unit.  */
static varray_type ssdf_decls;

/* A map from priority levels to information about that priority
   level.  There may be many such levels, so efficient lookup is
   important.  */
static splay_tree priority_info_map;

/* Begins the generation of the function that will handle all
   initialization and destruction of objects with static storage
   duration.  The function generated takes two parameters of type
   `int': __INITIALIZE_P and __PRIORITY.  If __INITIALIZE_P is
   non-zero, it performs initializations.  Otherwise, it performs
   destructions.  It only performs those initializations or
   destructions with the indicated __PRIORITY.  The generated function
   returns no value.  

   It is assumed that this function will only be called once per
   translation unit.  */

static tree
start_static_storage_duration_function ()
{
  static unsigned ssdf_number;

  tree parm_types;
  tree type;
  tree body;
  char id[sizeof (SSDF_IDENTIFIER) + 1 /* '\0' */ + 32];

  /* Create the identifier for this function.  It will be of the form
     SSDF_IDENTIFIER_<number>.  */
  sprintf (id, "%s_%u", SSDF_IDENTIFIER, ssdf_number++);
  if (ssdf_number == 0)
    {
      /* Overflow occurred.  That means there are at least 4 billion
	 initialization functions.  */
      sorry ("too many initialization functions required");
      abort ();
    }

  /* Create the parameters.  */
  parm_types = void_list_node;
  parm_types = tree_cons (NULL_TREE, integer_type_node, parm_types);
  parm_types = tree_cons (NULL_TREE, integer_type_node, parm_types);
  type = build_function_type (void_type_node, parm_types);

  /* Create the FUNCTION_DECL itself.  */
  ssdf_decl = build_lang_decl (FUNCTION_DECL, 
			       get_identifier (id),
			       type);
  TREE_PUBLIC (ssdf_decl) = 0;
  DECL_ARTIFICIAL (ssdf_decl) = 1;

  /* Put this function in the list of functions to be called from the
     static constructors and destructors.  */
  if (!ssdf_decls)
    {
      VARRAY_TREE_INIT (ssdf_decls, 32, "ssdf_decls");

      /* Take this opportunity to initialize the map from priority
	 numbers to information about that priority level. */
      priority_info_map = splay_tree_new (splay_tree_compare_ints,
					  /*delete_key_fn=*/0,
					  /*delete_value_fn=*/
					  (splay_tree_delete_value_fn) &free);

      /* We always need to generate functions for the
	 DEFAULT_INIT_PRIORITY so enter it now.  That way when we walk
	 priorities later, we'll be sure to find the
	 DEFAULT_INIT_PRIORITY.  */
      get_priority_info (DEFAULT_INIT_PRIORITY);
    }

  VARRAY_PUSH_TREE (ssdf_decls, ssdf_decl);

  /* Create the argument list.  */
  initialize_p_decl = build_decl (PARM_DECL,
				  get_identifier (INITIALIZE_P_IDENTIFIER),
				  integer_type_node);
  DECL_CONTEXT (initialize_p_decl) = ssdf_decl;
  DECL_ARG_TYPE (initialize_p_decl) = integer_type_node;
  TREE_USED (initialize_p_decl) = 1;
  priority_decl = build_decl (PARM_DECL, get_identifier (PRIORITY_IDENTIFIER),
			      integer_type_node);
  DECL_CONTEXT (priority_decl) = ssdf_decl;
  DECL_ARG_TYPE (priority_decl) = integer_type_node;
  TREE_USED (priority_decl) = 1;

  TREE_CHAIN (initialize_p_decl) = priority_decl;
  DECL_ARGUMENTS (ssdf_decl) = initialize_p_decl;

  /* Put the function in the global scope.  */
  pushdecl (ssdf_decl);

  /* Start the function itself.  This is equivalent to declarating the
     function as:

       static void __ssdf (int __initialize_p, init __priority_p);
       
     It is static because we only need to call this function from the
     various constructor and destructor functions for this module.  */
  start_function (/*specs=*/NULL_TREE, 
		  ssdf_decl,
		  /*attrs=*/NULL_TREE,
		  SF_PRE_PARSED);

  /* Set up the scope of the outermost block in the function.  */
  body = begin_compound_stmt (/*has_no_scope=*/0);

  /* This function must not be deferred because we are depending on
     its compilation to tell us what is TREE_SYMBOL_REFERENCED.  */
  current_function_cannot_inline 
    = "static storage duration functions cannot be inlined";

  return body;
}

/* Finish the generation of the function which performs initialization
   and destruction of objects with static storage duration.  After
   this point, no more such objects can be created.  */

static void
finish_static_storage_duration_function (body)
     tree body;
{
  /* Close out the function.  */
  finish_compound_stmt (/*has_no_scope=*/0, body);
  expand_body (finish_function (0));
}

/* Return the information about the indicated PRIORITY level.  If no
   code to handle this level has yet been generated, generate the
   appropriate prologue.  */

static priority_info
get_priority_info (priority)
     int priority;
{
  priority_info pi;
  splay_tree_node n;

  n = splay_tree_lookup (priority_info_map, 
			 (splay_tree_key) priority);
  if (!n)
    {
      /* Create a new priority information structure, and insert it
	 into the map.  */
      pi = (priority_info) xmalloc (sizeof (struct priority_info_s));
      pi->initializations_p = 0;
      pi->destructions_p = 0;
      splay_tree_insert (priority_info_map,
			 (splay_tree_key) priority,
			 (splay_tree_value) pi);
    }
  else
    pi = (priority_info) n->value;

  return pi;
}

/* Set up to handle the initialization or destruction of DECL.  If
   INITP is non-zero, we are initializing the variable.  Otherwise, we
   are destroying it.  */

static tree
start_static_initialization_or_destruction (decl, initp)
     tree decl;
     int initp;
{
  tree guard_if_stmt = NULL_TREE;
  int priority;
  tree cond;
  tree guard;
  tree init_cond;
  priority_info pi;

  /* Figure out the priority for this declaration.  */
  priority = DECL_INIT_PRIORITY (decl);
  if (!priority)
    priority = DEFAULT_INIT_PRIORITY;

  /* Remember that we had an initialization or finalization at this
     priority.  */
  pi = get_priority_info (priority);
  if (initp)
    pi->initializations_p = 1;
  else
    pi->destructions_p = 1;

  /* Trick the compiler into thinking we are at the file and line
     where DECL was declared so that error-messages make sense, and so
     that the debugger will show somewhat sensible file and line
     information.  */
  input_filename = DECL_SOURCE_FILE (decl);
  lineno = DECL_SOURCE_LINE (decl);

  /* Because of:

       [class.access.spec]

       Access control for implicit calls to the constructors,
       the conversion functions, or the destructor called to
       create and destroy a static data member is performed as
       if these calls appeared in the scope of the member's
       class.  

     we pretend we are in a static member function of the class of
     which the DECL is a member.  */
  if (member_p (decl))
    {
      DECL_CONTEXT (current_function_decl) = DECL_CONTEXT (decl);
      DECL_STATIC_FUNCTION_P (current_function_decl) = 1;
    }
  
  /* Conditionalize this initialization on being in the right priority
     and being initializing/finalizing appropriately.  */
  guard_if_stmt = begin_if_stmt ();
  cond = cp_build_binary_op (EQ_EXPR,
			     priority_decl,
			     build_int_2 (priority, 0));
  init_cond = initp ? integer_one_node : integer_zero_node;
  init_cond = cp_build_binary_op (EQ_EXPR,
				  initialize_p_decl,
				  init_cond);
  cond = cp_build_binary_op (TRUTH_ANDIF_EXPR, cond, init_cond);

  /* Assume we don't need a guard.  */
  guard = NULL_TREE;
  /* We need a guard if this is an object with external linkage that
     might be initialized in more than one place.  (For example, a
     static data member of a template, when the data member requires
     construction.)  */
  if (TREE_PUBLIC (decl) && (DECL_COMMON (decl) 
			     || DECL_ONE_ONLY (decl)
			     || DECL_WEAK (decl)))
    {
      tree guard_cond;

      guard = get_guard (decl);

      /* When using __cxa_atexit, we just check the GUARD as we would
	 for a local static.  */
      if (flag_use_cxa_atexit)
	{
	  /* When using __cxa_atexit, we never try to destroy
	     anything from a static destructor.  */
	  my_friendly_assert (initp, 20000629);
	  guard_cond = get_guard_cond (guard);
	}
      /* If we don't have __cxa_atexit, then we will be running
	 destructors from .fini sections, or their equivalents.  So,
	 we need to know how many times we've tried to initialize this
	 object.  We do initializations only if the GUARD is zero,
	 i.e., if we are the first to initialize the variable.  We do
	 destructions only if the GUARD is one, i.e., if we are the
	 last to destroy the variable.  */
      else if (initp)
	guard_cond 
	  = cp_build_binary_op (EQ_EXPR,
				build_unary_op (PREINCREMENT_EXPR,
						guard,
						/*noconvert=*/1),
				integer_one_node);
      else
	guard_cond 
	  = cp_build_binary_op (EQ_EXPR,
				build_unary_op (PREDECREMENT_EXPR,
						guard,
						/*noconvert=*/1),
				integer_zero_node);

      cond = cp_build_binary_op (TRUTH_ANDIF_EXPR, cond, guard_cond);
    }

  finish_if_stmt_cond (cond, guard_if_stmt);

  /* If we're using __cxa_atexit, we have not already set the GUARD,
     so we must do so now.  */
  if (guard && initp && flag_use_cxa_atexit)
    finish_expr_stmt (set_guard (guard));

  return guard_if_stmt;
}

/* We've just finished generating code to do an initialization or
   finalization.  GUARD_IF_STMT is the if-statement we used to guard
   the initialization.  */

static void
finish_static_initialization_or_destruction (guard_if_stmt)
     tree guard_if_stmt;
{
  finish_then_clause (guard_if_stmt);
  finish_if_stmt ();

  /* Now that we're done with DECL we don't need to pretend to be a
     member of its class any longer.  */
  DECL_CONTEXT (current_function_decl) = NULL_TREE;
  DECL_STATIC_FUNCTION_P (current_function_decl) = 0;
}

/* Generate code to do the static initialization of DECL.  The
   initialization is INIT.  If DECL may be initialized more than once
   in different object files, GUARD is the guard variable to 
   check.  PRIORITY is the priority for the initialization.  */

static void
do_static_initialization (decl, init)
     tree decl;
     tree init;
{
  tree expr;
  tree guard_if_stmt;

  /* Set up for the initialization.  */
  guard_if_stmt
    = start_static_initialization_or_destruction (decl,
						  /*initp=*/1);
  
  /* Do the initialization itself.  */
  if (IS_AGGR_TYPE (TREE_TYPE (decl))
      || TREE_CODE (TREE_TYPE (decl)) == ARRAY_TYPE)
    expr = build_aggr_init (decl, init, 0);
  else
    {
      expr = build (INIT_EXPR, TREE_TYPE (decl), decl, init);
      TREE_SIDE_EFFECTS (expr) = 1;
    }
  finish_expr_stmt (expr);

  /* If we're using __cxa_atexit, register a a function that calls the
     destructor for the object.  */
  if (flag_use_cxa_atexit)
    register_dtor_fn (decl);

  /* Finsh up.  */
  finish_static_initialization_or_destruction (guard_if_stmt);
}

/* Generate code to do the static destruction of DECL.  If DECL may be
   initialized more than once in different object files, GUARD is the
   guard variable to check.  PRIORITY is the priority for the
   destruction.  */

static void
do_static_destruction (decl)
     tree decl;
{
  tree guard_if_stmt;

  /* If we're using __cxa_atexit, then destructors are registered
     immediately after objects are initialized.  */
  my_friendly_assert (!flag_use_cxa_atexit, 20000121);

  /* If we don't need a destructor, there's nothing to do.  */
  if (TYPE_HAS_TRIVIAL_DESTRUCTOR (TREE_TYPE (decl)))
    return;

  /* Actually do the destruction.  */
  guard_if_stmt = start_static_initialization_or_destruction (decl,
							       /*initp=*/0);
  finish_expr_stmt (build_cleanup (decl));
  finish_static_initialization_or_destruction (guard_if_stmt);
}

/* VARS is a list of variables with static storage duration which may
   need initialization and/or finalization.  Remove those variables
   that don't really need to be initialized or finalized, and return
   the resulting list.  The order in which the variables appear in
   VARS is in reverse order of the order in which they should actually
   be initialized.  The list we return is in the unreversed order;
   i.e., the first variable should be initialized first.  */

static tree
prune_vars_needing_no_initialization (vars)
     tree vars;
{
  tree var;
  tree result;

  for (var = vars, result = NULL_TREE;
       var;
       var = TREE_CHAIN (var))
    {
      tree decl = TREE_VALUE (var);
      tree init = TREE_PURPOSE (var);

      /* Deal gracefully with error.  */
      if (decl == error_mark_node)
	continue;

      /* The only things that can be initialized are variables.  */
      my_friendly_assert (TREE_CODE (decl) == VAR_DECL, 19990420);

      /* If this object is not defined, we don't need to do anything
	 here.  */
      if (DECL_EXTERNAL (decl))
	continue;

      /* Also, if the initializer already contains errors, we can bail
	 out now.  */
      if (init && TREE_CODE (init) == TREE_LIST 
	  && value_member (error_mark_node, init))
	continue;

      /* This variable is going to need initialization and/or
	 finalization, so we add it to the list.  */
      result = tree_cons (init, decl, result);
    }

  return result;
}

/* Make sure we have told the back end about all the variables in
   VARS.  */

static void
write_out_vars (vars)
     tree vars;
{
  tree v;

  for (v = vars; v; v = TREE_CHAIN (v))
    if (! TREE_ASM_WRITTEN (TREE_VALUE (v)))
      rest_of_decl_compilation (TREE_VALUE (v), 0, 1, 1);
}

/* Generate a static constructor (if CONSTRUCTOR_P) or destructor
   (otherwise) that will initialize all gobal objects with static
   storage duration having the indicated PRIORITY.  */

static void
generate_ctor_or_dtor_function (constructor_p, priority)
     int constructor_p;
     int priority;
{
  char function_key;
  tree arguments;
  tree body;
  size_t i;

  /* We use `I' to indicate initialization and `D' to indicate
     destruction.  */
  if (constructor_p)
    function_key = 'I';
  else
    function_key = 'D';

  /* Begin the function.  */
  body = start_objects (function_key, priority);

  /* Call the static storage duration function with appropriate
     arguments.  */
  for (i = 0; i < ssdf_decls->elements_used; ++i) 
    {
      arguments = tree_cons (NULL_TREE, build_int_2 (priority, 0), 
			     NULL_TREE);
      arguments = tree_cons (NULL_TREE, build_int_2 (constructor_p, 0),
			     arguments);
      finish_expr_stmt (build_function_call (VARRAY_TREE (ssdf_decls, i),
					     arguments));
    }

  /* If we're generating code for the DEFAULT_INIT_PRIORITY, throw in
     calls to any functions marked with attributes indicating that
     they should be called at initialization- or destruction-time.  */
  if (priority == DEFAULT_INIT_PRIORITY)
    {
      tree fns;
      
      for (fns = constructor_p ? static_ctors : static_dtors; 
	   fns;
	   fns = TREE_CHAIN (fns))
	finish_expr_stmt (build_function_call (TREE_VALUE (fns), NULL_TREE));
    }

  /* Close out the function.  */
  finish_objects (function_key, priority, body);
}

/* Generate constructor and destructor functions for the priority
   indicated by N.  */

static int
generate_ctor_and_dtor_functions_for_priority (n, data)
     splay_tree_node n;
     void *data ATTRIBUTE_UNUSED;
{
  int priority = (int) n->key;
  priority_info pi = (priority_info) n->value;

  /* Generate the functions themselves, but only if they are really
     needed.  */
  if (pi->initializations_p
      || (priority == DEFAULT_INIT_PRIORITY && static_ctors))
    generate_ctor_or_dtor_function (/*constructor_p=*/1,
				    priority);
  if (pi->destructions_p
      || (priority == DEFAULT_INIT_PRIORITY && static_dtors))
    generate_ctor_or_dtor_function (/*constructor_p=*/0,
				    priority);

  /* Keep iterating.  */
  return 0;
}

/* This routine is called from the last rule in yyparse ().
   Its job is to create all the code needed to initialize and
   destroy the global aggregates.  We do the destruction
   first, since that way we only need to reverse the decls once.  */

void
finish_file ()
{
  tree vars;
  int reconsider;
  size_t i;

  at_eof = 1;

  /* Bad parse errors.  Just forget about it.  */
  if (! global_bindings_p () || current_class_type || decl_namespace_list)
    return;

  /* Otherwise, GDB can get confused, because in only knows
     about source for LINENO-1 lines.  */
  lineno -= 1;

  interface_unknown = 1;
  interface_only = 0;

  /* We now have to write out all the stuff we put off writing out.
     These include:

       o Template specializations that we have not yet instantiated,
         but which are needed.
       o Initialization and destruction for non-local objects with
         static storage duration.  (Local objects with static storage
	 duration are initialized when their scope is first entered,
	 and are cleaned up via atexit.)
       o Virtual function tables.  

     All of these may cause others to be needed.  For example,
     instantiating one function may cause another to be needed, and
     generating the intiailzer for an object may cause templates to be
     instantiated, etc., etc.  */

  timevar_push (TV_VARCONST);

  emit_support_tinfos ();
  
  do 
    {
      reconsider = 0;

      /* If there are templates that we've put off instantiating, do
	 them now.  */
      instantiate_pending_templates ();

      /* Write out virtual tables as required.  Note that writing out
	 the virtual table for a template class may cause the
	 instantiation of members of that class.  */
      if (walk_globals (vtable_decl_p,
			finish_vtable_vardecl,
			/*data=*/0))
	reconsider = 1;
      
      /* Write out needed type info variables. Writing out one variable
         might cause others to be needed.  */
      if (walk_globals (unemitted_tinfo_decl_p, emit_tinfo_decl, /*data=*/0))
	reconsider = 1;

      /* The list of objects with static storage duration is built up
	 in reverse order.  We clear STATIC_AGGREGATES so that any new
	 aggregates added during the initialization of these will be
	 initialized in the correct order when we next come around the
	 loop.  */
      vars = prune_vars_needing_no_initialization (static_aggregates);
      static_aggregates = NULL_TREE;

      if (vars)
	{
	  tree v;

	  /* We need to start a new initialization function each time
	     through the loop.  That's because we need to know which
	     vtables have been referenced, and TREE_SYMBOL_REFERENCED
	     isn't computed until a function is finished, and written
	     out.  That's a deficiency in the back-end.  When this is
	     fixed, these initialization functions could all become
	     inline, with resulting performance improvements.  */
	  tree ssdf_body = start_static_storage_duration_function ();

	  /* Make sure the back end knows about all the variables.  */
	  write_out_vars (vars);

	  /* First generate code to do all the initializations.  */
	  for (v = vars; v; v = TREE_CHAIN (v))
	    do_static_initialization (TREE_VALUE (v),
				      TREE_PURPOSE (v));

	  /* Then, generate code to do all the destructions.  Do these
	     in reverse order so that the most recently constructed
	     variable is the first destroyed.  If we're using
	     __cxa_atexit, then we don't need to do this; functions
	     were registered at initialization time to destroy the
	     local statics.  */
	  if (!flag_use_cxa_atexit)
	    {
	      vars = nreverse (vars);
	      for (v = vars; v; v = TREE_CHAIN (v))
		do_static_destruction (TREE_VALUE (v));
	    }
	  else
	    vars = NULL_TREE;

	  /* Finish up the static storage duration function for this
	     round.  */
	  finish_static_storage_duration_function (ssdf_body);

	  /* All those initializations and finalizations might cause
	     us to need more inline functions, more template
	     instantiations, etc.  */
	  reconsider = 1;
	}
      
      /* Go through the various inline functions, and see if any need
	 synthesizing.  */
      for (i = 0; i < deferred_fns_used; ++i)
	{
	  tree decl = VARRAY_TREE (deferred_fns, i);
	  import_export_decl (decl);
	  if (DECL_ARTIFICIAL (decl) && ! DECL_INITIAL (decl)
	      && TREE_USED (decl)
	      && (! DECL_REALLY_EXTERN (decl) || DECL_INLINE (decl)))
	    {
	      /* Even though we're already at the top-level, we push
		 there again.  That way, when we pop back a few lines
		 hence, all of our state is restored.  Otherwise,
		 finish_function doesn't clean things up, and we end
		 up with CURRENT_FUNCTION_DECL set.  */
	      push_to_top_level ();
	      synthesize_method (decl);
	      pop_from_top_level ();
	      reconsider = 1;
	    }
	}

      /* We lie to the back-end, pretending that some functions are
	 not defined when they really are.  This keeps these functions
	 from being put out unnecessarily.  But, we must stop lying
	 when the functions are referenced, or if they are not comdat
	 since they need to be put out now.
	 This is done in a separate for cycle, because if some deferred
	 function is contained in another deferred function later in
	 deferred_fns varray, rest_of_compilation would skip this
	 function and we really cannot expand the same function twice. */
      for (i = 0; i < deferred_fns_used; ++i)
	{
	  tree decl = VARRAY_TREE (deferred_fns, i);
      
	  if (DECL_NOT_REALLY_EXTERN (decl)
	      && DECL_INITIAL (decl)
	      && DECL_NEEDED_P (decl))
	    DECL_EXTERNAL (decl) = 0;
	}

      for (i = 0; i < deferred_fns_used; ++i)
	{
	  tree decl = VARRAY_TREE (deferred_fns, i);

	  /* If we're going to need to write this function out, and
	     there's already a body for it, create RTL for it now.
	     (There might be no body if this is a method we haven't
	     gotten around to synthesizing yet.)  */
	  if (!DECL_EXTERNAL (decl)
	      && DECL_NEEDED_P (decl)
	      && DECL_SAVED_TREE (decl)
	      && !TREE_ASM_WRITTEN (decl))
	    {
	      int saved_not_really_extern;

	      /* When we call finish_function in expand_body, it will
		 try to reset DECL_NOT_REALLY_EXTERN so we save and
		 restore it here.  */
	      saved_not_really_extern = DECL_NOT_REALLY_EXTERN (decl);
	      /* Generate RTL for this function now that we know we
		 need it.  */
	      expand_body (decl);
	      /* Undo the damage done by finish_function.  */
	      DECL_EXTERNAL (decl) = 0;
	      DECL_NOT_REALLY_EXTERN (decl) = saved_not_really_extern;
	      /* If we're compiling -fsyntax-only pretend that this
		 function has been written out so that we don't try to
		 expand it again.  */
	      if (flag_syntax_only)
		TREE_ASM_WRITTEN (decl) = 1;
	      reconsider = 1;
	    }
	}

      if (deferred_fns_used
	  && wrapup_global_declarations (&VARRAY_TREE (deferred_fns, 0),
					 deferred_fns_used))
	reconsider = 1;
      if (walk_namespaces (wrapup_globals_for_namespace, /*data=*/0))
	reconsider = 1;

      /* Static data members are just like namespace-scope globals.  */
      for (i = 0; i < pending_statics_used; ++i) 
	{
	  tree decl = VARRAY_TREE (pending_statics, i);
	  if (TREE_ASM_WRITTEN (decl))
	    continue;
	  import_export_decl (decl);
	  if (DECL_NOT_REALLY_EXTERN (decl) && ! DECL_IN_AGGR_P (decl))
	    DECL_EXTERNAL (decl) = 0;
	}
      if (pending_statics
	  && wrapup_global_declarations (&VARRAY_TREE (pending_statics, 0),
					 pending_statics_used))
	reconsider = 1;
    } 
  while (reconsider);

  /* We give C linkage to static constructors and destructors.  */
  push_lang_context (lang_name_c);

  /* Generate initialization and destruction functions for all
     priorities for which they are required.  */
  if (priority_info_map)
    splay_tree_foreach (priority_info_map, 
			generate_ctor_and_dtor_functions_for_priority,
			/*data=*/0);

  /* We're done with the splay-tree now.  */
  if (priority_info_map)
    splay_tree_delete (priority_info_map);

  /* We're done with static constructors, so we can go back to "C++"
     linkage now.  */
  pop_lang_context ();

  /* Now delete from the chain of variables all virtual function tables.
     We output them all ourselves, because each will be treated
     specially.  We don't do this if we're just doing semantic
     analysis, and not code-generation.  */
  if (!flag_syntax_only)
    walk_globals (vtable_decl_p, prune_vtable_vardecl, /*data=*/0);

  /* Now, issue warnings about static, but not defined, functions,
     etc., and emit debugging information.  */
  walk_namespaces (wrapup_globals_for_namespace, /*data=*/&reconsider);
  if (pending_statics)
    check_global_declarations (&VARRAY_TREE (pending_statics, 0),
			       pending_statics_used);

  finish_repo ();

  /* The entire file is now complete.  If requested, dump everything
     to a file.   */
  {
    int flags;
    FILE *stream = dump_begin (TDI_all, &flags);

    if (stream)
      {
	dump_node (global_namespace, flags & ~TDF_SLIM, stream);
	dump_end (TDI_all, stream);
      }
  }
  
  timevar_pop (TV_VARCONST);

  if (flag_detailed_statistics)
    {
      dump_tree_statistics ();
      dump_time_statistics ();
    }
}

/* This is something of the form 'A()()()()()+1' that has turned out to be an
   expr.  Since it was parsed like a type, we need to wade through and fix
   that.  Unfortunately, since operator() is left-associative, we can't use
   tail recursion.  In the above example, TYPE is `A', and DECL is
   `()()()()()'.

   Maybe this shouldn't be recursive, but how often will it actually be
   used?  (jason) */

tree
reparse_absdcl_as_expr (type, decl)
     tree type, decl;
{
  /* do build_functional_cast (type, NULL_TREE) at bottom */
  if (TREE_OPERAND (decl, 0) == NULL_TREE)
    return build_functional_cast (type, NULL_TREE);

  /* recurse */
  decl = reparse_absdcl_as_expr (type, TREE_OPERAND (decl, 0));

  decl = build_x_function_call (decl, NULL_TREE, current_class_ref);

  if (TREE_CODE (decl) == CALL_EXPR
      && (! TREE_TYPE (decl)
          || TREE_CODE (TREE_TYPE (decl)) != VOID_TYPE))
    decl = require_complete_type (decl);

  return decl;
}

/* This is something of the form `int ((int)(int)(int)1)' that has turned
   out to be an expr.  Since it was parsed like a type, we need to wade
   through and fix that.  Since casts are right-associative, we are
   reversing the order, so we don't have to recurse.

   In the above example, DECL is the `(int)(int)(int)', and EXPR is the
   `1'.  */

tree
reparse_absdcl_as_casts (decl, expr)
     tree decl, expr;
{
  tree type;
  int non_void_p = 0;
  
  if (TREE_CODE (expr) == CONSTRUCTOR
      && TREE_TYPE (expr) == 0)
    {
      type = groktypename (TREE_VALUE (CALL_DECLARATOR_PARMS (decl)));
      decl = TREE_OPERAND (decl, 0);

      if (processing_template_decl)
	TREE_TYPE (expr) = type;
      else
	{
	  expr = digest_init (type, expr, (tree *) 0);
	  if (TREE_CODE (type) == ARRAY_TYPE && !COMPLETE_TYPE_P (type))
	    {
	      int failure = complete_array_type (type, expr, 1);
	      my_friendly_assert (!failure, 78);
	    }
	}
    }

  while (decl)
    {
      type = groktypename (TREE_VALUE (CALL_DECLARATOR_PARMS (decl)));
      decl = TREE_OPERAND (decl, 0);
      if (!VOID_TYPE_P (type))
	non_void_p = 1;
      expr = build_c_cast (type, expr);
    }

  if (warn_old_style_cast && ! in_system_header
      && non_void_p && current_lang_name != lang_name_c)
    warning ("use of old-style cast");

  return expr;
}

/* Given plain tree nodes for an expression, build up the full semantics.  */

tree
build_expr_from_tree (t)
     tree t;
{
  if (t == NULL_TREE || t == error_mark_node)
    return t;

  switch (TREE_CODE (t))
    {
    case IDENTIFIER_NODE:
      return do_identifier (t, 0, NULL_TREE);

    case LOOKUP_EXPR:
      if (LOOKUP_EXPR_GLOBAL (t))
	return do_scoped_id (TREE_OPERAND (t, 0), 0);
      else
	return do_identifier (TREE_OPERAND (t, 0), 0, NULL_TREE);

    case TEMPLATE_ID_EXPR:
      return (lookup_template_function
	      (build_expr_from_tree (TREE_OPERAND (t, 0)),
	       build_expr_from_tree (TREE_OPERAND (t, 1))));

    case INDIRECT_REF:
      return build_x_indirect_ref
	(build_expr_from_tree (TREE_OPERAND (t, 0)), "unary *");

    case CAST_EXPR:
      return build_functional_cast
	(TREE_TYPE (t), build_expr_from_tree (TREE_OPERAND (t, 0)));

    case REINTERPRET_CAST_EXPR:
      return build_reinterpret_cast
	(TREE_TYPE (t), build_expr_from_tree (TREE_OPERAND (t, 0)));

    case CONST_CAST_EXPR:
      return build_const_cast
	(TREE_TYPE (t), build_expr_from_tree (TREE_OPERAND (t, 0)));

    case DYNAMIC_CAST_EXPR:
      return build_dynamic_cast
	(TREE_TYPE (t), build_expr_from_tree (TREE_OPERAND (t, 0)));

    case STATIC_CAST_EXPR:
      return build_static_cast
	(TREE_TYPE (t), build_expr_from_tree (TREE_OPERAND (t, 0)));

    case PREDECREMENT_EXPR:
    case PREINCREMENT_EXPR:
    case POSTDECREMENT_EXPR:
    case POSTINCREMENT_EXPR:
    case NEGATE_EXPR:
    case BIT_NOT_EXPR:
    case ABS_EXPR:
    case TRUTH_NOT_EXPR:
    case ADDR_EXPR:
    case CONVERT_EXPR:      /* Unary + */
    case REALPART_EXPR:
    case IMAGPART_EXPR:
      if (TREE_TYPE (t))
	return t;
      return build_x_unary_op (TREE_CODE (t),
			       build_expr_from_tree (TREE_OPERAND (t, 0)));

    case PLUS_EXPR:
    case MINUS_EXPR:
    case MULT_EXPR:
    case TRUNC_DIV_EXPR:
    case CEIL_DIV_EXPR:
    case FLOOR_DIV_EXPR:
    case ROUND_DIV_EXPR:
    case EXACT_DIV_EXPR:
    case BIT_AND_EXPR:
    case BIT_ANDTC_EXPR:
    case BIT_IOR_EXPR:
    case BIT_XOR_EXPR:
    case TRUNC_MOD_EXPR:
    case FLOOR_MOD_EXPR:
    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case TRUTH_AND_EXPR:
    case TRUTH_OR_EXPR:
    case RSHIFT_EXPR:
    case LSHIFT_EXPR:
    case RROTATE_EXPR:
    case LROTATE_EXPR:
    case EQ_EXPR:
    case NE_EXPR:
    case MAX_EXPR:
    case MIN_EXPR:
    case LE_EXPR:
    case GE_EXPR:
    case LT_EXPR:
    case GT_EXPR:
    case MEMBER_REF:
      return build_x_binary_op
	(TREE_CODE (t), 
	 build_expr_from_tree (TREE_OPERAND (t, 0)),
	 build_expr_from_tree (TREE_OPERAND (t, 1)));

    case DOTSTAR_EXPR:
      return build_m_component_ref
	(build_expr_from_tree (TREE_OPERAND (t, 0)),
	 build_expr_from_tree (TREE_OPERAND (t, 1)));

    case SCOPE_REF:
      return build_offset_ref (TREE_OPERAND (t, 0), TREE_OPERAND (t, 1));

    case ARRAY_REF:
      if (TREE_OPERAND (t, 0) == NULL_TREE)
	/* new-type-id */
	return build_nt (ARRAY_REF, NULL_TREE,
			 build_expr_from_tree (TREE_OPERAND (t, 1)));
      return grok_array_decl (build_expr_from_tree (TREE_OPERAND (t, 0)),
			      build_expr_from_tree (TREE_OPERAND (t, 1)));

    case SIZEOF_EXPR:
    case ALIGNOF_EXPR:
      {
	tree r = build_expr_from_tree (TREE_OPERAND (t, 0));
	if (!TYPE_P (r))
	  return TREE_CODE (t) == SIZEOF_EXPR ? expr_sizeof (r) : c_alignof_expr (r);
	else
	  return TREE_CODE (t) == SIZEOF_EXPR ? c_sizeof (r) : c_alignof (r);
      }

    case MODOP_EXPR:
      return build_x_modify_expr
	(build_expr_from_tree (TREE_OPERAND (t, 0)),
	 TREE_CODE (TREE_OPERAND (t, 1)),
	 build_expr_from_tree (TREE_OPERAND (t, 2)));

    case ARROW_EXPR:
      return build_x_arrow
	(build_expr_from_tree (TREE_OPERAND (t, 0)));

    case NEW_EXPR:
      return build_new
	(build_expr_from_tree (TREE_OPERAND (t, 0)),
	 build_expr_from_tree (TREE_OPERAND (t, 1)),
	 build_expr_from_tree (TREE_OPERAND (t, 2)),
	 NEW_EXPR_USE_GLOBAL (t));

    case DELETE_EXPR:
      return delete_sanity
	(build_expr_from_tree (TREE_OPERAND (t, 0)),
	 build_expr_from_tree (TREE_OPERAND (t, 1)),
	 DELETE_EXPR_USE_VEC (t), DELETE_EXPR_USE_GLOBAL (t));

    case COMPOUND_EXPR:
      if (TREE_OPERAND (t, 1) == NULL_TREE)
	return build_x_compound_expr
	  (build_expr_from_tree (TREE_OPERAND (t, 0)));
      else
	abort ();

    case METHOD_CALL_EXPR:
      if (TREE_CODE (TREE_OPERAND (t, 0)) == SCOPE_REF)
	{
	  tree ref = TREE_OPERAND (t, 0);
	  tree name = TREE_OPERAND (ref, 1);
	  
	  if (TREE_CODE (name) == TEMPLATE_ID_EXPR)
	    name = build_nt (TEMPLATE_ID_EXPR,
	                     TREE_OPERAND (name, 0),
	                     build_expr_from_tree (TREE_OPERAND (name, 1)));
	    
	  return build_scoped_method_call
	    (build_expr_from_tree (TREE_OPERAND (t, 1)),
	     build_expr_from_tree (TREE_OPERAND (ref, 0)),
	     name,
	     build_expr_from_tree (TREE_OPERAND (t, 2)));
	}
      else 
	{
	  tree fn = TREE_OPERAND (t, 0);

	  /* We can get a TEMPLATE_ID_EXPR here on code like:

	       x->f<2>();
	      
	     so we must resolve that.  However, we can also get things
	     like a BIT_NOT_EXPR here, when referring to a destructor,
	     and things like that are not correctly resolved by
	     build_expr_from_tree.  So, just use build_expr_from_tree
	     when we really need it.  */
	  if (TREE_CODE (fn) == TEMPLATE_ID_EXPR)
	    fn = lookup_template_function
	      (TREE_OPERAND (fn, 0),
	       build_expr_from_tree (TREE_OPERAND (fn, 1)));

	  return build_method_call
	    (build_expr_from_tree (TREE_OPERAND (t, 1)),
	     fn,
	     build_expr_from_tree (TREE_OPERAND (t, 2)),
	     NULL_TREE, LOOKUP_NORMAL);
	}

    case CALL_EXPR:
      if (TREE_CODE (TREE_OPERAND (t, 0)) == SCOPE_REF)
	{
	  tree ref = TREE_OPERAND (t, 0);
	  tree name = TREE_OPERAND (ref, 1);
	  
	  if (TREE_CODE (name) == TEMPLATE_ID_EXPR)
	    name = build_nt (TEMPLATE_ID_EXPR,
	                     TREE_OPERAND (name, 0),
	                     build_expr_from_tree (TREE_OPERAND (name, 1)));
	    
	  return build_member_call
	    (build_expr_from_tree (TREE_OPERAND (ref, 0)),
	     name,
	     build_expr_from_tree (TREE_OPERAND (t, 1)));
	}
      else
	{
	  tree name = TREE_OPERAND (t, 0);
          tree id;
          tree args = build_expr_from_tree (TREE_OPERAND (t, 1));
          if (args != NULL_TREE && TREE_CODE (name) == LOOKUP_EXPR
              && !LOOKUP_EXPR_GLOBAL (name)
              && TREE_CODE ((id = TREE_OPERAND (name, 0))) == IDENTIFIER_NODE
              && (!current_class_type
                  || !lookup_member (current_class_type, id, 0, 0)))
            {
              /* Do Koenig lookup if there are no class members. */
              name = do_identifier (id, 0, args);
            }
          else if (TREE_CODE (name) == TEMPLATE_ID_EXPR
	      || ! really_overloaded_fn (name))
	    name = build_expr_from_tree (name);
	  return build_x_function_call (name, args, current_class_ref);
	}

    case COND_EXPR:
      return build_x_conditional_expr
	(build_expr_from_tree (TREE_OPERAND (t, 0)),
	 build_expr_from_tree (TREE_OPERAND (t, 1)),
	 build_expr_from_tree (TREE_OPERAND (t, 2)));

    case PSEUDO_DTOR_EXPR:
      return (finish_pseudo_destructor_call_expr 
	      (build_expr_from_tree (TREE_OPERAND (t, 0)),
	       build_expr_from_tree (TREE_OPERAND (t, 1)),
	       build_expr_from_tree (TREE_OPERAND (t, 2))));

    case TREE_LIST:
      {
	tree purpose, value, chain;

	if (t == void_list_node)
	  return t;

	purpose = TREE_PURPOSE (t);
	if (purpose)
	  purpose = build_expr_from_tree (purpose);
	value = TREE_VALUE (t);
	if (value)
	  value = build_expr_from_tree (value);
	chain = TREE_CHAIN (t);
	if (chain && chain != void_type_node)
	  chain = build_expr_from_tree (chain);
	return tree_cons (purpose, value, chain);
      }

    case COMPONENT_REF:
      {
	tree object = build_expr_from_tree (TREE_OPERAND (t, 0));
	tree field = TREE_OPERAND (t, 1);
	
	/* We use a COMPONENT_REF to indicate things of the form `x.b'
	   and `x.A::b'.  We must distinguish between those cases
	   here.  */
	if (TREE_CODE (field) == SCOPE_REF)
	  return build_object_ref (object, 
				   TREE_OPERAND (field, 0),
				   TREE_OPERAND (field, 1));
	else
	  return build_x_component_ref (object, field,
					NULL_TREE, 1);
      }

    case THROW_EXPR:
      return build_throw (build_expr_from_tree (TREE_OPERAND (t, 0)));

    case CONSTRUCTOR:
      {
	tree r;
	tree elts;
	tree type = TREE_TYPE (t);
	bool purpose_p;

	/* digest_init will do the wrong thing if we let it.  */
	if (type && TYPE_PTRMEMFUNC_P (type))
	  return t;

	r = NULL_TREE;
	/* We do not want to process the purpose of aggregate
	   initializers as they are identifier nodes which will be
	   looked up by digest_init.  */
	purpose_p = !(type && IS_AGGR_TYPE (type));
	for (elts = CONSTRUCTOR_ELTS (t); elts; elts = TREE_CHAIN (elts))
	  {
	    tree purpose = TREE_PURPOSE (elts);
	    tree value = TREE_VALUE (elts);
	    
	    if (purpose && purpose_p)
	      purpose = build_expr_from_tree (purpose);
	    value = build_expr_from_tree (value);
	    r = tree_cons (purpose, value, r);
	  }
	
	r = build_nt (CONSTRUCTOR, NULL_TREE, nreverse (r));
	TREE_HAS_CONSTRUCTOR (r) = TREE_HAS_CONSTRUCTOR (t);

	if (type)
	  return digest_init (type, r, 0);
	return r;
      }

    case TYPEID_EXPR:
      if (TYPE_P (TREE_OPERAND (t, 0)))
	return get_typeid (TREE_OPERAND (t, 0));
      return build_typeid (build_expr_from_tree (TREE_OPERAND (t, 0)));

    case VAR_DECL:
      return convert_from_reference (t);

    case VA_ARG_EXPR:
      return build_va_arg (build_expr_from_tree (TREE_OPERAND (t, 0)),
			   TREE_TYPE (t));

    default:
      return t;
    }
}

/* This is something of the form `int (*a)++' that has turned out to be an
   expr.  It was only converted into parse nodes, so we need to go through
   and build up the semantics.  Most of the work is done by
   build_expr_from_tree, above.

   In the above example, TYPE is `int' and DECL is `*a'.  */

tree
reparse_decl_as_expr (type, decl)
     tree type, decl;
{
  decl = build_expr_from_tree (decl);
  if (type)
    return build_functional_cast (type, build_tree_list (NULL_TREE, decl));
  else
    return decl;
}

/* This is something of the form `int (*a)' that has turned out to be a
   decl.  It was only converted into parse nodes, so we need to do the
   checking that make_{pointer,reference}_declarator do.  */

tree
finish_decl_parsing (decl)
     tree decl;
{
  switch (TREE_CODE (decl))
    {
    case IDENTIFIER_NODE:
      return decl;
    case INDIRECT_REF:
      return make_pointer_declarator
	(NULL_TREE, finish_decl_parsing (TREE_OPERAND (decl, 0)));
    case ADDR_EXPR:
      return make_reference_declarator
	(NULL_TREE, finish_decl_parsing (TREE_OPERAND (decl, 0)));
    case BIT_NOT_EXPR:
      TREE_OPERAND (decl, 0) = finish_decl_parsing (TREE_OPERAND (decl, 0));
      return decl;
    case SCOPE_REF:
      push_nested_class (TREE_TYPE (TREE_OPERAND (decl, 0)), 3);
      TREE_COMPLEXITY (decl) = current_class_depth;
      return decl;
    case ARRAY_REF:
      TREE_OPERAND (decl, 0) = finish_decl_parsing (TREE_OPERAND (decl, 0));
      return decl;
    case TREE_LIST:
      /* For attribute handling.  */
      TREE_VALUE (decl) = finish_decl_parsing (TREE_VALUE (decl));
      return decl;
    case TEMPLATE_ID_EXPR:
      return decl;
    default:
      abort ();
      return NULL_TREE;
    }
}

/* Return 1 if root encloses child. */

static int
is_namespace_ancestor (root, child)
     tree root, child;
{
  if (root == child)
    return 1;
  if (root == global_namespace)
    return 1;
  if (child == global_namespace)
    return 0;
  return is_namespace_ancestor (root, CP_DECL_CONTEXT (child));
}
  

/* Return the namespace that is the common ancestor 
   of two given namespaces. */

tree
namespace_ancestor (ns1, ns2)
     tree ns1, ns2;
{
  if (is_namespace_ancestor (ns1, ns2))
    return ns1;
  return namespace_ancestor (CP_DECL_CONTEXT (ns1), ns2);
}

/* Insert used into the using list of user. Set indirect_flag if this
   directive is not directly from the source. Also find the common
   ancestor and let our users know about the new namespace */
static void 
add_using_namespace (user, used, indirect)
     tree user;
     tree used;
     int indirect;
{
  tree t;
  /* Using oneself is a no-op. */
  if (user == used)
    return;
  my_friendly_assert (TREE_CODE (user) == NAMESPACE_DECL, 380);
  my_friendly_assert (TREE_CODE (used) == NAMESPACE_DECL, 380);
  /* Check if we already have this. */
  t = purpose_member (used, DECL_NAMESPACE_USING (user));
  if (t != NULL_TREE)
    {
      if (!indirect)
	/* Promote to direct usage. */
	TREE_INDIRECT_USING (t) = 0;
      return;
    }

  /* Add used to the user's using list. */
  DECL_NAMESPACE_USING (user) 
    = tree_cons (used, namespace_ancestor (user, used), 
		 DECL_NAMESPACE_USING (user));

  TREE_INDIRECT_USING (DECL_NAMESPACE_USING (user)) = indirect;

  /* Add user to the used's users list. */
  DECL_NAMESPACE_USERS (used)
    = tree_cons (user, 0, DECL_NAMESPACE_USERS (used));

  /* Recursively add all namespaces used. */
  for (t = DECL_NAMESPACE_USING (used); t; t = TREE_CHAIN (t))
    /* indirect usage */
    add_using_namespace (user, TREE_PURPOSE (t), 1);

  /* Tell everyone using us about the new used namespaces. */
  for (t = DECL_NAMESPACE_USERS (user); t; t = TREE_CHAIN (t))
    add_using_namespace (TREE_PURPOSE (t), used, 1);
}

/* Combines two sets of overloaded functions into an OVERLOAD chain, removing
   duplicates.  The first list becomes the tail of the result.

   The algorithm is O(n^2).  We could get this down to O(n log n) by
   doing a sort on the addresses of the functions, if that becomes
   necessary.  */

static tree
merge_functions (s1, s2)
     tree s1;
     tree s2;
{
  for (; s2; s2 = OVL_NEXT (s2))
    {
      tree fn2 = OVL_CURRENT (s2);
      tree fns1;

      for (fns1 = s1; fns1; fns1 = OVL_NEXT (fns1))
	{
	  tree fn1 = OVL_CURRENT (fns1);

	  /* If the function from S2 is already in S1, there is no
	     need to add it again.  For `extern "C"' functions, we
	     might have two FUNCTION_DECLs for the same function, in
	     different namespaces; again, we only need one of them.  */
	  if (fn1 == fn2 
	      || (DECL_EXTERN_C_P (fn1) && DECL_EXTERN_C_P (fn2)
		  && DECL_NAME (fn1) == DECL_NAME (fn2)))
	    break;
	}
      
      /* If we exhausted all of the functions in S1, FN2 is new.  */
      if (!fns1)
	s1 = build_overload (fn2, s1);
    }
  return s1;
}

/* This should return an error not all definitions define functions.
   It is not an error if we find two functions with exactly the
   same signature, only if these are selected in overload resolution.
   old is the current set of bindings, new the freshly-found binding.
   XXX Do we want to give *all* candidates in case of ambiguity?
   XXX In what way should I treat extern declarations?
   XXX I don't want to repeat the entire duplicate_decls here */

static tree
ambiguous_decl (name, old, new, flags)
     tree name;
     tree old;
     tree new;
     int flags;
{
  tree val, type;
  my_friendly_assert (old != NULL_TREE, 393);
  /* Copy the value. */
  val = BINDING_VALUE (new);
  if (val)
    switch (TREE_CODE (val))
      {
      case TEMPLATE_DECL:
        /* If we expect types or namespaces, and not templates,
           or this is not a template class. */
        if (LOOKUP_QUALIFIERS_ONLY (flags)
            && !DECL_CLASS_TEMPLATE_P (val))
          val = NULL_TREE;
        break;
      case TYPE_DECL:
        if (LOOKUP_NAMESPACES_ONLY (flags))
          val = NULL_TREE;
        break;
      case NAMESPACE_DECL:
        if (LOOKUP_TYPES_ONLY (flags))
          val = NULL_TREE;
        break;
      case FUNCTION_DECL:
        /* Ignore built-in functions that are still anticipated.  */
        if (LOOKUP_QUALIFIERS_ONLY (flags) || DECL_ANTICIPATED (val))
          val = NULL_TREE;
        break;
      default:
        if (LOOKUP_QUALIFIERS_ONLY (flags))
          val = NULL_TREE;
      }
        
  if (!BINDING_VALUE (old))
    BINDING_VALUE (old) = val;
  else if (val && val != BINDING_VALUE (old))
    {
      if (is_overloaded_fn (BINDING_VALUE (old)) 
	  && is_overloaded_fn (val))
	{
	  BINDING_VALUE (old) = merge_functions (BINDING_VALUE (old),
						 val);
	}
      else
	{
	  /* Some declarations are functions, some are not. */
          if (flags & LOOKUP_COMPLAIN)
            {
	      /* If we've already given this error for this lookup,
		 BINDING_VALUE (old) is error_mark_node, so let's not
		 repeat ourselves.  */
	      if (BINDING_VALUE (old) != error_mark_node)
		{
		  error ("use of `%D' is ambiguous", name);
		  cp_error_at ("  first declared as `%#D' here",
			       BINDING_VALUE (old));
		}
              cp_error_at ("  also declared as `%#D' here", val);
            }
	  BINDING_VALUE (old) = error_mark_node;
	}
    }
  /* ... and copy the type. */
  type = BINDING_TYPE (new);
  if (LOOKUP_NAMESPACES_ONLY (flags))
    type = NULL_TREE;
  if (!BINDING_TYPE (old))
    BINDING_TYPE (old) = type;
  else if (type && BINDING_TYPE (old) != type)
    {
      if (flags & LOOKUP_COMPLAIN)
        {
          error ("`%D' denotes an ambiguous type",name);
          cp_error_at ("  first type here", BINDING_TYPE (old));
          cp_error_at ("  other type here", type);
        }
    }
  return old;
}

/* Subroutine of unualified_namespace_lookup:
   Add the bindings of NAME in used namespaces to VAL.
   We are currently looking for names in namespace SCOPE, so we
   look through USINGS for using-directives of namespaces
   which have SCOPE as a common ancestor with the current scope.
   Returns zero on errors. */

int
lookup_using_namespace (name, val, usings, scope, flags, spacesp)
     tree name, val, usings, scope;
     int flags;
     tree *spacesp;
{
  tree iter;
  tree val1;
  /* Iterate over all used namespaces in current, searching for using
     directives of scope. */
  for (iter = usings; iter; iter = TREE_CHAIN (iter))
    if (TREE_VALUE (iter) == scope)
      {
	if (spacesp)
	  *spacesp = tree_cons (TREE_PURPOSE (iter), NULL_TREE,
				*spacesp);
	val1 = binding_for_name (name, TREE_PURPOSE (iter));
	/* Resolve ambiguities. */
	val = ambiguous_decl (name, val, val1, flags);
      }
  return BINDING_VALUE (val) != error_mark_node;
}

/* [namespace.qual]
   Accepts the NAME to lookup and its qualifying SCOPE.
   Returns the name/type pair found into the CPLUS_BINDING RESULT,
   or 0 on error. */

int
qualified_lookup_using_namespace (name, scope, result, flags)
     tree name;
     tree scope;
     tree result;
     int flags;
{
  /* Maintain a list of namespaces visited... */
  tree seen = NULL_TREE;
  /* ... and a list of namespace yet to see. */
  tree todo = NULL_TREE;
  tree usings;
  /* Look through namespace aliases.  */
  scope = ORIGINAL_NAMESPACE (scope);
  while (scope && (result != error_mark_node))
    {
      seen = tree_cons (scope, NULL_TREE, seen);
      result = ambiguous_decl (name, result,
                               binding_for_name (name, scope), flags);
      if (!BINDING_VALUE (result) && !BINDING_TYPE (result))
	/* Consider using directives. */
	for (usings = DECL_NAMESPACE_USING (scope); usings;
	     usings = TREE_CHAIN (usings))
	  /* If this was a real directive, and we have not seen it. */
	  if (!TREE_INDIRECT_USING (usings)
	      && !purpose_member (TREE_PURPOSE (usings), seen))
	    todo = tree_cons (TREE_PURPOSE (usings), NULL_TREE, todo);
      if (todo)
	{
	  scope = TREE_PURPOSE (todo);
	  todo = TREE_CHAIN (todo);
	}
      else
	scope = NULL_TREE; /* If there never was a todo list. */
    }
  return result != error_mark_node;
}

/* [namespace.memdef]/2 */

/* Set the context of a declaration to scope. Complain if we are not
   outside scope. */

void
set_decl_namespace (decl, scope, friendp)
     tree decl;
     tree scope;
     int friendp;
{
  tree old;
  
  /* Get rid of namespace aliases. */
  scope = ORIGINAL_NAMESPACE (scope);
  
  /* It is ok for friends to be qualified in parallel space.  */
  if (!friendp && !is_namespace_ancestor (current_namespace, scope))
    error ("declaration of `%D' not in a namespace surrounding `%D'",
	      decl, scope);
  DECL_CONTEXT (decl) = FROB_CONTEXT (scope);
  if (scope != current_namespace)
    {
      /* See whether this has been declared in the namespace. */
      old = namespace_binding (DECL_NAME (decl), scope);
      if (!old)
	/* No old declaration at all. */
	goto complain;
      /* A template can be explicitly specialized in any namespace.  */
      if (processing_explicit_instantiation)
	return;
      if (!is_overloaded_fn (decl))
	/* Don't compare non-function decls with decls_match here,
	   since it can't check for the correct constness at this
	   point. pushdecl will find those errors later.  */
	return;
      /* Since decl is a function, old should contain a function decl. */
      if (!is_overloaded_fn (old))
	goto complain;
      if (processing_template_decl || processing_specialization)
	/* We have not yet called push_template_decl to turn the
	   FUNCTION_DECL into a TEMPLATE_DECL, so the declarations
	   won't match.  But, we'll check later, when we construct the
	   template.  */
	return;
      for (; old; old = OVL_NEXT (old))
	if (decls_match (decl, OVL_CURRENT (old)))
	  return;
    }
  else
    return;
 complain:
  error ("`%D' should have been declared inside `%D'",
	    decl, scope);
} 

/* Compute the namespace where a declaration is defined. */

static tree
decl_namespace (decl)
     tree decl;
{
  if (TYPE_P (decl))
    decl = TYPE_STUB_DECL (decl);
  while (DECL_CONTEXT (decl))
    {
      decl = DECL_CONTEXT (decl);
      if (TREE_CODE (decl) == NAMESPACE_DECL)
	return decl;
      if (TYPE_P (decl))
	decl = TYPE_STUB_DECL (decl);
      my_friendly_assert (DECL_P (decl), 390);
    }

  return global_namespace;
}

/* Return the namespace where the current declaration is declared. */

tree
current_decl_namespace ()
{
  tree result;
  /* If we have been pushed into a different namespace, use it. */
  if (decl_namespace_list)
    return TREE_PURPOSE (decl_namespace_list);

  if (current_class_type)
    result = decl_namespace (TYPE_STUB_DECL (current_class_type));
  else if (current_function_decl)
    result = decl_namespace (current_function_decl);
  else 
    result = current_namespace;
  return result;
}

/* Temporarily set the namespace for the current declaration. */

void
push_decl_namespace (decl)
     tree decl;
{
  if (TREE_CODE (decl) != NAMESPACE_DECL)
    decl = decl_namespace (decl);
  decl_namespace_list = tree_cons (ORIGINAL_NAMESPACE (decl),
                                   NULL_TREE, decl_namespace_list);
}

void
pop_decl_namespace ()
{
  decl_namespace_list = TREE_CHAIN (decl_namespace_list);
}

/* Enter a class or namespace scope. */

void
push_scope (t)
     tree t;
{
  if (TREE_CODE (t) == NAMESPACE_DECL)
    push_decl_namespace (t);
  else
    pushclass (t, 2);
}

/* Leave scope pushed by push_scope. */

void
pop_scope (t)
     tree t;
{
  if (TREE_CODE (t) == NAMESPACE_DECL)
    pop_decl_namespace ();
  else
    popclass ();
}

/* [basic.lookup.koenig] */
/* A non-zero return value in the functions below indicates an error.
   All nodes allocated in the procedure are on the scratch obstack. */

struct arg_lookup
{
  tree name;
  tree namespaces;
  tree classes;
  tree functions;
};

static int arg_assoc         PARAMS ((struct arg_lookup*, tree));
static int arg_assoc_args    PARAMS ((struct arg_lookup*, tree));
static int arg_assoc_type    PARAMS ((struct arg_lookup*, tree));
static int add_function      PARAMS ((struct arg_lookup *, tree));
static int arg_assoc_namespace PARAMS ((struct arg_lookup *, tree));
static int arg_assoc_class   PARAMS ((struct arg_lookup *, tree));
static int arg_assoc_template_arg PARAMS ((struct arg_lookup*, tree));

/* Add a function to the lookup structure.
   Returns 1 on error.  */

static int
add_function (k, fn)
     struct arg_lookup *k;
     tree fn;
{
  /* We used to check here to see if the function was already in the list,
     but that's O(n^2), which is just too expensive for function lookup.
     Now we deal with the occasional duplicate in joust.  In doing this, we
     assume that the number of duplicates will be small compared to the
     total number of functions being compared, which should usually be the
     case.  */

  /* We must find only functions, or exactly one non-function. */
  if (!k->functions) 
    k->functions = fn;
  else if (is_overloaded_fn (k->functions) && is_overloaded_fn (fn))
    k->functions = build_overload (fn, k->functions);
  else
    {
      tree f1 = OVL_CURRENT (k->functions);
      tree f2 = fn;
      if (is_overloaded_fn (f1))
	{
	  fn = f1; f1 = f2; f2 = fn;
	}
      cp_error_at ("`%D' is not a function,", f1);
      cp_error_at ("  conflict with `%D'", f2);
      error ("  in call to `%D'", k->name);
      return 1;
    }

  return 0;
}

/* Add functions of a namespace to the lookup structure.
   Returns 1 on error.  */

static int
arg_assoc_namespace (k, scope)
     struct arg_lookup *k;
     tree scope;
{
  tree value;

  if (purpose_member (scope, k->namespaces))
    return 0;
  k->namespaces = tree_cons (scope, NULL_TREE, k->namespaces);
  
  value = namespace_binding (k->name, scope);
  if (!value)
    return 0;

  for (; value; value = OVL_NEXT (value))
    if (add_function (k, OVL_CURRENT (value)))
      return 1;
  
  return 0;
}

/* Adds everything associated with a template argument to the lookup
   structure.  Returns 1 on error.  */

static int
arg_assoc_template_arg (k, arg)
     struct arg_lookup* k;
     tree arg;
{
  /* [basic.lookup.koenig]

     If T is a template-id, its associated namespaces and classes are
     ... the namespaces and classes associated with the types of the
     template arguments provided for template type parameters
     (excluding template template parameters); the namespaces in which
     any template template arguments are defined; and the classes in
     which any member templates used as template template arguments
     are defined.  [Note: non-type template arguments do not
     contribute to the set of associated namespaces.  ]  */

  /* Consider first template template arguments.  */
  if (TREE_CODE (arg) == TEMPLATE_TEMPLATE_PARM
      || TREE_CODE (arg) == UNBOUND_CLASS_TEMPLATE)
    return 0;
  else if (TREE_CODE (arg) == TEMPLATE_DECL)
    {
      tree ctx = CP_DECL_CONTEXT (arg);

      /* It's not a member template.  */
      if (TREE_CODE (ctx) == NAMESPACE_DECL)
        return arg_assoc_namespace (k, ctx);
      /* Otherwise, it must be member template.  */
      else 
        return arg_assoc_class (k, ctx);
    }
  /* It's not a template template argument, but it is a type template
     argument.  */
  else if (TYPE_P (arg))
    return arg_assoc_type (k, arg);
  /* It's a non-type template argument.  */
  else
    return 0;
}

/* Adds everything associated with class to the lookup structure.
   Returns 1 on error.  */

static int
arg_assoc_class (k, type)
     struct arg_lookup* k;
     tree type;
{
  tree list, friends, context;
  int i;
  
  /* Backend build structures, such as __builtin_va_list, aren't
     affected by all this.  */
  if (!CLASS_TYPE_P (type))
    return 0;

  if (purpose_member (type, k->classes))
    return 0;
  k->classes = tree_cons (type, NULL_TREE, k->classes);
  
  context = decl_namespace (TYPE_MAIN_DECL (type));
  if (arg_assoc_namespace (k, context))
    return 1;
  
  /* Process baseclasses. */
  for (i = 0; i < CLASSTYPE_N_BASECLASSES (type); i++)
    if (arg_assoc_class (k, TYPE_BINFO_BASETYPE (type, i)))
      return 1;
  
  /* Process friends. */
  for (list = DECL_FRIENDLIST (TYPE_MAIN_DECL (type)); list; 
       list = TREE_CHAIN (list))
    if (k->name == TREE_PURPOSE (list))
      for (friends = TREE_VALUE (list); friends; 
	   friends = TREE_CHAIN (friends))
	/* Only interested in global functions with potentially hidden
           (i.e. unqualified) declarations. */
	if (TREE_PURPOSE (friends) == error_mark_node && TREE_VALUE (friends)
	    && decl_namespace (TREE_VALUE (friends)) == context)
	  if (add_function (k, TREE_VALUE (friends)))
	    return 1;

  /* Process template arguments.  */
  if (CLASSTYPE_TEMPLATE_INFO (type))
    {
      list = INNERMOST_TEMPLATE_ARGS (CLASSTYPE_TI_ARGS (type));
      for (i = 0; i < TREE_VEC_LENGTH (list); ++i) 
        arg_assoc_template_arg (k, TREE_VEC_ELT (list, i));
    }

  return 0;
}

/* Adds everything associated with a given type.
   Returns 1 on error.  */

static int
arg_assoc_type (k, type)
     struct arg_lookup *k;
     tree type;
{
  switch (TREE_CODE (type))
    {
    case VOID_TYPE:
    case INTEGER_TYPE:
    case REAL_TYPE:
    case COMPLEX_TYPE:
    case VECTOR_TYPE:
    case CHAR_TYPE:
    case BOOLEAN_TYPE:
      return 0;
    case RECORD_TYPE:
      if (TYPE_PTRMEMFUNC_P (type))
	return arg_assoc_type (k, TYPE_PTRMEMFUNC_FN_TYPE (type));
      return arg_assoc_class (k, type);
    case POINTER_TYPE:
    case REFERENCE_TYPE:
    case ARRAY_TYPE:
      return arg_assoc_type (k, TREE_TYPE (type));
    case UNION_TYPE:
    case ENUMERAL_TYPE:
      return arg_assoc_namespace (k, decl_namespace (TYPE_MAIN_DECL (type)));
    case OFFSET_TYPE:
      /* Pointer to member: associate class type and value type. */
      if (arg_assoc_type (k, TYPE_OFFSET_BASETYPE (type)))
	return 1;
      return arg_assoc_type (k, TREE_TYPE (type));
    case METHOD_TYPE:
      /* The basetype is referenced in the first arg type, so just
	 fall through.  */
    case FUNCTION_TYPE:
      /* Associate the parameter types. */
      if (arg_assoc_args (k, TYPE_ARG_TYPES (type)))
	return 1;
      /* Associate the return type. */
      return arg_assoc_type (k, TREE_TYPE (type));
    case TEMPLATE_TYPE_PARM:
    case BOUND_TEMPLATE_TEMPLATE_PARM:
      return 0;
    case TYPENAME_TYPE:
      return 0;
    case LANG_TYPE:
      if (type == unknown_type_node)
	return 0;
      /* else fall through */
    default:
      abort ();
    }
  return 0;
}

/* Adds everything associated with arguments.  Returns 1 on error.  */

static int
arg_assoc_args (k, args)
     struct arg_lookup* k;
     tree args;
{
  for (; args; args = TREE_CHAIN (args))
    if (arg_assoc (k, TREE_VALUE (args)))
      return 1;
  return 0;
}

/* Adds everything associated with a given tree_node.  Returns 1 on error.  */

static int
arg_assoc (k, n)
     struct arg_lookup* k;
     tree n;
{
  if (n == error_mark_node)
    return 0;

  if (TYPE_P (n))
    return arg_assoc_type (k, n);

  if (! type_unknown_p (n))
    return arg_assoc_type (k, TREE_TYPE (n));

  if (TREE_CODE (n) == ADDR_EXPR)
    n = TREE_OPERAND (n, 0);
  if (TREE_CODE (n) == COMPONENT_REF)
    n = TREE_OPERAND (n, 1);
  if (TREE_CODE (n) == OFFSET_REF)
    n = TREE_OPERAND (n, 1);
  while (TREE_CODE (n) == TREE_LIST)
    n = TREE_VALUE (n);

  if (TREE_CODE (n) == FUNCTION_DECL)
    return arg_assoc_type (k, TREE_TYPE (n));
  if (TREE_CODE (n) == TEMPLATE_ID_EXPR)
    {
      /* [basic.lookup.koenig]

	 If T is a template-id, its associated namespaces and classes
	 are the namespace in which the template is defined; for
	 member templates, the member template's class...  */
      tree template = TREE_OPERAND (n, 0);
      tree args = TREE_OPERAND (n, 1);
      tree ctx;
      tree arg;

      if (TREE_CODE (template) == COMPONENT_REF)
        template = TREE_OPERAND (template, 1);
      
      /* First, the template.  There may actually be more than one if
	 this is an overloaded function template.  But, in that case,
	 we only need the first; all the functions will be in the same
	 namespace.  */
      template = OVL_CURRENT (template);

      ctx = CP_DECL_CONTEXT (template);
       
      if (TREE_CODE (ctx) == NAMESPACE_DECL)
	{
	  if (arg_assoc_namespace (k, ctx) == 1)
	    return 1;
	}
      /* It must be a member template.  */
      else if (arg_assoc_class (k, ctx) == 1)
	return 1;

      /* Now the arguments.  */
      for (arg = args; arg != NULL_TREE; arg = TREE_CHAIN (arg))
	if (arg_assoc_template_arg (k, TREE_VALUE (arg)) == 1)
	  return 1;
    }
  else
    {
      my_friendly_assert (TREE_CODE (n) == OVERLOAD, 980715);
      
      for (; n; n = OVL_CHAIN (n))
	if (arg_assoc_type (k, TREE_TYPE (OVL_FUNCTION (n))))
	  return 1;
    }

  return 0;
}

/* Performs Koenig lookup depending on arguments, where fns
   are the functions found in normal lookup. */

tree
lookup_arg_dependent (name, fns, args)
     tree name;
     tree fns;
     tree args;
{
  struct arg_lookup k;
  tree fn = NULL_TREE;

  k.name = name;
  k.functions = fns;
  k.classes = NULL_TREE;

  /* Note that we've already looked at some namespaces during normal
     unqualified lookup, unless we found a decl in function scope.  */
  if (fns)
    fn = OVL_CURRENT (fns);
  if (fn && TREE_CODE (fn) == FUNCTION_DECL && DECL_LOCAL_FUNCTION_P (fn))
    k.namespaces = NULL_TREE;
  else
    unqualified_namespace_lookup (name, 0, &k.namespaces);

  arg_assoc_args (&k, args);
  return k.functions;
}

/* Process a namespace-alias declaration. */

void
do_namespace_alias (alias, namespace)
     tree alias, namespace;
{
  if (TREE_CODE (namespace) != NAMESPACE_DECL)
    {
      /* The parser did not find it, so it's not there. */
      error ("unknown namespace `%D'", namespace);
      return;
    }

  namespace = ORIGINAL_NAMESPACE (namespace);

  /* Build the alias. */
  alias = build_lang_decl (NAMESPACE_DECL, alias, void_type_node);     
  DECL_NAMESPACE_ALIAS (alias) = namespace;
  pushdecl (alias);
}

/* Check a non-member using-declaration. Return the name and scope
   being used, and the USING_DECL, or NULL_TREE on failure. */

static tree
validate_nonmember_using_decl (decl, scope, name)
     tree decl;
     tree *scope;
     tree *name;
{
  if (TREE_CODE (decl) == SCOPE_REF)
    {
      *scope = TREE_OPERAND (decl, 0);
      *name = TREE_OPERAND (decl, 1);

      if (!processing_template_decl)
        {
          /* [namespace.udecl]
             A using-declaration for a class member shall be a
             member-declaration.  */
          if(TREE_CODE (*scope) != NAMESPACE_DECL)
            {
              if (TYPE_P (*scope))
                error ("`%T' is not a namespace", *scope);
              else
                error ("`%D' is not a namespace", *scope);
              return NULL_TREE;
            }
          
          /* 7.3.3/5
             A using-declaration shall not name a template-id.  */
          if (TREE_CODE (*name) == TEMPLATE_ID_EXPR)
            {
              *name = TREE_OPERAND (*name, 0);
              error ("a using-declaration cannot specify a template-id.  Try `using %D'", *name);
              return NULL_TREE;
            }
        }
    }
  else if (TREE_CODE (decl) == IDENTIFIER_NODE
           || TREE_CODE (decl) == TYPE_DECL
	   || TREE_CODE (decl) == TEMPLATE_DECL)
    {
      *scope = global_namespace;
      *name = decl;
    }
  else if (TREE_CODE (decl) == NAMESPACE_DECL)
    {
      error ("namespace `%D' not allowed in using-declaration", decl);
      return NULL_TREE;
    }
  else
    abort ();
  if (DECL_P (*name))
    *name = DECL_NAME (*name);
  /* Make a USING_DECL. */
  return push_using_decl (*scope, *name);
}

/* Process local and global using-declarations. */

static void
do_nonmember_using_decl (scope, name, oldval, oldtype, newval, newtype)
     tree scope, name;
     tree oldval, oldtype;
     tree *newval, *newtype;
{
  tree decls;

  *newval = *newtype = NULL_TREE;
  decls = make_node (CPLUS_BINDING);
  if (!qualified_lookup_using_namespace (name, scope, decls, 0))
    /* Lookup error */
    return;

  if (!BINDING_VALUE (decls) && !BINDING_TYPE (decls))
    {
      error ("`%D' not declared", name);
      return;
    }

  /* Check for using functions. */
  if (BINDING_VALUE (decls) && is_overloaded_fn (BINDING_VALUE (decls)))
    {
      tree tmp, tmp1;

      if (oldval && !is_overloaded_fn (oldval))
	{
	  duplicate_decls (OVL_CURRENT (BINDING_VALUE (decls)), oldval);
	  oldval = NULL_TREE;
	}

      *newval = oldval;
      for (tmp = BINDING_VALUE (decls); tmp; tmp = OVL_NEXT (tmp))
	{
	  tree new_fn = OVL_CURRENT (tmp);

	  /* [namespace.udecl]

	     If a function declaration in namespace scope or block
	     scope has the same name and the same parameter types as a
	     function introduced by a using declaration the program is
	     ill-formed.  */
	  for (tmp1 = oldval; tmp1; tmp1 = OVL_NEXT (tmp1))
	    {
	      tree old_fn = OVL_CURRENT (tmp1);

              if (new_fn == old_fn)
                /* The function already exists in the current namespace.  */
                break;
	      else if (OVL_USED (tmp1))
	        continue; /* this is a using decl */
	      else if (compparms (TYPE_ARG_TYPES (TREE_TYPE (new_fn)),
		  		  TYPE_ARG_TYPES (TREE_TYPE (old_fn))))
		{
                  /* If this using declaration introduces a function
                     recognized as a built-in, no longer mark it as
                     anticipated in this scope.  */
                  if (DECL_ANTICIPATED (old_fn))
                    {
                      DECL_ANTICIPATED (old_fn) = 0;
                      break;
                    }

	          /* There was already a non-using declaration in
		     this scope with the same parameter types. If both
	             are the same extern "C" functions, that's ok.  */
                  if (!decls_match (new_fn, old_fn))
    	            error ("`%D' is already declared in this scope", name);
		  break;
		}
	    }

	  /* If we broke out of the loop, there's no reason to add
	     this function to the using declarations for this
	     scope.  */
	  if (tmp1)
	    continue;
	    
	  *newval = build_overload (OVL_CURRENT (tmp), *newval);
	  if (TREE_CODE (*newval) != OVERLOAD)
	    *newval = ovl_cons (*newval, NULL_TREE);
	  OVL_USED (*newval) = 1;
	}
    }
  else 
    {
      *newval = BINDING_VALUE (decls);
      if (oldval)
	duplicate_decls (*newval, oldval);
    } 

  *newtype = BINDING_TYPE (decls);
  if (oldtype && *newtype && oldtype != *newtype)
    {
      error ("using declaration `%D' introduced ambiguous type `%T'",
		name, oldtype);
      return;
    }
}

/* Process a using-declaration not appearing in class or local scope. */

void
do_toplevel_using_decl (decl)
     tree decl;
{
  tree scope, name, binding;
  tree oldval, oldtype, newval, newtype;

  decl = validate_nonmember_using_decl (decl, &scope, &name);
  if (decl == NULL_TREE)
    return;
  
  binding = binding_for_name (name, current_namespace);

  oldval = BINDING_VALUE (binding);
  oldtype = BINDING_TYPE (binding);

  do_nonmember_using_decl (scope, name, oldval, oldtype, &newval, &newtype);

  /* Copy declarations found. */
  if (newval)
    BINDING_VALUE (binding) = newval;
  if (newtype)
    BINDING_TYPE (binding) = newtype;
  return;
}

/* Process a using-declaration at function scope.  */

void
do_local_using_decl (decl)
     tree decl;
{
  tree scope, name;
  tree oldval, oldtype, newval, newtype;

  decl = validate_nonmember_using_decl (decl, &scope, &name);
  if (decl == NULL_TREE)
    return;

  if (building_stmt_tree ()
      && at_function_scope_p ())
    add_decl_stmt (decl);

  oldval = lookup_name_current_level (name);
  oldtype = lookup_type_current_level (name);

  do_nonmember_using_decl (scope, name, oldval, oldtype, &newval, &newtype);

  if (newval)
    {
      if (is_overloaded_fn (newval))
	{
	  tree fn, term;

	  /* We only need to push declarations for those functions
	     that were not already bound in the current level.
	     The old value might be NULL_TREE, it might be a single
	     function, or an OVERLOAD.  */
	  if (oldval && TREE_CODE (oldval) == OVERLOAD)
	    term = OVL_FUNCTION (oldval);
	  else
	    term = oldval;
	  for (fn = newval; fn && OVL_CURRENT (fn) != term; 
	       fn = OVL_NEXT (fn))
	    push_overloaded_decl (OVL_CURRENT (fn), 
				  PUSH_LOCAL | PUSH_USING);
	}
      else
	push_local_binding (name, newval, PUSH_USING);
    }
  if (newtype)
    set_identifier_type_value (name, newtype);
}

tree
do_class_using_decl (decl)
     tree decl;
{
  tree name, value;

  if (TREE_CODE (decl) != SCOPE_REF
      || !TYPE_P (TREE_OPERAND (decl, 0)))
    {
      error ("using-declaration for non-member at class scope");
      return NULL_TREE;
    }
  name = TREE_OPERAND (decl, 1);
  if (TREE_CODE (name) == BIT_NOT_EXPR)
    {
      error ("using-declaration for destructor");
      return NULL_TREE;
    }
  else if (TREE_CODE (name) == TEMPLATE_ID_EXPR)
    {
      name = TREE_OPERAND (name, 0);
      error ("a using-declaration cannot specify a template-id.  Try  `using %T::%D'", TREE_OPERAND (decl, 0), name);
      return NULL_TREE;
    }
  if (TREE_CODE (name) == TYPE_DECL || TREE_CODE (name) == TEMPLATE_DECL)
    name = DECL_NAME (name);

  my_friendly_assert (TREE_CODE (name) == IDENTIFIER_NODE, 980716);

  value = build_lang_decl (USING_DECL, name, void_type_node);
  DECL_INITIAL (value) = TREE_OPERAND (decl, 0);
  return value;
}

/* Process a using-directive. */

void
do_using_directive (namespace)
     tree namespace;
{
  if (building_stmt_tree ())
    add_stmt (build_stmt (USING_STMT, namespace));
  
  /* using namespace A::B::C; */
  if (TREE_CODE (namespace) == SCOPE_REF)
      namespace = TREE_OPERAND (namespace, 1);
  if (TREE_CODE (namespace) == IDENTIFIER_NODE)
    {
      /* Lookup in lexer did not find a namespace. */
      if (!processing_template_decl)
	error ("namespace `%T' undeclared", namespace);
      return;
    }
  if (TREE_CODE (namespace) != NAMESPACE_DECL)
    {
      if (!processing_template_decl)
	error ("`%T' is not a namespace", namespace);
      return;
    }
  namespace = ORIGINAL_NAMESPACE (namespace);
  if (!toplevel_bindings_p ())
    push_using_directive (namespace);
  else
    /* direct usage */
    add_using_namespace (current_namespace, namespace, 0);
}

void
check_default_args (x)
     tree x;
{
  tree arg = TYPE_ARG_TYPES (TREE_TYPE (x));
  int saw_def = 0, i = 0 - (TREE_CODE (TREE_TYPE (x)) == METHOD_TYPE);
  for (; arg && arg != void_list_node; arg = TREE_CHAIN (arg), ++i)
    {
      if (TREE_PURPOSE (arg))
	saw_def = 1;
      else if (saw_def)
	{
	  cp_error_at ("default argument missing for parameter %P of `%+#D'",
		       i, x);
	  break;
	}
    }
}

void
mark_used (decl)
     tree decl;
{
  TREE_USED (decl) = 1;
  if (processing_template_decl)
    return;
  assemble_external (decl);

  /* Is it a synthesized method that needs to be synthesized?  */
  if (TREE_CODE (decl) == FUNCTION_DECL
      && DECL_NONSTATIC_MEMBER_FUNCTION_P (decl)
      && DECL_ARTIFICIAL (decl) 
      && ! DECL_INITIAL (decl)
      /* Kludge: don't synthesize for default args.  */
      && current_function_decl)
    {
      synthesize_method (decl);
      /* If we've already synthesized the method we don't need to
	 instantiate it, so we can return right away.  */
      return;
    }

  /* If this is a function or variable that is an instance of some
     template, we now know that we will need to actually do the
     instantiation. We check that DECL is not an explicit
     instantiation because that is not checked in instantiate_decl.  */
  if ((DECL_NON_THUNK_FUNCTION_P (decl) || TREE_CODE (decl) == VAR_DECL)
      && DECL_LANG_SPECIFIC (decl) && DECL_TEMPLATE_INFO (decl)
      && (!DECL_EXPLICIT_INSTANTIATION (decl)
	  || (TREE_CODE (decl) == FUNCTION_DECL && DECL_INLINE (decl))))
    instantiate_decl (decl, /*defer_ok=*/1);
}

/* Helper function for class_head_decl and class_head_defn
   nonterminals. AGGR is the class, union or struct tag. SCOPE is the
   explicit scope used (NULL for no scope resolution). ID is the
   name. DEFN_P is true, if this is a definition of the class and
   NEW_TYPE_P is set to non-zero, if we push into the scope containing
   the to be defined aggregate.
   
   Return a TYPE_DECL for the type declared by ID in SCOPE.  */

tree
handle_class_head (aggr, scope, id, defn_p, new_type_p)
     tree aggr, scope, id;
     int defn_p;
     int *new_type_p;
{
  tree decl = NULL_TREE;
  tree current = current_scope ();
  bool xrefd_p = false;
  
  if (current == NULL_TREE)
    current = current_namespace;

  *new_type_p = 0;
  
  if (scope)
    {
      if (TREE_CODE (id) == TYPE_DECL)
	/* We must bash typedefs back to the main decl of the
       	   type. Otherwise we become confused about scopes.  */
	decl = TYPE_MAIN_DECL (TREE_TYPE (id));
      else if (DECL_CLASS_TEMPLATE_P (id))
	decl = DECL_TEMPLATE_RESULT (id);
      else
	{
	  if (TYPE_P (scope))
	    {
	      /* According to the suggested resolution of core issue
	     	 180, 'typename' is assumed after a class-key.  */
	      decl = make_typename_type (scope, id, tf_error);
	      if (decl != error_mark_node)
		decl = TYPE_MAIN_DECL (decl);
	      else
		decl = NULL_TREE;
	    }
	  else if (scope == current)
	    {
	      /* We've been given AGGR SCOPE::ID, when we're already
             	 inside SCOPE.  Be nice about it.  */
	      if (pedantic)
		pedwarn ("extra qualification `%T::' on member `%D' ignored",
			 scope, id);
	    }
	  else
	    error ("`%T' does not have a class or union named `%D'",
		   scope, id);
	}
    }
  
  if (!decl)
    {
      decl = TYPE_MAIN_DECL (xref_tag (aggr, id, !defn_p));
      xrefd_p = true;
    }

  if (!TYPE_BINFO (TREE_TYPE (decl)))
    {
      error ("`%T' is not a class or union type", decl);
      return error_mark_node;
    }
  
  if (defn_p)
    {
      /* For a definition, we want to enter the containing scope
	 before looking up any base classes etc. Only do so, if this
	 is different to the current scope.  */
      tree context = CP_DECL_CONTEXT (decl);

      *new_type_p = (current != context
		     && TREE_CODE (context) != TEMPLATE_TYPE_PARM
		     && TREE_CODE (context) != BOUND_TEMPLATE_TEMPLATE_PARM);
      if (*new_type_p)
	push_scope (context);

      if (TREE_CODE (TREE_TYPE (decl)) == RECORD_TYPE)
	/* It is legal to define a class with a different class key,
	   and this changes the default member access.  */
	CLASSTYPE_DECLARED_CLASS (TREE_TYPE (decl))
	  = aggr == class_type_node;
	
      if (!xrefd_p && PROCESSING_REAL_TEMPLATE_DECL_P ())
	decl = push_template_decl (decl);
    }

  return decl;
}

/* Initialize decl2.c.  */

void
init_decl2 ()
{
  ggc_add_tree_varray_root (&deferred_fns, 1);
  ggc_add_tree_varray_root (&pending_statics, 1);
  ggc_add_tree_varray_root (&ssdf_decls, 1);
  ggc_add_tree_root (&ssdf_decl, 1);
  ggc_add_tree_root (&priority_decl, 1);
  ggc_add_tree_root (&initialize_p_decl, 1);
}
