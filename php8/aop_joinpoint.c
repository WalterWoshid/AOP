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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/pcre/php_pcre.h"

#include "php_aop.h"
#include "aop_joinpoint.h"

zend_class_entry *aop_joinpoint_class_entry;

zend_object_handlers AopJoinpoint_object_handlers;

ZEND_BEGIN_ARG_INFO_EX(arginfo_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_aop_args_returnbyref, 0, ZEND_RETURN_REFERENCE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_aop_args_setArguments, 0)
    ZEND_ARG_ARRAY_INFO(0, arguments, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_aop_args_setReturnedValue, 0)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_aop_args_setAssignedValue, 0)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

zend_function_entry aop_joinpoint_methods[] = {
    PHP_ME(AopJoinpoint, getArguments, arginfo_void, 0)
    PHP_ME(AopJoinpoint, setArguments, arginfo_aop_args_setArguments, 0)
    PHP_ME(AopJoinpoint, getException, arginfo_void, 0)
    PHP_ME(AopJoinpoint, getPointcut, arginfo_void, 0)
    PHP_ME(AopJoinpoint, process, arginfo_void, 0)
    PHP_ME(AopJoinpoint, getKindOfAdvice, arginfo_void, 0)
    PHP_ME(AopJoinpoint, getObject, arginfo_void, 0)
    PHP_ME(AopJoinpoint, getReturnedValue, arginfo_aop_args_returnbyref, 0)
    PHP_ME(AopJoinpoint, setReturnedValue, arginfo_aop_args_setReturnedValue, 0)
    PHP_ME(AopJoinpoint, getClassName, arginfo_void, 0)
    PHP_ME(AopJoinpoint, getMethodName, arginfo_void, 0)
    PHP_ME(AopJoinpoint, getFunctionName, arginfo_void, 0)
    PHP_ME(AopJoinpoint, getAssignedValue, arginfo_aop_args_returnbyref, 0)
    PHP_ME(AopJoinpoint, setAssignedValue, arginfo_aop_args_setAssignedValue, 0)
    PHP_ME(AopJoinpoint, getPropertyName, arginfo_void, 0)
    PHP_ME(AopJoinpoint, getPropertyValue, arginfo_void, 0)

    PHP_FE_END
};

void aop_free_JoinPoint(zend_object *object)
{
    AopJoinpoint_object *obj = (AopJoinpoint_object *) object;

    if (obj->args != NULL) {
        zval_ptr_dtor(obj->args);
        efree(obj->args);
    }
    if (obj->return_value != NULL) {
        zval_ptr_dtor(obj->return_value);
        efree(obj->return_value);
    }
    if (Z_TYPE(obj->property_value) != IS_UNDEF) {
        zval_ptr_dtor(&obj->property_value);
        // efree(obj->property_value);
    }
    zend_object_std_dtor(object);
}

static inline void _zend_assign_to_variable_reference(zval *variable_ptr, zval *value_ptr) // NOLINT(bugprone-reserved-identifier)
{
    if (EXPECTED(!Z_ISREF_P(value_ptr))) {
        ZVAL_NEW_REF(value_ptr, value_ptr);
    } else if (UNEXPECTED(variable_ptr == value_ptr)) {
        return;
    }

    zend_reference *ref = Z_REF_P(value_ptr);
    if (Z_REFCOUNTED_P(variable_ptr)) {
        zend_refcounted *garbage = Z_COUNTED_P(variable_ptr);

        int ref_garbage = GC_REFCOUNT(garbage);
        if (--ref_garbage == 0) {
            ZVAL_REF(variable_ptr, ref);
            zval_dtor_func_for_ptr(garbage);
            return;
        } else {
            if (UNEXPECTED(GC_MAY_LEAK(garbage))) {
                gc_possible_root(garbage);
            }
        }
    }
    ZVAL_REF(variable_ptr, ref);
}

// new AopJoinPoint()
zend_object *aop_create_handler_JoinPoint(zend_class_entry *class_entry)
{
    AopJoinpoint_object *obj = (AopJoinpoint_object *) emalloc(sizeof(AopJoinpoint_object));
    
    zend_object_std_init(&obj->std, class_entry);
    obj->std.handlers = &AopJoinpoint_object_handlers;

    return &obj->std;
}

void register_class_AopJoinPoint(void)
{
    zend_class_entry class_entry;
    INIT_CLASS_ENTRY(class_entry, "AopJoinpoint", aop_joinpoint_methods)
    aop_joinpoint_class_entry = zend_register_internal_class(&class_entry);

    aop_joinpoint_class_entry->create_object = aop_create_handler_JoinPoint;
    memcpy(&AopJoinpoint_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    AopJoinpoint_object_handlers.clone_obj = NULL;
    AopJoinpoint_object_handlers.free_obj = aop_free_JoinPoint;
}

/* proto array AopJoinpoint::getArguments() */
PHP_METHOD(AopJoinpoint, getArguments)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());

    if (object->args == NULL) {
        zval *arg, *extra_start;
        zval *ret = emalloc(sizeof(zval));
        zend_op_array *op_array = &object->ex->func->op_array;
        
        array_init(ret);

        uint32_t first_extra_arg = op_array->num_args;
        uint32_t call_num_args = ZEND_CALL_NUM_ARGS(object->ex);

        int i;
        if (call_num_args <= first_extra_arg) {
            for (i = 0; i < call_num_args; i++){
                arg = ZEND_CALL_VAR_NUM(object->ex, i);
                if (Z_ISUNDEF_P(arg)) {
                    continue;
                }
                Z_TRY_ADDREF_P(arg);
                zend_hash_next_index_insert(Z_ARR_P(ret), arg);
            }
        } else {
            for (i = 0; i < first_extra_arg; i++){
                arg = ZEND_CALL_VAR_NUM(object->ex, i);
                if (Z_ISUNDEF_P(arg)) {
                    continue;
                }
                Z_TRY_ADDREF_P(arg);
                zend_hash_next_index_insert(Z_ARR_P(ret), arg);
            }
            // Get extra params
            extra_start = ZEND_CALL_VAR_NUM(object->ex, op_array->last_var + op_array->T);
            for (i = 0; i < call_num_args - first_extra_arg; i++) {
                Z_TRY_ADDREF_P(extra_start + i);
                zend_hash_next_index_insert(Z_ARR_P(ret), extra_start + i);
            }
        }
        
        object->args = ret;
    }
    RETURN_ZVAL(object->args, 1, 0);
}

/**
 * AopJoinpoint::setArguments(array $arguments)
 *
 * Set arguments for current joinpoint
 *
 * @php_param array $arguments
 *
 * @php_return void
 */
PHP_METHOD(AopJoinpoint, setArguments)
{
    // Arguments
    zval *params;

    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    if (object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY) {
        zend_error(
            E_ERROR,
            "setArguments is only available when the JoinPoint is a function or a method call"
        );
    }

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY(params)
	ZEND_PARSE_PARAMETERS_END();

    if (object->args != NULL) {
        zval_ptr_dtor(object->args);
    } else {
        object->args = emalloc(sizeof(zval));
    }
    ZVAL_COPY(object->args, params);

    RETURN_NULL();
}

/* proto mixed AopJoinpoint::getException() */
PHP_METHOD(AopJoinpoint, getException)
{
    // Exception
    zval exception_val;

    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    
    if (!(object->current_pointcut->kind_of_advice & AOP_KIND_CATCH)) {
        zend_error(
            E_ERROR,
            "getException is only available when the advice was added with aop_add_after or aop_add_after_throwing"
        );
    }

    if (object->exception != NULL) {
        ZVAL_OBJ(&exception_val, object->exception);
        RETURN_ZVAL(&exception_val, 1, 0);
    }
    RETURN_NULL();
}

/* proto string AopJoinpoint::getPointcut() */
PHP_METHOD(AopJoinpoint, getPointcut)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    RETURN_STR(object->current_pointcut->selector);
}

/* proto void AopJoinpoint::process() */
PHP_METHOD(AopJoinpoint, process)
{
    AopJoinpoint_object *aop_object = (AopJoinpoint_object *) Z_OBJ_P(getThis());

    if (!aop_object || !aop_object->current_pointcut || !aop_object->current_pointcut->kind_of_advice) {
        zend_error(
            E_ERROR,
            "Error" // todo: add error message
        );
    }
    if (!(aop_object->current_pointcut->kind_of_advice & AOP_KIND_AROUND)) {
        zend_error(
            E_ERROR,
            "Process is only available when the advice was added with aop_add_around"
        );
    }

    if (aop_object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY) {
        if (aop_object->kind_of_advice & AOP_KIND_WRITE) {
            do_write_property(aop_object->pos, aop_object->advice, getThis());
        } else {
            do_read_property(aop_object->pos, aop_object->advice, getThis());
        }
    } else {
        zval call_ret;
        int is_ret_overloaded = 0;
        if (aop_object->ex->return_value == NULL) {
            aop_object->ex->return_value = &call_ret;
            is_ret_overloaded = 1;
        }
        do_func_execute(aop_object->pos, aop_object->advice, aop_object->ex, getThis());
        if (is_ret_overloaded == 0) {
            if (EG(exception) == NULL) {
                ZVAL_COPY(return_value, aop_object->ex->return_value);
            }
        } else {
            if (EG(exception) == NULL) {
                ZVAL_COPY_VALUE(return_value, aop_object->ex->return_value);
            }
            aop_object->ex->return_value = NULL;
        }
    }
}

/* proto int AopJoinpoint::getKindOfAdvice() */
PHP_METHOD(AopJoinpoint, getKindOfAdvice)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    RETURN_LONG(object->kind_of_advice);
}

/* proto mixed AopJoinpoint::getObject() */
PHP_METHOD(AopJoinpoint, getObject)
{
    zend_object *call_object = NULL;
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    
    if (object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY) {
        if (object->object != NULL) {
            RETURN_OBJ(object->object);
        }
    } else {
        if (Z_TYPE(object->ex->This) == IS_OBJECT) {
            call_object = Z_OBJ(object->ex->This);
        }

        if (call_object != NULL) {
            RETURN_ZVAL(&object->ex->This, 1, 0);
        }
    }
    RETURN_NULL();
}

/* proto mixed AopJoinpoint::getReturnedValue() */
PHP_METHOD(AopJoinpoint, getReturnedValue)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    
    if (object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY) {
        zend_error(
            E_ERROR,
            "getReturnedValue is not available when the JoinPoint is a property operation (read or write)"
        );
    }
    if (object->current_pointcut->kind_of_advice & AOP_KIND_BEFORE) {
        zend_error(
            E_ERROR,
            "getReturnedValue is not available when the advice was added with aop_add_before"
        );
    }

    if (object->ex->return_value != NULL) {
        if (EXPECTED(!Z_ISREF_P(object->ex->return_value))) {
            object->return_value_changed = 1;
        }
        _zend_assign_to_variable_reference(return_value, object->ex->return_value);
    }
}

/* proto void AopJoinpoint::setReturnedValue(mixed return_value) */
PHP_METHOD(AopJoinpoint, setReturnedValue)
{
    // Returned value
    zval *ret;

    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    
    if (object->kind_of_advice & AOP_KIND_WRITE) {
        zend_error(
            E_ERROR,
            "setReturnedValue is not available when the JoinPoint is a property write operation"
        );
    }
    
    ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ZVAL(ret)
	ZEND_PARSE_PARAMETERS_END();

    if (object->return_value != NULL) {
        zval_ptr_dtor(object->return_value);
    } else {
        object->return_value = emalloc(sizeof(zval));
    }
    ZVAL_COPY(object->return_value, ret);

    RETURN_NULL();
}

/* proto string AopJoinpoint::getClassName() */
PHP_METHOD(AopJoinpoint, getClassName)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());

    if (object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY) {
        if (object->object != NULL) {
            zend_class_entry *class_entry = object->object->ce;
            RETURN_STR(class_entry->name);
        }
    } else {
        zend_class_entry *class_entry = NULL;
        zend_object *call_object = NULL;

        if (Z_TYPE(object->ex->This) == IS_OBJECT) {
            call_object = Z_OBJ(object->ex->This);
        }

        if (call_object != NULL) {
            class_entry = Z_OBJCE(object->ex->This);
            RETURN_STR(class_entry->name);
        }

        if (object->ex->func->common.fn_flags & ZEND_ACC_STATIC) {
            class_entry = object->ex->func->common.scope; // object->ex->called_scope;
            RETURN_STR(class_entry->name);
        }
    }
    RETURN_NULL();
}

/* proto string AopJoinpoint::getMethodName() */
PHP_METHOD(AopJoinpoint, getMethodName)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    
    if (object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY
        || object->current_pointcut->kind_of_advice & AOP_KIND_FUNCTION
    ) {
        zend_error(E_ERROR, "getMethodName is only available when the JoinPoint is a method call"); 
    }
    if (object->ex == NULL) {
        RETURN_NULL();
    }
    RETURN_STR(object->ex->func->common.function_name);
}

/* proto string AopJoinpoint::getFunctionName() */
PHP_METHOD(AopJoinpoint, getFunctionName)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    
    if (object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY
        || object->current_pointcut->kind_of_advice & AOP_KIND_METHOD
    ) {
        zend_error(
            E_ERROR,
            "getMethodName is only available when the JoinPoint is a function call"
        );
    }
    if (object->ex == NULL) {
        RETURN_NULL();
    }
    RETURN_STR(object->ex->func->common.function_name);
}

/* proto mixed AopJoinpoint::getAssignedValue() */
PHP_METHOD(AopJoinpoint, getAssignedValue)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    
    if (!(object->kind_of_advice & AOP_KIND_WRITE)) {
        zend_error(
            E_ERROR,
            "getAssignedValue is only available when the JoinPoint is a property write operation"
        );
    }

    if (Z_TYPE(object->property_value) != IS_UNDEF) {
        _zend_assign_to_variable_reference(return_value, &object->property_value);
    } else {
        RETURN_NULL();
    } 
}

/* proto void AopJoinpoint::setAssignedValue(mixed property_value) */
PHP_METHOD(AopJoinpoint, setAssignedValue)
{
    // Assigned value
    zval *assigned_value;

    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());
    
    if (object->kind_of_advice & AOP_KIND_READ) {
        zend_error(
            E_ERROR,
            "setAssignedValue is not available when the JoinPoint is a property read operation"
        );
    }
    // Parse parameters
	ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(assigned_value)
	ZEND_PARSE_PARAMETERS_END_EX(
        zend_error(E_ERROR, "Error");
		return;
	);

    if (Z_TYPE(object->property_value) != IS_UNDEF) {
        zval_ptr_dtor(&object->property_value);
    }

    ZVAL_COPY(&object->property_value, assigned_value);
    RETURN_NULL();
}

/* proto mixed AopJoinpoint::getPropertyName() */
PHP_METHOD(AopJoinpoint, getPropertyName)
{
    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());

    if (!(object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY)) {
        zend_error(
            E_ERROR,
            "getPropertyName is only available when the JoinPoint is a property operation (read or write)"
        );
    }

    if (object->member != NULL) {
        RETURN_STR(object->member);
    }
    RETURN_NULL();
}

/* proto mixed AopJoinpoint::getPropertyValue() */
PHP_METHOD(AopJoinpoint, getPropertyValue)
{
    // Property value
    zval *ret;

    AopJoinpoint_object *object = (AopJoinpoint_object *) Z_OBJ_P(getThis());

    if (!(object->current_pointcut->kind_of_advice & AOP_KIND_PROPERTY)) {
        zend_error(E_ERROR, "getPropertyValue is only available when the JoinPoint is a property operation (read or write)"); 
    }

    if (object->object != NULL && object->member != NULL) {
       ret = aop_get_property_ptr_ptr(
           object->object,
           object->member,
           object->type,
           object->cache_slot
       );
    }
    RETURN_ZVAL(ret, 1, 0);
}
