#ifndef minimal_HEADER_H
#define minimal_HEADER_H 1
#define minimal_IN_HEADER 1

#line 5 "eval/lexer.h"

#line 7 "eval/lexer.h"

#define  YY_INT_ALIGNED short int

/* A lexical scanner generated by flex */

#define FLEX_SCANNER
#define YY_FLEX_MAJOR_VERSION 2
#define YY_FLEX_MINOR_VERSION 6
#define YY_FLEX_SUBMINOR_VERSION 3
#if YY_FLEX_SUBMINOR_VERSION > 0
#define FLEX_BETA
#endif

    #define yy_create_buffer minimal__create_buffer

    #define yy_delete_buffer minimal__delete_buffer

    #define yy_scan_buffer minimal__scan_buffer

    #define yy_scan_string minimal__scan_string

    #define yy_scan_bytes minimal__scan_bytes

    #define yy_init_buffer minimal__init_buffer

    #define yy_flush_buffer minimal__flush_buffer

    #define yy_load_buffer_state minimal__load_buffer_state

    #define yy_switch_to_buffer minimal__switch_to_buffer

    #define yypush_buffer_state minimal_push_buffer_state

    #define yypop_buffer_state minimal_pop_buffer_state

    #define yyensure_buffer_stack minimal_ensure_buffer_stack

    #define yylex minimal_lex

    #define yyrestart minimal_restart

    #define yylex_init minimal_lex_init

    #define yylex_init_extra minimal_lex_init_extra

    #define yylex_destroy minimal_lex_destroy

    #define yyget_debug minimal_get_debug

    #define yyset_debug minimal_set_debug

    #define yyget_extra minimal_get_extra

    #define yyset_extra minimal_set_extra

    #define yyget_in minimal_get_in

    #define yyset_in minimal_set_in

    #define yyget_out minimal_get_out

    #define yyset_out minimal_set_out

    #define yyget_leng minimal_get_leng

    #define yyget_text minimal_get_text

    #define yyget_lineno minimal_get_lineno

    #define yyset_lineno minimal_set_lineno

        #define yyget_column minimal_get_column

        #define yyset_column minimal_set_column

    #define yywrap minimal_wrap

    #define yyget_lval minimal_get_lval

    #define yyset_lval minimal_set_lval

    #define yyalloc minimal_alloc

    #define yyrealloc minimal_realloc

    #define yyfree minimal_free

/* First, we deal with  platform-specific or compiler-specific issues. */

/* begin standard C headers. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* end standard C headers. */

/* flex integer type definitions */

#ifndef FLEXINT_H
#define FLEXINT_H

/* C99 systems have <inttypes.h>. Non-C99 systems may or may not. */

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

/* C99 says to define __STDC_LIMIT_MACROS before including stdint.h,
 * if you want the limit (max/min) macros for int types. 
 */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#include <inttypes.h>
typedef int8_t flex_int8_t;
typedef uint8_t flex_uint8_t;
typedef int16_t flex_int16_t;
typedef uint16_t flex_uint16_t;
typedef int32_t flex_int32_t;
typedef uint32_t flex_uint32_t;
#else
typedef signed char flex_int8_t;
typedef short int flex_int16_t;
typedef int flex_int32_t;
typedef unsigned char flex_uint8_t; 
typedef unsigned short int flex_uint16_t;
typedef unsigned int flex_uint32_t;

/* Limits of integral types. */
#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#endif /* ! C99 */

#endif /* ! FLEXINT_H */

/* TODO: this is always defined, so inline it */
#define yyconst const

#if defined(__GNUC__) && __GNUC__ >= 3
#define yynoreturn __attribute__((__noreturn__))
#else
#define yynoreturn
#endif

/* An opaque pointer. */
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

/* For convenience, these vars (plus the bison vars far below)
   are macros in the reentrant scanner. */
#define yyin yyg->yyin_r
#define yyout yyg->yyout_r
#define yyextra yyg->yyextra_r
#define yyleng yyg->yyleng_r
#define yytext yyg->yytext_r
#define yylineno (YY_CURRENT_BUFFER_LVALUE->yy_bs_lineno)
#define yycolumn (YY_CURRENT_BUFFER_LVALUE->yy_bs_column)
#define yy_flex_debug yyg->yy_flex_debug_r

/* Size of default input buffer. */
#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

#ifndef YY_STRUCT_YY_BUFFER_STATE
#define YY_STRUCT_YY_BUFFER_STATE
struct yy_buffer_state
	{
	FILE *yy_input_file;

	char *yy_ch_buf;		/* input buffer */
	char *yy_buf_pos;		/* current position in input buffer */

	/* Size of input buffer in bytes, not including room for EOB
	 * characters.
	 */
	int yy_buf_size;

	/* Number of characters read into yy_ch_buf, not including EOB
	 * characters.
	 */
	int yy_n_chars;

	/* Whether we "own" the buffer - i.e., we know we created it,
	 * and can realloc() it to grow it, and should free() it to
	 * delete it.
	 */
	int yy_is_our_buffer;

	/* Whether this is an "interactive" input source; if so, and
	 * if we're using stdio for input, then we want to use getc()
	 * instead of fread(), to make sure we stop fetching input after
	 * each newline.
	 */
	int yy_is_interactive;

	/* Whether we're considered to be at the beginning of a line.
	 * If so, '^' rules will be active on the next match, otherwise
	 * not.
	 */
	int yy_at_bol;

    int yy_bs_lineno; /**< The line count. */
    int yy_bs_column; /**< The column count. */

	/* Whether to try to fill the input buffer when we reach the
	 * end of it.
	 */
	int yy_fill_buffer;

	int yy_buffer_status;

	};
#endif /* !YY_STRUCT_YY_BUFFER_STATE */

void minimal_restart ( FILE *input_file , yyscan_t yyscanner );
void minimal__switch_to_buffer ( YY_BUFFER_STATE new_buffer , yyscan_t yyscanner );
YY_BUFFER_STATE minimal__create_buffer ( FILE *file, int size , yyscan_t yyscanner );
void minimal__delete_buffer ( YY_BUFFER_STATE b , yyscan_t yyscanner );
void minimal__flush_buffer ( YY_BUFFER_STATE b , yyscan_t yyscanner );
void minimal_push_buffer_state ( YY_BUFFER_STATE new_buffer , yyscan_t yyscanner );
void minimal_pop_buffer_state ( yyscan_t yyscanner );

YY_BUFFER_STATE minimal__scan_buffer ( char *base, yy_size_t size , yyscan_t yyscanner );
YY_BUFFER_STATE minimal__scan_string ( const char *yy_str , yyscan_t yyscanner );
YY_BUFFER_STATE minimal__scan_bytes ( const char *bytes, int len , yyscan_t yyscanner );

void *minimal_alloc ( yy_size_t , yyscan_t yyscanner );
void *minimal_realloc ( void *, yy_size_t , yyscan_t yyscanner );
void minimal_free ( void * , yyscan_t yyscanner );

/* Begin user sect3 */

#define minimal_wrap(yyscanner) (/*CONSTCOND*/1)
#define YY_SKIP_YYWRAP

#define yytext_ptr yytext_r

#ifdef YY_HEADER_EXPORT_START_CONDITIONS
#define INITIAL 0
#define comment 1
#define cdoc 2
#define COMMENT 3
#define CDOC 4

#endif

#ifndef YY_NO_UNISTD_H
/* Special case for "unistd.h", since it is non-ANSI. We include it way
 * down here because we want the user's section 1 to have been scanned first.
 * The user has a chance to override it with an option.
 */
#include <unistd.h>
#endif

#ifndef YY_EXTRA_TYPE
#define YY_EXTRA_TYPE void *
#endif

int minimal_lex_init (yyscan_t* scanner);

int minimal_lex_init_extra ( YY_EXTRA_TYPE user_defined, yyscan_t* scanner);

/* Accessor methods to globals.
   These are made visible to non-reentrant scanners for convenience. */

int minimal_lex_destroy ( yyscan_t yyscanner );

int minimal_get_debug ( yyscan_t yyscanner );

void minimal_set_debug ( int debug_flag , yyscan_t yyscanner );

YY_EXTRA_TYPE minimal_get_extra ( yyscan_t yyscanner );

void minimal_set_extra ( YY_EXTRA_TYPE user_defined , yyscan_t yyscanner );

FILE *minimal_get_in ( yyscan_t yyscanner );

void minimal_set_in  ( FILE * _in_str , yyscan_t yyscanner );

FILE *minimal_get_out ( yyscan_t yyscanner );

void minimal_set_out  ( FILE * _out_str , yyscan_t yyscanner );

			int minimal_get_leng ( yyscan_t yyscanner );

char *minimal_get_text ( yyscan_t yyscanner );

int minimal_get_lineno ( yyscan_t yyscanner );

void minimal_set_lineno ( int _line_number , yyscan_t yyscanner );

int minimal_get_column  ( yyscan_t yyscanner );

void minimal_set_column ( int _column_no , yyscan_t yyscanner );

YYSTYPE * minimal_get_lval ( yyscan_t yyscanner );

void minimal_set_lval ( YYSTYPE * yylval_param , yyscan_t yyscanner );

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int minimal_wrap ( yyscan_t yyscanner );
#else
extern int minimal_wrap ( yyscan_t yyscanner );
#endif
#endif

#ifndef yytext_ptr
static void yy_flex_strncpy ( char *, const char *, int , yyscan_t yyscanner);
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen ( const char * , yyscan_t yyscanner);
#endif

#ifndef YY_NO_INPUT

#endif

/* Amount of stuff to slurp up with each read. */
#ifndef YY_READ_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k */
#define YY_READ_BUF_SIZE 16384
#else
#define YY_READ_BUF_SIZE 8192
#endif /* __ia64__ */
#endif

/* Number of entries by which start-condition stack grows. */
#ifndef YY_START_STACK_INCR
#define YY_START_STACK_INCR 25
#endif

/* Default declaration of generated scanner - a define so the user can
 * easily add parameters.
 */
#ifndef YY_DECL
#define YY_DECL_IS_OURS 1

extern int minimal_lex \
               (YYSTYPE * yylval_param , yyscan_t yyscanner);

#define YY_DECL int minimal_lex \
               (YYSTYPE * yylval_param , yyscan_t yyscanner)
#endif /* !YY_DECL */

/* yy_get_previous_state - get the state just before the EOB char was reached */

#undef YY_NEW_FILE
#undef YY_FLUSH_BUFFER
#undef yy_set_bol
#undef yy_new_buffer
#undef yy_set_interactive
#undef YY_DO_BEFORE_ACTION

#ifdef YY_DECL_IS_OURS
#undef YY_DECL_IS_OURS
#undef YY_DECL
#endif

#line 258 "eval/gwion.l"


#line 423 "eval/lexer.h"
#undef minimal_IN_HEADER
#endif /* minimal_HEADER_H */
