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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "ext/pcre/php_pcre.h"

#include "php_aop.h"
#include "aop_joinpoint.h"
#include "lexer.h"

static int le_aop;

ZEND_DECLARE_MODULE_GLOBALS(aop)


ZEND_BEGIN_ARG_INFO_EX(arginfo_aop_add, 0, 0, 2)
	ZEND_ARG_INFO(0, pointcut)
	ZEND_ARG_INFO(0, advice)
ZEND_END_ARG_INFO()


void make_regexp_on_pointcut (pointcut *pc) /*{{{*/
{
	int preg_options = 0;
	zend_string *regexp;
	zend_string *regexp_buffer = NULL;
	zend_string *regexp_tmp = NULL;
	char tempregexp[500];

	pc->method_jok = (strchr(ZSTR_VAL(pc->method), '*') != NULL);

	regexp_buffer = php_str_to_str(ZSTR_VAL(pc->method), ZSTR_LEN(pc->method), "**\\", 3, "[.#}", 4);
	
	regexp_tmp = regexp_buffer;
	regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "**", 2, "[.#]", 4);
	zend_string_release(regexp_tmp);

	regexp_tmp = regexp_buffer;
	regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "\\", 1, "\\\\", 2);
	zend_string_release(regexp_tmp);
	
	regexp_tmp = regexp_buffer;
	regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "*", 1, "[^\\\\]*", 6);
	zend_string_release(regexp_tmp);

	regexp_tmp = regexp_buffer;
	regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "[.#]", 4, ".*", 2);
	zend_string_release(regexp_tmp);
	
	regexp_tmp = regexp_buffer;
	regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "[.#}", 4, "(.*\\\\)?", 7);
	zend_string_release(regexp_tmp);

    if (ZSTR_VAL(regexp_buffer)[0] != '\\') {
        sprintf((char *)tempregexp, "/^%s$/i", ZSTR_VAL(regexp_buffer));
    } else {
        sprintf((char *)tempregexp, "/^%s$/i", ZSTR_VAL(regexp_buffer) + 2);
    }
	zend_string_release(regexp_buffer);

	regexp = zend_string_init(tempregexp, strlen(tempregexp), 0);
	pc->re_method = pcre_get_compiled_regex(regexp, &preg_options);
	zend_string_release(regexp);	

	if (!pc->re_method) {
        php_error_docref(NULL, E_WARNING, "Invalid expression");
    }

	if (pc->class_name != NULL) {
		regexp_buffer = php_str_to_str(ZSTR_VAL(pc->class_name), ZSTR_LEN(pc->class_name), "**\\", 3, "[.#}", 4);
		
		regexp_tmp = regexp_buffer;
		regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "**", 2, "[.#]", 4);
		zend_string_release(regexp_tmp);
		
		regexp_tmp = regexp_buffer;
		regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "\\", 1, "\\\\", 2);
		zend_string_release(regexp_tmp);
		
		regexp_tmp = regexp_buffer;
		regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "*", 1, "[^\\\\]*", 6);
		zend_string_release(regexp_tmp);
		
		regexp_tmp = regexp_buffer;
		regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "[.#]", 4, ".*", 2);
		zend_string_release(regexp_tmp);
		
		regexp_tmp = regexp_buffer;
		regexp_buffer = php_str_to_str(ZSTR_VAL(regexp_tmp), ZSTR_LEN(regexp_buffer), "[.#}", 4, "(.*\\\\)?", 7);
		zend_string_release(regexp_tmp);

		if (ZSTR_VAL(regexp_buffer)[0] != '\\') {
			sprintf((char *)tempregexp, "/^%s$/i", ZSTR_VAL(regexp_buffer));
		} else {
			sprintf((char *)tempregexp, "/^%s$/i", ZSTR_VAL(regexp_buffer) + 2);
		}
		zend_string_release(regexp_buffer);

		regexp = zend_string_init(tempregexp, strlen(tempregexp), 0);
		pc->re_class = pcre_get_compiled_regex(regexp, &preg_options);
		zend_string_release(regexp);

		if (!pc->re_class) {
			php_error_docref(NULL, E_WARNING, "Invalid expression");
		}
	}
}
/*}}}*/

static pointcut *alloc_pointcut() /*{{{*/
{
    pointcut *pc = (pointcut *)emalloc(sizeof(pointcut));

    pc->scope = 0;
    pc->static_state = 2;
    pc->method_jok = 0;
    pc->class_jok = 0;
    pc->class_name = NULL;
    pc->method = NULL;
    pc->selector = NULL;
    pc->kind_of_advice = 0;
    //pc->fci = NULL;
    //pc->fcic = NULL;
    pc->re_method = NULL;
    pc->re_class = NULL;
    return pc;
}
/*}}}*/

static void free_pointcut(zval *elem)
{
	pointcut *pc = (pointcut *)Z_PTR_P(elem);

	if (pc == NULL) {
		return;
	}

	if (&(pc->fci.function_name)) {
		zval_ptr_dtor(&pc->fci.function_name);
	}

	if (pc->method != NULL) {
		zend_string_release(pc->method);
	}
	if (pc->class_name != NULL) {
		zend_string_release(pc->class_name);
	}
	efree(pc);
}

void free_pointcut_cache(zval *elem)
{
    pointcut_cache *cache = (pointcut_cache *)Z_PTR_P(elem);
    if (cache->ht != NULL) {
        zend_hash_destroy(cache->ht);
        FREE_HASHTABLE(cache->ht);
    }
	efree(cache);
}

static void add_pointcut (zend_fcall_info fci, zend_fcall_info_cache fci_cache, zend_string *selector, int cut_type) /*{{{*/
{
	zval pointcut_val;
	pointcut *pc = NULL;
	char *temp_str = NULL;
	int is_class = 0;
	scanner_state *state;
	scanner_token *token;

	if (ZSTR_LEN(selector) < 2) {
		zend_error(E_ERROR, "The given pointcut is invalid. You must specify a function call, a method call or a property operation");
	}

	pc = alloc_pointcut();
	pc->selector = selector;
	pc->fci = fci;
	pc->fci_cache = fci_cache;
	pc->kind_of_advice = cut_type;

	state = (scanner_state *)emalloc(sizeof(scanner_state));
	token = (scanner_token *)emalloc(sizeof(scanner_token));

	state->start = ZSTR_VAL(selector);
	state->end = state->start;
	while(0 <= scan(state, token)) {
	    //	    php_printf("TOKEN %d \n", token->TOKEN);
		switch (token->TOKEN) {
			case TOKEN_STATIC:
				pc->static_state=token->int_val;
				break;
			case TOKEN_SCOPE:
				pc->scope |= token->int_val;
				break;
			case TOKEN_CLASS:
				pc->class_name = zend_string_init(temp_str, strlen(temp_str), 0);//estrdup(temp_str);
				efree(temp_str);
				temp_str=NULL;
				is_class=1;
				break;
			case TOKEN_PROPERTY:
				pc->kind_of_advice |= AOP_KIND_PROPERTY | token->int_val;
				break;
			case TOKEN_FUNCTION:
				if (is_class) {
					pc->kind_of_advice |= AOP_KIND_METHOD;
				} else {
					pc->kind_of_advice |= AOP_KIND_FUNCTION;
				}
				break;
			case TOKEN_TEXT:
				if (temp_str!=NULL) {
					efree(temp_str);
				}
				temp_str=estrdup(token->str_val);
				efree(token->str_val);
				break;
			default:
				break;
		}
	}
	if (temp_str != NULL) {
		//method or property
		pc->method = zend_string_init(temp_str, strlen(temp_str), 0);
		efree(temp_str);
	}
	efree(state);
    efree(token);

	//add("class::property", xxx)
	if (pc->kind_of_advice == cut_type) {
		pc->kind_of_advice |= AOP_KIND_READ | AOP_KIND_WRITE | AOP_KIND_PROPERTY;
	}

	make_regexp_on_pointcut(pc);
	
	//insert into hashTable:AOP_G(pointcuts)
	ZVAL_PTR(&pointcut_val, pc);
	zend_hash_next_index_insert(AOP_G(pointcuts_table), &pointcut_val);
	AOP_G(pointcut_version)++;
}
/*}}}*/

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_PHP_INI_BOOLEAN("aop.enable", "1", PHP_INI_ALL, OnUpdateBool, aop_enable, zend_aop_globals, aop_globals)
PHP_INI_END()
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(aop)
{
	REGISTER_INI_ENTRIES();
	
	//1.overload zend_execute_ex and zend_execute_internal
	original_zend_execute_ex = zend_execute_ex;
	zend_execute_ex = aop_execute_ex;

	original_zend_execute_internal = zend_execute_internal;
	zend_execute_internal = aop_execute_internal;

	//2.overload zend_std_read_property and zend_std_write_property
	original_zend_std_read_property = std_object_handlers.read_property;
	std_object_handlers.read_property = aop_read_property;

	original_zend_std_write_property = std_object_handlers.write_property;
	std_object_handlers.write_property = aop_write_property;

	/*
	 * To avoid zendvm inc/dec property value directly
	 * When get_property_ptr_ptr return NULL, zendvm will use write_property to inc/dec property value
	 */
	original_zend_std_get_property_ptr_ptr = std_object_handlers.get_property_ptr_ptr;
	std_object_handlers.get_property_ptr_ptr = aop_get_property_ptr_ptr;

	register_class_AopJoinPoint();

	REGISTER_LONG_CONSTANT("AOP_KIND_BEFORE", AOP_KIND_BEFORE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AFTER", AOP_KIND_AFTER, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AROUND", AOP_KIND_AROUND, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_PROPERTY", AOP_KIND_PROPERTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_FUNCTION", AOP_KIND_FUNCTION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_METHOD", AOP_KIND_METHOD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_READ", AOP_KIND_READ, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_WRITE", AOP_KIND_WRITE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AROUND_WRITE_PROPERTY", AOP_KIND_AROUND_WRITE_PROPERTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AROUND_READ_PROPERTY", AOP_KIND_AROUND_READ_PROPERTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_BEFORE_WRITE_PROPERTY", AOP_KIND_BEFORE_WRITE_PROPERTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_BEFORE_READ_PROPERTY", AOP_KIND_BEFORE_READ_PROPERTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AFTER_WRITE_PROPERTY", AOP_KIND_AFTER_WRITE_PROPERTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AFTER_READ_PROPERTY", AOP_KIND_AFTER_READ_PROPERTY, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_BEFORE_METHOD", AOP_KIND_BEFORE_METHOD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AFTER_METHOD", AOP_KIND_AFTER_METHOD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AROUND_METHOD", AOP_KIND_AROUND_METHOD, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_BEFORE_FUNCTION", AOP_KIND_BEFORE_FUNCTION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AFTER_FUNCTION", AOP_KIND_AFTER_FUNCTION, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("AOP_KIND_AROUND_FUNCTION", AOP_KIND_AROUND_FUNCTION, CONST_CS | CONST_PERSISTENT);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(aop)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(aop)
{
#if defined(COMPILE_DL_AOP) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    AOP_G(overloaded) = 0;
    AOP_G(pointcut_version) = 0;

	AOP_G(object_cache_size) = 1024;
    AOP_G(object_cache) = ecalloc(1024, sizeof(object_cache *));
    
	AOP_G(property_value) = NULL;

	AOP_G(lock_read_property) = 0;
	AOP_G(lock_write_property) = 0;

	//init AOP_G(pointcuts_table)
	ALLOC_HASHTABLE(AOP_G(pointcuts_table));
	zend_hash_init(AOP_G(pointcuts_table), 16, NULL, free_pointcut, 0);	

	ALLOC_HASHTABLE(AOP_G(function_cache));
	zend_hash_init(AOP_G(function_cache), 16, NULL, free_pointcut_cache, 0);	
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(aop)
{
	zend_array_destroy(AOP_G(pointcuts_table));
	zend_array_destroy(AOP_G(function_cache));
	
	int i;
    for (i = 0; i < AOP_G(object_cache_size); i++) {
        if (AOP_G(object_cache)[i] != NULL) {
			object_cache *_cache = AOP_G(object_cache)[i];
			if (_cache->write!=NULL) {
				zend_hash_destroy(_cache->write);
				FREE_HASHTABLE(_cache->write);
			}
			if (_cache->read!=NULL) {
				zend_hash_destroy(_cache->read);
				FREE_HASHTABLE(_cache->read);
			}
			if (_cache->func!=NULL) {
				zend_hash_destroy(_cache->func);
				FREE_HASHTABLE(_cache->func);
			}
			efree(_cache);
		}
	}
	efree(AOP_G(object_cache));

	if (AOP_G(property_value) != NULL) {
		zval_ptr_dtor(AOP_G(property_value));
		efree(AOP_G(property_value));
	}

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(aop)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "aop support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */

/*{{{ proto void aop_add_before(string selector, mixed pointcut)
 */
PHP_FUNCTION(aop_add_before)
{
	zend_string *selector;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	//parse prameters
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(selector)
		Z_PARAM_FUNC(fci, fci_cache)
	ZEND_PARSE_PARAMETERS_END_EX(
        zend_error(E_ERROR, "aop_add_before() expects a string for the pointcut as a first argument and a callback as a second argument");
		return;
	);

	if (&(fci.function_name)) {
		Z_TRY_ADDREF(fci.function_name);
	}
	add_pointcut(fci, fci_cache, selector, AOP_KIND_BEFORE);
}
/*}}}*/

/*{{{ proto void aop_add_around(string selector, mixed pointcut)
 */
PHP_FUNCTION(aop_add_around)
{
	zend_string *selector;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	//parse prameters
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(selector)
		Z_PARAM_FUNC(fci, fci_cache)
	ZEND_PARSE_PARAMETERS_END_EX(
		zend_error(E_ERROR, "aop_add_around() expects a string for the pointcut as a first argument and a callback as a second argument");
		return;
	);

	if (&(fci.function_name)) {
		Z_TRY_ADDREF(fci.function_name);
	}
	add_pointcut(fci, fci_cache, selector, AOP_KIND_AROUND);
}
/*}}}*/

/*{{{ proto void aop_add_after(string selector, mixed pointcut)
 */
PHP_FUNCTION(aop_add_after)
{
	zend_string *selector;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	//parse prameters
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(selector)
		Z_PARAM_FUNC(fci, fci_cache)
	ZEND_PARSE_PARAMETERS_END_EX(
        zend_error(E_ERROR, "aop_add_after() expects a string for the pointcut as a first argument and a callback as a second argument");
		return;
	);

	if (&(fci.function_name)) {
		Z_TRY_ADDREF(fci.function_name);
	}
	add_pointcut(fci, fci_cache, selector, AOP_KIND_AFTER | AOP_KIND_CATCH | AOP_KIND_RETURN);
}
/*}}}*/

/*{{{ proto void aop_add_after_returning(string selector, mixed pointcut)
 */
PHP_FUNCTION(aop_add_after_returning)
{
	zend_string *selector;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	//parse prameters
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(selector)
		Z_PARAM_FUNC(fci, fci_cache)
	ZEND_PARSE_PARAMETERS_END_EX(
        zend_error(E_ERROR, "aop_add_after() expects a string for the pointcut as a first argument and a callback as a second argument");
		return;
	);

	if (&(fci.function_name)) {
		Z_TRY_ADDREF(fci.function_name);
	}
	add_pointcut(fci, fci_cache, selector, AOP_KIND_AFTER | AOP_KIND_RETURN);
}
/*}}}*/

/*{{{ proto void aop_add_after_throwing(string selector, mixed pointcut)
 */
PHP_FUNCTION(aop_add_after_throwing)
{
	zend_string *selector;
	zend_fcall_info fci;
	zend_fcall_info_cache fci_cache;

	//parse prameters
	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(selector)
		Z_PARAM_FUNC(fci, fci_cache)
	ZEND_PARSE_PARAMETERS_END_EX(
        zend_error(E_ERROR, "aop_add_after() expects a string for the pointcut as a first argument and a callback as a second argument");
		return;
	);

	if (&(fci.function_name)) {
		Z_TRY_ADDREF(fci.function_name);
	}
	add_pointcut(fci, fci_cache, selector, AOP_KIND_AFTER | AOP_KIND_CATCH);
}
/*}}}*/

/* {{{ aop_functions[]
 */
const zend_function_entry aop_functions[] = {
	PHP_FE(aop_add_before,  arginfo_aop_add)
	PHP_FE(aop_add_around,  arginfo_aop_add)
	PHP_FE(aop_add_after,  arginfo_aop_add)
	PHP_FE(aop_add_after_returning,  arginfo_aop_add)
	PHP_FE(aop_add_after_throwing,  arginfo_aop_add)
	PHP_FE_END  /* Must be the last line in aop_functions[] */
};
/* }}} */

/* {{{ aop_module_entry
 */
zend_module_entry aop_module_entry = {
    STANDARD_MODULE_HEADER,
    "aop",
    aop_functions,
    PHP_MINIT(aop),
    PHP_MSHUTDOWN(aop),
    PHP_RINIT(aop),     /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(aop), /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(aop),
	PHP_AOP_VERSION,
	PHP_MODULE_GLOBALS(aop),
	NULL,
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_AOP
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(aop)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

