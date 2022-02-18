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
#include "zend_execute.h"

#include "php_aop.h"
#include "aop_execute.h"
#include "aop_joinpoint.h"

static int strcmp_with_joker_case(char *str_with_jok, char *str, int case_sensitive)
{
    if (str_with_jok[0] == '*') {
        if (str_with_jok[1] == '\0') {
            return 1;
        }
    }
    if (str_with_jok[0] == '*') {
        return !strcmp(str_with_jok+1, str+(strlen(str)-(strlen(str_with_jok)-1)));
    }
    if (str_with_jok[strlen(str_with_jok)-1] == '*') {
        return !strncmp(str_with_jok, str, strlen(str_with_jok)-1);
    }
    if (case_sensitive) {
        return !strcmp(str_with_jok, str);
    } else {
        return !strcasecmp(str_with_jok, str);
    }
}

static int pointcut_match_zend_class_entry(pointcut *pc, zend_class_entry *class_entry)
{
    if (!pc->re_class) {
        return 0;
    }

    pcre2_match_data *match_data = php_pcre_create_match_data(0, pc->re_class);
    if (!match_data) {
        return 0;
    }
    
    int matches = pcre2_match(
        pc->re_class,
        (PCRE2_SPTR) ZSTR_VAL(class_entry->name),
        ZSTR_LEN(class_entry->name),
        0,
        0,
        match_data,
        php_pcre_mctx()
    );
    if (matches >= 0) {
        php_pcre_free_match_data(match_data);
        return 1;
    }           
    for (int i = 0; i < (int) class_entry->num_interfaces; i++) {
        matches = pcre2_match(
            pc->re_class,
            (PCRE2_SPTR) ZSTR_VAL(class_entry->interfaces[i]->name),
            ZSTR_LEN(class_entry->interfaces[i]->name),
            0,
            0,
            match_data,
            php_pcre_mctx()
        );
        if (matches >= 0) {
            php_pcre_free_match_data(match_data);
            return 1;
        }
    }
    
    for (int i = 0; i < (int) class_entry->num_traits; i++) {
        matches = pcre2_match(
            pc->re_class,
            (PCRE2_SPTR) ZSTR_VAL(class_entry->trait_names[i].name),
            ZSTR_LEN(class_entry->trait_names[i].name),
            0,
            0,
            match_data,
            php_pcre_mctx()
        );
        if (matches >= 0) {
            php_pcre_free_match_data(match_data);
            return 1;
        }
    }

    class_entry = class_entry->parent;
    while (class_entry != NULL) {
        matches = pcre2_match(
            pc->re_class,
            (PCRE2_SPTR) ZSTR_VAL(class_entry->name),
            ZSTR_LEN(class_entry->name),
            0,
            0,
            match_data,
            php_pcre_mctx()
        );
        if (matches >= 0) {
            php_pcre_free_match_data(match_data);
            return 1;
        }
        class_entry = class_entry->parent;
    }
    php_pcre_free_match_data(match_data);
    return 0; 
}

static zend_array *calculate_class_pointcuts(zend_class_entry *class_entry, pointcut_type kind_of_advice)
{
    zend_array *hash_table;
    ALLOC_HASHTABLE(hash_table);
    zend_hash_init(hash_table, 16, NULL, NULL , 0);

    zval *pc_value;
    ZEND_HASH_FOREACH_VAL(AOP_G(pointcuts_table), pc_value) {
        pointcut *pc = (pointcut *) Z_PTR_P(pc_value);

        // Find pointcut type that matches 'kind_of_advice' from this method
        if (!(pc->kind_of_advice & kind_of_advice)) {
            continue;
        }

        if ((class_entry == NULL && (pc->kind_of_advice & AOP_KIND_FUNCTION))
            || (class_entry != NULL && pointcut_match_zend_class_entry(pc, class_entry))
        ) {
            zend_hash_next_index_insert(hash_table, pc_value);
        }
    } ZEND_HASH_FOREACH_END(); 

    return hash_table;
}

static int pointcut_match_zend_function(pointcut *pc, zend_execute_data *ex)
{
    // Check static
    zend_function *curr_func = ex->func;
    if (pc->static_state != 2) {
        if (pc->static_state) {
            if (!(curr_func->common.fn_flags & ZEND_ACC_STATIC)) {
                return 0;
            }
        } else {
            if ((curr_func->common.fn_flags & ZEND_ACC_STATIC)) {
                return 0;
            }
        }
    }

    // Check public/protect/private
    if (pc->scope != 0 && !(pc->scope & (curr_func->common.fn_flags & ZEND_ACC_PPP_MASK))) {
        return 0;
    } 

    if (pc->class_name == NULL && ZSTR_VAL(pc->method_name)[0] == '*' && ZSTR_VAL(pc->method_name)[1] == '\0') {
        return 1;
    }
    if (pc->class_name == NULL && curr_func->common.scope != NULL) {
        return 0;
    }

    // Check if the method name matches
    if (pc->method_name_has_wildcard) {
        pcre2_match_data *match_data = php_pcre_create_match_data(0, pc->re_method);
        if (!match_data) {
            return 0;
        }
        zend_string *function_name = curr_func->common.function_name;
        int matches = pcre2_match(
            pc->re_method,                        // code
            (PCRE2_SPTR) ZSTR_VAL(function_name), // subject
            ZSTR_LEN(function_name),              // length
            0,                                    // start offset
            0,                         // options
            match_data,                           // match data
            php_pcre_mctx()                       // match context
        );
        php_pcre_free_match_data(match_data);
    
        // 0 means that the vector is too small to hold all the captured substring offsets
        if (matches < 0) {
            return 0;
        }
    } else {
        int comp_start = (ZSTR_VAL(pc->method_name)[0] == '\\') ? 1 : 0;
        if (strcasecmp(ZSTR_VAL(pc->method_name) + comp_start, ZSTR_VAL(curr_func->common.function_name)) > 0) {
            return 0;
        }
    }
    return 1;
}

static zend_array *calculate_function_pointcuts(zend_execute_data *ex)
{
    zend_object *object = NULL;
    if (Z_TYPE(ex->This) == IS_OBJECT) {
        object = Z_OBJ(ex->This);
    }

    zend_class_entry *class_entry = NULL;
    if (object != NULL) {
        class_entry = Z_OBJCE(ex->This);
    }

    if (class_entry == NULL && ex->func->common.fn_flags & ZEND_ACC_STATIC) {
        class_entry = ex->func->common.scope; // ex->called_scope;
    }

    zend_array *class_pointcuts = calculate_class_pointcuts(class_entry, AOP_KIND_METHOD_FUNCTION);

    zval *pc_value;
    zend_ulong h;
    ZEND_HASH_FOREACH_NUM_KEY_VAL(class_pointcuts, h, pc_value) {
        pointcut *pc = (pointcut *) Z_PTR_P(pc_value);
        if (pointcut_match_zend_function(pc, ex)) {
            continue;
        }
        // Delete unmatched element
        zend_hash_index_del(class_pointcuts, h);
    } ZEND_HASH_FOREACH_END(); 

    return class_pointcuts;
}

static int test_property_scope(pointcut *current_pc, zend_class_entry *class_entry, zend_string *member_str)
{
    zval *property_info_val = zend_hash_find(&class_entry->properties_info, member_str);
    if (property_info_val) {
        zend_property_info *property_info = (zend_property_info *) Z_PTR_P(property_info_val);
        if (current_pc->static_state != 2) {
            if (current_pc->static_state) {
                if (!(property_info->flags & ZEND_ACC_STATIC)) {
                    return 0;
                }
            } else {
                if ((property_info->flags & ZEND_ACC_STATIC)) {
                    return 0;
                }
            }
        }       
        if (current_pc->scope != 0 && !(current_pc->scope & (property_info->flags & ZEND_ACC_PPP_MASK))) {
            return 0;
        }
    } else {
        if (current_pc->scope != 0 && !(current_pc->scope & ZEND_ACC_PUBLIC)) {
            return 0;
        }
        if (current_pc->static_state == 1) {
            return 0;
        }
    }
    return 1;
}

static zend_array *calculate_property_pointcuts(zend_object *object, zend_string *member_str, pointcut_type kind)
{
    zend_array *class_pointcuts = calculate_class_pointcuts(object->ce, kind);

    zval *pc_value;
    zend_ulong h;
    ZEND_HASH_FOREACH_NUM_KEY_VAL(class_pointcuts, h, pc_value) {
        pointcut *pc = (pointcut *) Z_PTR_P(pc_value);
        if (ZSTR_VAL(pc->method_name)[0] != '*') {
            if (!strcmp_with_joker_case(ZSTR_VAL(pc->method_name), ZSTR_VAL(member_str), 1)) {
                zend_hash_index_del(class_pointcuts, h);
                continue;
            }
        }
        // Scope
        if (pc->static_state != 2 || pc->scope != 0) {
            if (!test_property_scope(pc, object->ce, member_str)) {
                zend_hash_index_del(class_pointcuts, h);
                continue;
            }
        }
    } ZEND_HASH_FOREACH_END(); 

    return class_pointcuts;
}

object_cache *get_object_cache (zend_object *object)
{
    uint32_t handle = object->handle;
    if (handle >= AOP_G(object_cache_size)) {
        AOP_G(object_cache) = erealloc(AOP_G(object_cache), sizeof(object_cache)*handle + 1);
        for (int i = AOP_G(object_cache_size); i <= handle; i++) {
            AOP_G(object_cache)[i] = NULL;
        }
        AOP_G(object_cache_size) = (int) handle + 1;
    }
    if (AOP_G(object_cache)[handle] == NULL) {
        AOP_G(object_cache)[handle] = emalloc(sizeof(object_cache));
        AOP_G(object_cache)[handle]->write = NULL;
        AOP_G(object_cache)[handle]->read = NULL;
        AOP_G(object_cache)[handle]->func = NULL;
    }
    return AOP_G(object_cache)[handle];
}

/* get_object_cache_func/read/write */
zend_array *get_object_cache_func(zend_object *object)
{
    object_cache *cache;
    cache = get_object_cache(object);
    if (cache->func == NULL) {
        ALLOC_HASHTABLE(cache->func);
        zend_hash_init(cache->func, 16, NULL, free_pointcut_cache , 0);
    }
    return cache->func;
}

zend_array *get_object_cache_read(zend_object *object)
{
    object_cache *cache;
    cache = get_object_cache(object);
    if (cache->read == NULL) {
        ALLOC_HASHTABLE(cache->read);
        zend_hash_init(cache->read, 16, NULL, free_pointcut_cache, 0);
    }
    return cache->read;
}

zend_array *get_object_cache_write(zend_object *object)
{
    object_cache *cache;
    cache = get_object_cache(object);
    if (cache->write == NULL) {
        ALLOC_HASHTABLE(cache->write);
        zend_hash_init(cache->write, 16, NULL, free_pointcut_cache , 0);
    }
    return cache->write;
}

/**
 * Find the pointcut of a function/method by cache or function name
 *
 * @param ex
 * @return zend_array
 */
static zend_array *find_pointcuts(zend_execute_data *ex)
{
    zend_object *object = NULL;
    if (Z_TYPE(ex->This) == IS_OBJECT) {
        object = Z_OBJ(ex->This);
    }

    // 1. Search cache
    zend_array *ht_object_cache = NULL;
    zend_class_entry *class_entry;
    zend_string *cache_key;
    if (object == NULL) { // Function or static method
        ht_object_cache = AOP_G(function_cache);
        if (ex->func->common.fn_flags & ZEND_ACC_STATIC) {
            class_entry = ex->func->common.scope; // ex->called_scope;
            cache_key = zend_string_init(
                "",
                ZSTR_LEN(ex->func->common.function_name) + ZSTR_LEN(class_entry->name) + 2, 0
            );
            sprintf(
                (char *) ZSTR_VAL(cache_key),
                "%s::%s",
                ZSTR_VAL(class_entry->name),
                ZSTR_VAL(ex->func->common.function_name)
            );
        } else {
            cache_key = zend_string_copy(ex->func->common.function_name);
        }
    }

    // Method
    else {
        class_entry = ex->func->common.scope; // ex->called_scope;
        cache_key = zend_string_copy(ex->func->common.function_name);
        ht_object_cache = get_object_cache_func(object);
    }

    zval *cache = zend_hash_find(ht_object_cache, cache_key);

    pointcut_cache *_pointcut_cache = NULL;
    if (cache != NULL) {
        _pointcut_cache = (pointcut_cache *) Z_PTR_P(cache);
        if (_pointcut_cache->version != AOP_G(pointcut_version) || (object != NULL && _pointcut_cache->class_entry != class_entry)) {
            // Cache lost
            _pointcut_cache = NULL;
            zend_hash_del(ht_object_cache, cache_key);
        }
    }

    // 2. Calculate function hit pointcut
    if (_pointcut_cache == NULL) {
        _pointcut_cache = (pointcut_cache *) emalloc(sizeof(pointcut_cache));
        _pointcut_cache->hash_table = calculate_function_pointcuts(ex);
        _pointcut_cache->version = AOP_G(pointcut_version);

        if (object == NULL) {
            _pointcut_cache->class_entry = NULL;
        } else {
            _pointcut_cache->class_entry = class_entry;
        }
        zval pointcut_cache_value;
        ZVAL_PTR(&pointcut_cache_value, _pointcut_cache);

        zend_hash_add(ht_object_cache, cache_key, &pointcut_cache_value);
    }
    zend_string_release(cache_key);

    return _pointcut_cache->hash_table;
}

static zend_array *get_cache_property(zend_object *object, zend_string *member, pointcut_type type)
{
    zend_array *ht_object_cache = NULL;
    if (type & AOP_KIND_READ) {
        ht_object_cache = get_object_cache_read(object);
    } else {
        ht_object_cache = get_object_cache_write(object);
    }

    zval *cache = zend_hash_find(ht_object_cache, member);

    pointcut_cache *_pointcut_cache = NULL;
    if (cache != NULL) {
        _pointcut_cache = (pointcut_cache *) Z_PTR_P(cache);
        if (_pointcut_cache->version != AOP_G(pointcut_version) || _pointcut_cache->class_entry != object->ce) {
            // Cache lost
            _pointcut_cache = NULL;
            zend_hash_del(ht_object_cache, member);
        }
    }
    if (_pointcut_cache == NULL) {
        _pointcut_cache = (pointcut_cache *) emalloc(sizeof(pointcut_cache));
        _pointcut_cache->hash_table = calculate_property_pointcuts(object, member, type);
        _pointcut_cache->version = AOP_G(pointcut_version);
        _pointcut_cache->class_entry = object->ce;

        zval pointcut_cache_value;
        ZVAL_PTR(&pointcut_cache_value, _pointcut_cache);
        zend_hash_add(ht_object_cache, member, &pointcut_cache_value);
    }
    zend_string_release(member);
    
    return _pointcut_cache->hash_table;
}

/**
 * Execute the pointcut
 *
 * @param pc
 * @param arg
 * @param retval
 */
static void execute_pointcut(pointcut *pc, zval *arg, zval *retval)
{
    zval params[1];
    
    ZVAL_COPY_VALUE(&params[0], arg);

    pc->fcall_info.retval = retval;
    pc->fcall_info.param_count = 1;
    pc->fcall_info.params = params;

    if (zend_call_function(&pc->fcall_info, &pc->fcall_info_cache) == FAILURE) {
        zend_error(E_ERROR, "Problem in AOP Callback");
    }
}

static void execute_context(zend_execute_data *execute_data, zval *args)
{
    if (EG(exception)) {
        return;
    }

    // Overload arguments
    if (args != NULL) {
        zval *original_args_value;
        zval *overload_args_value;
        zend_op_array *op_array = &EX(func)->op_array;

        uint32_t first_extra_arg = op_array->num_args;
        uint32_t call_num_args = zend_hash_num_elements(Z_ARR_P(args)); // ZEND_CALL_NUM_ARGS(execute_data);

        uint32_t i;
        if (call_num_args <= first_extra_arg) {
            for (i = 0; i < call_num_args; i++) {
                original_args_value = ZEND_CALL_VAR_NUM(execute_data, i);
                overload_args_value = zend_hash_index_find(Z_ARR_P(args), (zend_ulong)i);
                
                zval_ptr_dtor(original_args_value);
                ZVAL_COPY(original_args_value, overload_args_value);
            }
        } else {
            // 1. overload common params
            for (i = 0; i < first_extra_arg; i++) {
                original_args_value = ZEND_CALL_VAR_NUM(execute_data, i);
                overload_args_value = zend_hash_index_find(Z_ARR_P(args), (zend_ulong) i);

                zval_ptr_dtor(original_args_value);
                ZVAL_COPY(original_args_value, overload_args_value);
            }

            // 2. overload extra params
            if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
                for (i = 0; i < call_num_args - first_extra_arg; i++) {
                    original_args_value = ZEND_CALL_VAR_NUM(execute_data, op_array->last_var + op_array->T + i);
                    overload_args_value = zend_hash_index_find(
                        Z_ARR_P(args),
                        (zend_ulong) (int) (i + first_extra_arg)
                    );

                    zval_ptr_dtor(original_args_value);
                    ZVAL_COPY(original_args_value, overload_args_value);
                }
            }
        }
        ZEND_CALL_NUM_ARGS(execute_data) = call_num_args;
    }
    
    EG(current_execute_data) = execute_data;

    if (execute_data->func->common.type == ZEND_USER_FUNCTION) {
        original_zend_execute_ex(execute_data);
    } else if (execute_data->func->common.type == ZEND_INTERNAL_FUNCTION) {
        // zval return_value;
        if (original_zend_execute_internal) {
            original_zend_execute_internal(execute_data, execute_data->return_value);
        } else {
            execute_internal(execute_data, execute_data->return_value);
        }
    } else { // ZEND_OVERLOADED_FUNCTION
        zend_do_fcall_overloaded(execute_data, execute_data->return_value);
    }
}

/**
 * Execute the pointcut
 *
 * @param pos
 * @param pointcut_table
 * @param ex
 * @param aop_object
 */
void do_func_execute(HashPosition pos, zend_array *pointcut_table, zend_execute_data *ex, zval *aop_object)
{
    zval *current_pc_value = NULL;
    while (1) {
        current_pc_value = zend_hash_get_current_data_ex(pointcut_table, &pos);
        if (current_pc_value == NULL || Z_TYPE_P(current_pc_value) != IS_UNDEF) {
            break;
        } else {
            zend_hash_move_forward_ex(pointcut_table, &pos);
        }
    }

    // Execute original function if no entries
    AopJoinpoint_object *joinpoint = (AopJoinpoint_object *) Z_OBJ_P(aop_object);
    if (current_pc_value == NULL) {
        AOP_G(overloaded) = 0;
        execute_context(ex, joinpoint->args);
        AOP_G(overloaded) = 1;

        joinpoint->is_ex_executed = 1;

        return;
    }
    zend_hash_move_forward_ex(pointcut_table, &pos);

    pointcut *current_pc = (pointcut *) Z_PTR_P(current_pc_value);

    joinpoint->current_pointcut = current_pc;
    joinpoint->pos = pos;
    joinpoint->kind_of_advice = current_pc->kind_of_advice;

    zval pointcut_ret;

    // Before
    if (current_pc->kind_of_advice & AOP_KIND_BEFORE) {
        if (!EG(exception)) {
            execute_pointcut(current_pc, aop_object, &pointcut_ret);
            if (!Z_ISNULL(pointcut_ret)) {
                zval_ptr_dtor(&pointcut_ret);
            }
        }
    }

    // Around
    if (current_pc->kind_of_advice & AOP_KIND_AROUND) {
        if (!EG(exception)) {
            execute_pointcut(current_pc, aop_object, &pointcut_ret);
            if (!Z_ISNULL(pointcut_ret)) {
                if (ex->return_value != NULL) {
                    zval_ptr_dtor(ex->return_value);
                    ZVAL_COPY_VALUE(ex->return_value, &pointcut_ret);
                } else {
                    zval_ptr_dtor(&pointcut_ret);
                }
            } else if (joinpoint->return_value != NULL) {
                if (ex->return_value != NULL) {
                    zval_ptr_dtor(ex->return_value);
                    ZVAL_COPY(ex->return_value, joinpoint->return_value);
                }
            }
        }
    } else {
        do_func_execute(pos, pointcut_table, ex, aop_object);
    }

    // After
    if (current_pc->kind_of_advice & AOP_KIND_AFTER) {
        if (current_pc->kind_of_advice & AOP_KIND_CATCH && EG(exception)) {
            zend_object *exception = EG(exception);
            joinpoint->exception = exception;
            EG(exception) = NULL;
            execute_pointcut(current_pc, aop_object, &pointcut_ret);
            EG(exception) = exception;

            if (!Z_ISNULL(pointcut_ret)) {
                if (ex->return_value != NULL) {
                    zval_ptr_dtor(ex->return_value);
                    ZVAL_COPY_VALUE(ex->return_value, &pointcut_ret);
                } else {
                    zval_ptr_dtor(&pointcut_ret);
                }
            } else if (joinpoint->return_value != NULL) {
                if (ex->return_value != NULL) {
                    zval_ptr_dtor(ex->return_value);
                    ZVAL_COPY(ex->return_value, joinpoint->return_value);
                }
            }

        } else if (current_pc->kind_of_advice & AOP_KIND_RETURN && !EG(exception)) {
            execute_pointcut(current_pc, aop_object, &pointcut_ret);
            
            if (!Z_ISNULL(pointcut_ret)) {
                if (ex->return_value != NULL) {
                    zval_ptr_dtor(ex->return_value);
                    ZVAL_COPY_VALUE(ex->return_value, &pointcut_ret);
                } else {
                    zval_ptr_dtor(&pointcut_ret);
                }
            } else if (joinpoint->return_value != NULL) {
                if (ex->return_value != NULL) {
                    zval_ptr_dtor(ex->return_value);
                    ZVAL_COPY(ex->return_value, joinpoint->return_value);
                }
            }
        }
    }
}

/**
 * Check if a PHP function/method has a pointcut,
 * then execute the function with or without the pointcut
 *
 * @param ex
 */
void find_pointcut_then_execute(zend_execute_data *ex)
{
    // Find pointcut of current call function
    zend_array *pointcut_table = find_pointcuts(ex);
    if (pointcut_table == NULL || zend_hash_num_elements(pointcut_table) == 0) {
        // If pointcut table empty, execute original function
        AOP_G(overloaded) = 0;
        execute_context(ex, NULL);
        AOP_G(overloaded) = 1;
        return;
    }

    HashPosition pos = 0;
    zend_hash_internal_pointer_reset_ex(pointcut_table, &pos);

    // Create a new joinpoint object
    zval aop_object;
    object_init_ex(&aop_object, aop_joinpoint_class_entry);
    AopJoinpoint_object *joinpoint = (AopJoinpoint_object *) Z_OBJ(aop_object);
    joinpoint->ex = ex;
    joinpoint->is_ex_executed = 0;
    joinpoint->advice = pointcut_table;
    joinpoint->exception = NULL;
    joinpoint->args = NULL;
    joinpoint->return_value = NULL;
    ZVAL_UNDEF(&joinpoint->property_value);

    if (EG(current_execute_data) == ex) {
        // Delay to execute call function, execute pointcut first
        EG(current_execute_data) = ex->prev_execute_data;
    }

    int no_ret = 0;
    if (ex->return_value == NULL) {
        no_ret = 1;
        ex->return_value = emalloc(sizeof(zval));
        ZVAL_UNDEF(ex->return_value);
    }

    do_func_execute(pos, pointcut_table, ex, &aop_object);

    if (no_ret == 1) {
        zval_ptr_dtor(ex->return_value);
        efree(ex->return_value);
    } else {
        if (joinpoint->return_value_changed && Z_ISREF_P(ex->return_value)) {
            zval *real_return_value = Z_REFVAL_P(ex->return_value);
            Z_TRY_ADDREF_P(real_return_value);
            zval_ptr_dtor(ex->return_value);
            ZVAL_COPY_VALUE(ex->return_value, real_return_value);
        }
    }

    if (joinpoint->is_ex_executed == 0) {
        for (int i = 0; i < ex->func->common.num_args; i++) {
            zval *original_args_value = ZEND_CALL_VAR_NUM(ex, i);
            zval_ptr_dtor(original_args_value);
        }
    }
    
    zval_ptr_dtor(&aop_object);
}

/**
 * Intercept every PHP function call
 *
 * @param ex
 */
ZEND_API void aop_execute_ex(zend_execute_data *ex)
{
    zend_function *function = ex->func;

    if (!AOP_G(aop_enable)
        || function == NULL
        || AOP_G(overloaded)
        || EG(exception)
        || function->common.function_name == NULL
        || function->common.type == ZEND_EVAL_CODE
        || function->common.fn_flags & ZEND_ACC_CLOSURE
    ) {
        // Execute original function
        return original_zend_execute_ex(ex);
    }

    AOP_G(overloaded) = 1;
    find_pointcut_then_execute(ex);
    AOP_G(overloaded) = 0;
}

/**
 * Intercept every PHP function call
 *
 * @param ex
 * @param return_value
 */
ZEND_API void aop_execute_internal(zend_execute_data *ex, zval *return_value)
{
    zend_function *function = ex->func;

    if (!AOP_G(aop_enable)
        || function == NULL
        || AOP_G(overloaded)
        || EG(exception)
        || function->common.function_name == NULL
    ) {
        // Execute original function
        if (original_zend_execute_internal) {
            return original_zend_execute_internal(ex, return_value);
        } else {
            return execute_internal(ex, return_value);
        }
    }

    ex->return_value = return_value;

    AOP_G(overloaded) = 1;
    find_pointcut_then_execute(ex);
    AOP_G(overloaded) = 0;
}

void do_read_property(HashPosition pos, zend_array *pointcut_table, zval *aop_object)
{
    zval *current_pc_value = NULL;
    while (1) {
        current_pc_value = zend_hash_get_current_data_ex(pointcut_table, &pos);
        if (current_pc_value == NULL || Z_TYPE_P(current_pc_value) != IS_UNDEF) {
            break;
        } else {
            zend_hash_move_forward_ex(pointcut_table, &pos);
        }
    }

    AopJoinpoint_object *joinpoint = (AopJoinpoint_object *) Z_OBJ_P(aop_object);
    if (current_pc_value == NULL) {
        zend_class_entry *current_scope = NULL;
        if (EG(fake_scope) != joinpoint->ex->func->common.scope) {
            current_scope = EG(fake_scope);
            EG(fake_scope) = joinpoint->ex->func->common.scope;
        }

        zval *property_value = original_zend_std_read_property(
            joinpoint->object,
            joinpoint->member,
            joinpoint->type,
            joinpoint->cache_slot,
            joinpoint->rv
        );
        ZVAL_COPY_VALUE(AOP_G(property_value), property_value);

        if (current_scope != NULL) {
            EG(fake_scope) = current_scope;
        }
        return;
    }
    zend_hash_move_forward_ex(pointcut_table, &pos);

    pointcut *current_pc = (pointcut *) Z_PTR_P(current_pc_value);

    joinpoint->current_pointcut = current_pc;
    joinpoint->pos = pos;
    joinpoint->kind_of_advice = (current_pc->kind_of_advice & AOP_KIND_WRITE)
        ? (current_pc->kind_of_advice - AOP_KIND_WRITE)
        : current_pc->kind_of_advice;

    zval pointcut_ret;
    if (current_pc->kind_of_advice & AOP_KIND_BEFORE) {
        execute_pointcut(current_pc, aop_object, &pointcut_ret);
        if (!Z_ISNULL(pointcut_ret)) {
            zval_ptr_dtor(&pointcut_ret);
        }
    }

    if (current_pc->kind_of_advice & AOP_KIND_AROUND) {
        execute_pointcut(current_pc, aop_object, &pointcut_ret);
        if (!Z_ISNULL(pointcut_ret)) {
            ZVAL_COPY_VALUE(AOP_G(property_value), &pointcut_ret);
        } else if (joinpoint->return_value != NULL) {
            ZVAL_COPY(AOP_G(property_value), joinpoint->return_value);
        }
    } else {
        do_read_property(pos, pointcut_table, aop_object);
    }

    if (current_pc->kind_of_advice & AOP_KIND_AFTER) {
        execute_pointcut(current_pc, aop_object, &pointcut_ret);
        if (!Z_ISNULL(pointcut_ret)) {
            ZVAL_COPY_VALUE(AOP_G(property_value), &pointcut_ret);
        } else if (joinpoint->return_value != NULL) {
            ZVAL_COPY(AOP_G(property_value), joinpoint->return_value);
        }
    }
}

zval *aop_read_property(zend_object *object, zend_string *member, int type, void **cache_slot, zval *rv)
{
    if (AOP_G(lock_read_property) > 25) {
        zend_error(E_ERROR, "Too many level of nested advices. Are there any recursive calls?");
    }

    zend_array *pointcut_table = get_cache_property(object, member, AOP_KIND_READ);
    if (pointcut_table == NULL || zend_hash_num_elements(pointcut_table) == 0) {
        return original_zend_std_read_property(object, member, type, cache_slot, rv);
    }
    zend_hash_internal_pointer_reset_ex(pointcut_table, 0);

    zval aop_object;
    object_init_ex(&aop_object, aop_joinpoint_class_entry);
    AopJoinpoint_object *joinpoint = (AopJoinpoint_object *) Z_OBJ(aop_object);
    joinpoint->advice = pointcut_table;
    joinpoint->ex = EG(current_execute_data);
    joinpoint->args = NULL;
    joinpoint->return_value = NULL;
    joinpoint->object = object;
    joinpoint->member = member;
    joinpoint->type = type;
    // To avoid use runtime cache
    joinpoint->cache_slot = NULL; // cache_slot;
    joinpoint->rv = rv;
    
    ZVAL_UNDEF(&joinpoint->property_value);

    if (AOP_G(property_value) == NULL) {
        AOP_G(property_value) = emalloc(sizeof(zval));
    }
    ZVAL_NULL(AOP_G(property_value));

    AOP_G(lock_read_property)++;
    do_read_property(0, pointcut_table, &aop_object);
    AOP_G(lock_read_property)--;

    zval_ptr_dtor(&aop_object);
    return AOP_G(property_value);
}

void do_write_property(HashPosition pos, zend_array *pointcut_table, zval *aop_object)
{
    zval *current_pc_value;

    while (1) {
        current_pc_value = zend_hash_get_current_data_ex(pointcut_table, &pos);
        if (current_pc_value == NULL || Z_TYPE_P(current_pc_value) != IS_UNDEF) {
            break;
        } else {
            zend_hash_move_forward_ex(pointcut_table, &pos);
        }
    }

    AopJoinpoint_object *joinpoint = (AopJoinpoint_object *) Z_OBJ_P(aop_object);
    if (current_pc_value == NULL) {
        zend_class_entry *current_scope;
        if (EG(fake_scope) != joinpoint->ex->func->common.scope) {
            current_scope = EG(fake_scope);
            EG(fake_scope) = joinpoint->ex->func->common.scope;
        }

        original_zend_std_write_property(
            joinpoint->object,
            joinpoint->member,
            &joinpoint->property_value,
            joinpoint->cache_slot
        );

        if (current_scope != NULL) {
            EG(fake_scope) = current_scope;
        }
    }
    zend_hash_move_forward_ex(pointcut_table, &pos);

    pointcut *current_pc = (pointcut *) Z_PTR_P(current_pc_value);

    joinpoint->current_pointcut = current_pc;
    joinpoint->pos = pos;
    joinpoint->kind_of_advice = (current_pc->kind_of_advice & AOP_KIND_READ)
        ? (current_pc->kind_of_advice - AOP_KIND_READ)
        : current_pc->kind_of_advice;

    zval pointcut_ret;
    if (current_pc->kind_of_advice & AOP_KIND_BEFORE) {
        execute_pointcut(current_pc, aop_object, &pointcut_ret);
        if (!Z_ISNULL(pointcut_ret)) {
            zval_ptr_dtor(&pointcut_ret);
        }
    }

    if (current_pc->kind_of_advice & AOP_KIND_AROUND) {
        execute_pointcut(current_pc, aop_object, &pointcut_ret);
    } else {
        do_write_property(pos, pointcut_table, aop_object);
    }

    if (current_pc->kind_of_advice & AOP_KIND_AFTER) {
        execute_pointcut(current_pc, aop_object, &pointcut_ret);
    }
}

zval *aop_write_property(zend_object *object, zend_string *member, zval *value, void **cache_slot)
{
    if (AOP_G(lock_write_property) > 25) {
        zend_error(E_ERROR, "Too many level of nested advices. Are there any recursive calls?");
        return value;
    }

    zend_array *pointcut_table = get_cache_property(object, member, AOP_KIND_WRITE);
    if (pointcut_table == NULL || zend_hash_num_elements(pointcut_table) == 0) {
        return original_zend_std_write_property(object, member, value, cache_slot);
    }
    zend_hash_internal_pointer_reset_ex(pointcut_table, 0);

    zval aop_object;
    object_init_ex(&aop_object, aop_joinpoint_class_entry);
    AopJoinpoint_object *joinpoint = (AopJoinpoint_object *) Z_OBJ(aop_object);
    joinpoint->advice = pointcut_table;
    joinpoint->ex = EG(current_execute_data);
    joinpoint->args = NULL;
    joinpoint->return_value = NULL;
    joinpoint->object = object;
    joinpoint->member = member;
    // To avoid use runtime cache
    joinpoint->cache_slot = NULL; // cache_slot;

    ZVAL_COPY(&joinpoint->property_value, value);

    AOP_G(lock_write_property)++;
    do_write_property(0, pointcut_table, &aop_object);
    AOP_G(lock_write_property)--;

    zval_ptr_dtor(&aop_object);
    return joinpoint->return_value;
}

zval *aop_get_property_ptr_ptr(zend_object *object, zend_string *member, int type, void **cache_slot)
{
    zend_execute_data *ex = EG(current_execute_data);
    if (ex->opline == NULL
        || (ex->opline->opcode    != ZEND_PRE_INC_OBJ
            && ex->opline->opcode != ZEND_POST_INC_OBJ
            && ex->opline->opcode != ZEND_PRE_DEC_OBJ
            && ex->opline->opcode != ZEND_POST_DEC_OBJ)
    ) {
        return original_zend_std_get_property_ptr_ptr(object, member, type, cache_slot);
    }
    return NULL;
}