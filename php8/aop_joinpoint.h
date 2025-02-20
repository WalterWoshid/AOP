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

#ifndef PHP_AOP_JOINPOINT_H
#define PHP_AOP_JOINPOINT_H

typedef struct {
    zend_object std;
    pointcut *current_pointcut;
    zend_array *advice;
    HashPosition pos;
    pointcut_type kind_of_advice;
    zend_execute_data *ex;
    int is_ex_executed;
    zend_object *exception;
    
    zval *args;
    zval *return_value;
    int return_value_changed;

    zend_object *object;
    zend_string *member;
    int type;
    void **cache_slot;
    zval *rv;
    zval property_value;
} AopJoinpoint_object;

extern zend_class_entry *aop_joinpoint_class_entry;

void register_class_AopJoinPoint(void);

PHP_METHOD(AopJoinpoint, getArguments);
PHP_METHOD(AopJoinpoint, getPropertyName);
PHP_METHOD(AopJoinpoint, getPropertyValue);
PHP_METHOD(AopJoinpoint, setArguments);
PHP_METHOD(AopJoinpoint, getKindOfAdvice);
PHP_METHOD(AopJoinpoint, getReturnedValue);
PHP_METHOD(AopJoinpoint, setReturnedValue);
PHP_METHOD(AopJoinpoint, getAssignedValue);
PHP_METHOD(AopJoinpoint, setAssignedValue);
PHP_METHOD(AopJoinpoint, getPointcut);
PHP_METHOD(AopJoinpoint, getObject);
PHP_METHOD(AopJoinpoint, getClassName);
PHP_METHOD(AopJoinpoint, getMethodName);
PHP_METHOD(AopJoinpoint, getFunctionName);
PHP_METHOD(AopJoinpoint, getException);
PHP_METHOD(AopJoinpoint, process);

ZEND_API void ZEND_FASTCALL _zval_dtor_func_for_ptr(zend_refcounted *p ZEND_FILE_LINE_DC);
#define zval_dtor_func_for_ptr(zv) _zval_dtor_func_for_ptr(zv ZEND_FILE_LINE_CC)

#endif
