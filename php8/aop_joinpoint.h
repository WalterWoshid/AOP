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
    int kind_of_advice;
    zend_execute_data *ex;
    int is_ex_executed;
    zend_object *exception;
    
    zval *args;
    zval *return_value;
    int return_value_changed;

    zval *object;
    zval *member;
    int type;
    void **cache_slot;
    zval *rv;
    zval property_value;
} AopJoinpoint_object;

extern zend_class_entry *aop_joinpoint_ce;

void register_class_AopJoinPoint(void);

PHP_METHOD(AopJoinpoint, getArguments); //done
PHP_METHOD(AopJoinpoint, getPropertyName);//done
PHP_METHOD(AopJoinpoint, getPropertyValue);//done
PHP_METHOD(AopJoinpoint, setArguments);//done
PHP_METHOD(AopJoinpoint, getKindOfAdvice);//done
PHP_METHOD(AopJoinpoint, getReturnedValue);//done
PHP_METHOD(AopJoinpoint, setReturnedValue);//done
PHP_METHOD(AopJoinpoint, getAssignedValue);//done
PHP_METHOD(AopJoinpoint, setAssignedValue);//done
PHP_METHOD(AopJoinpoint, getPointcut);//done
PHP_METHOD(AopJoinpoint, getObject);//done
PHP_METHOD(AopJoinpoint, getClassName);//done
PHP_METHOD(AopJoinpoint, getMethodName);//done
PHP_METHOD(AopJoinpoint, getFunctionName);//done
PHP_METHOD(AopJoinpoint, getException);//done
PHP_METHOD(AopJoinpoint, process);//done

#endif


