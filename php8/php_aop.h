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

extern zend_module_entry aop_module_entry;
#define phpext_aop_ptr &aop_module_entry

#define PHP_AOP_VERSION "1.0.0" /* Replace with version number for your extension */

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

#define AOP_KIND_AROUND		1
#define AOP_KIND_BEFORE		2
#define AOP_KIND_AFTER		4
#define AOP_KIND_READ		8
#define AOP_KIND_WRITE 		16
#define AOP_KIND_PROPERTY	32
#define AOP_KIND_METHOD		64
#define AOP_KIND_FUNCTION	128
#define AOP_KIND_CATCH		256
#define AOP_KIND_RETURN		512

#define AOP_KIND_AROUND_READ_PROPERTY (AOP_KIND_AROUND+AOP_KIND_READ+AOP_KIND_PROPERTY)
#define AOP_KIND_AROUND_WRITE_PROPERTY (AOP_KIND_AROUND+AOP_KIND_WRITE+AOP_KIND_PROPERTY)
#define AOP_KIND_BEFORE_READ_PROPERTY (AOP_KIND_BEFORE+AOP_KIND_READ+AOP_KIND_PROPERTY)
#define AOP_KIND_BEFORE_WRITE_PROPERTY (AOP_KIND_BEFORE+AOP_KIND_WRITE+AOP_KIND_PROPERTY)
#define AOP_KIND_AFTER_READ_PROPERTY (AOP_KIND_AFTER+AOP_KIND_READ+AOP_KIND_PROPERTY)
#define AOP_KIND_AFTER_WRITE_PROPERTY (AOP_KIND_AFTER+AOP_KIND_WRITE+AOP_KIND_PROPERTY)

#define AOP_KIND_AROUND_METHOD (AOP_KIND_AROUND+AOP_KIND_METHOD)
#define AOP_KIND_AROUND_FUNCTION (AOP_KIND_AROUND+AOP_KIND_FUNCTION)
#define AOP_KIND_BEFORE_METHOD (AOP_KIND_BEFORE+AOP_KIND_METHOD)
#define AOP_KIND_BEFORE_FUNCTION (AOP_KIND_BEFORE+AOP_KIND_FUNCTION)
#define AOP_KIND_AFTER_METHOD (AOP_KIND_AFTER+AOP_KIND_METHOD)
#define AOP_KIND_AFTER_FUNCTION (AOP_KIND_AFTER+AOP_KIND_FUNCTION)

typedef struct {
	int scope;
	int static_state;

	zend_string *class_name;
	int class_jok;
	
	zend_string *method;
	int method_jok;
	
	zend_string *selector;
	int kind_of_advice;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;
	
	pcre2_code *re_method;
	pcre2_code *re_class;
} pointcut;

typedef struct {
    zend_array *ht;
    int version;
    zend_class_entry *ce;
} pointcut_cache;

typedef struct {
    zend_array *read;
    zend_array *write;
    zend_array *func;
} object_cache;

ZEND_BEGIN_MODULE_GLOBALS(aop)
	zend_bool aop_enable;
	zend_array *pointcuts_table;
	int pointcut_version;
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

void do_func_execute(HashPosition pos, zend_array *pointcut_table, zend_execute_data *ex, zval *object);
void do_read_property(HashPosition pos, zend_array *pointcut_table, zval *aop_object);
void do_write_property(HashPosition pos, zend_array *pointcut_table, zval *aop_object);

zend_object_read_property_t			original_zend_std_read_property;
zend_object_write_property_t		original_zend_std_write_property;
zend_object_get_property_ptr_ptr_t	original_zend_std_get_property_ptr_ptr;

zval *aop_read_property(zval *object, zval *member, int type, void **cache_slot, zval *rv);
void aop_write_property(zval *object, zval *member, zval *value, void **cache_slot);
zval *aop_get_property_ptr_ptr(zval *object, zval *member, int type, void **cache_slot);

void free_pointcut_cache(zval *elem);

extern ZEND_DECLARE_MODULE_GLOBALS(aop);

#if defined(ZTS) && defined(COMPILE_DL_AOP)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif  /* PHP_AOP_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
