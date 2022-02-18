/*
  +----------------------------------------------------------------------+
  | PHP Version 8                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: pangudashu@gmail.com                                         |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_AOP_H
#define PHP_AOP_H

// Ignore unused macros
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

extern zend_module_entry aop_module_entry;
#define phpext_aop_ptr &aop_module_entry

#define PHP_AOP_VERSION "2.0.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#   define PHP_AOP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#   define PHP_AOP_API __attribute__ ((visibility("default")))
#else
#   define PHP_AOP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define AOP_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(aop, v)

/**
 * <h1>Types of pointcuts for bitwise operations</h1>
 */
typedef enum pointcut_type {
    AOP_KIND_NONE     = 0,

    AOP_KIND_AROUND   = 1,
    AOP_KIND_BEFORE   = 2,
    AOP_KIND_AFTER    = 4,

    AOP_KIND_READ     = 8,
    AOP_KIND_WRITE    = 16,
    AOP_KIND_PROPERTY = 32,
    AOP_KIND_METHOD   = 64,
    AOP_KIND_FUNCTION = 128,

    // Method + function
    AOP_KIND_METHOD_FUNCTION = 192,

    // Read + write + property + method + function
    AOP_KIND_ALL = 248,
    AOP_KIND_ALL_AROUND = 249,
    AOP_KIND_ALL_BEFORE = 250,
    AOP_KIND_ALL_AFTER  = 252,

    AOP_KIND_CATCH    = 256,
    AOP_KIND_RETURN   = 512,

    AOP_KIND_AROUND_READ_PROPERTY  = 41,
    AOP_KIND_AROUND_WRITE_PROPERTY = 49,
    AOP_KIND_BEFORE_READ_PROPERTY  = 42,
    AOP_KIND_BEFORE_WRITE_PROPERTY = 50,
    AOP_KIND_AFTER_READ_PROPERTY   = 44,
    AOP_KIND_AFTER_WRITE_PROPERTY  = 52,

    AOP_KIND_AROUND_METHOD   = 65,
    AOP_KIND_AROUND_FUNCTION = 129,
    AOP_KIND_BEFORE_METHOD   = 66,
    AOP_KIND_BEFORE_FUNCTION = 130,
    AOP_KIND_AFTER_METHOD    = 68,
    AOP_KIND_AFTER_FUNCTION  = 132,
} pointcut_type;

typedef struct {
	int scope;
	int static_state;

	zend_string *class_name;
	int class_name_has_wildcard;
	
	zend_string *method_name;
	int method_name_has_wildcard;
	
	zend_string *selector;
	pointcut_type kind_of_advice;
	zend_fcall_info fcall_info;
	zend_fcall_info_cache fcall_info_cache;
	
	pcre2_code *re_method;
	pcre2_code *re_class;
} pointcut;

typedef struct {
    zend_array *hash_table;
    int version;
    zend_class_entry *class_entry;
} pointcut_cache;

typedef struct {
    zend_array *read;
    zend_array *write;
    zend_array *func;
} object_cache;

ZEND_BEGIN_MODULE_GLOBALS(aop)
    // If AOP is enabled in the ini (aop.enabled)
	zend_bool aop_enable;
	zend_array *pointcuts_table;
	int pointcut_version;

    // We set this to 1 when we execute an internal function that is not aop-able
	int overloaded;

	zend_array *function_cache;

	object_cache **object_cache;
	int object_cache_size;

	int lock_read_property;
	int lock_write_property;

	zval *property_value;
ZEND_END_MODULE_GLOBALS(aop)

ZEND_API void (*original_zend_execute_ex)(zend_execute_data *execute_data);
ZEND_API void (*original_zend_execute_internal)(zend_execute_data *execute_data, zval *return_value);

ZEND_API void aop_execute_ex(zend_execute_data *execute_data);
ZEND_API void aop_execute_internal(zend_execute_data *execute_data, zval *return_value);

void do_func_execute(HashPosition pos, zend_array *pointcut_table, zend_execute_data *ex, zval *aop_object);
void do_read_property(HashPosition pos, zend_array *pointcut_table, zval *aop_object);
void do_write_property(HashPosition pos, zend_array *pointcut_table, zval *aop_object);

zend_object_read_property_t			original_zend_std_read_property;
zend_object_write_property_t		original_zend_std_write_property;
zend_object_get_property_ptr_ptr_t	original_zend_std_get_property_ptr_ptr;

zval *aop_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv);
zval *aop_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot);
zval *aop_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot);

void free_pointcut_cache(zval *elem);

extern ZEND_DECLARE_MODULE_GLOBALS(aop)

#if defined(ZTS) && defined(COMPILE_DL_AOP)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif  /* PHP_AOP_H */
