/* $Id$ */

#include "zookeeper_client.h"

// ---- Core functions ----

void register_zookeeper_client_class(TSRMLS_D)
{
    zend_class_entry class_entry;

    INIT_CLASS_ENTRY(class_entry, "ZookeeperClient", zookeeper_client_method_entry);
    zookeeper_client_class_entry = zend_register_internal_class_ex(&class_entry, NULL, NULL TSRMLS_CC);
    zookeeper_client_class_entry->create_object = zookeeper_client_create_object;
}

zend_object_value zookeeper_client_create_object(zend_class_entry *class_entry TSRMLS_DC)
{
    zend_object_value retval;
    zookeeper_client_storage_object *storage_object;

    storage_object = ecalloc(1, sizeof(*storage_object));
    zend_object_std_init( &storage_object->object, class_entry TSRMLS_CC );
#if PHP_VERSION_ID < 50399
	zend_hash_copy(storage_object->object.properties, &class_entry->default_properties, (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
#else
    object_properties_init( (zend_object *) storage_object->object, class_entry);
#endif

    retval.handle = zend_objects_store_put(storage_object, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)zookeeper_client_free_object, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

void zookeeper_client_free_object(zookeeper_client_storage_object *storage_object TSRMLS_DC)
{
    if (storage_object->zk_handle) {
        zookeeper_close(storage_object->zk_handle);
    }

    zend_object_std_dtor(&storage_object->object TSRMLS_CC);
    efree(storage_object);
}

// ---- Definitions ----

ZEND_BEGIN_ARG_INFO_EX(connect_arg_info, 0, 0, 1)
    ZEND_ARG_INFO(0, hosts)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(get_arg_info, 0, 0, 1)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

zend_function_entry zookeeper_client_method_entry[] = {
    PHP_ME(ZookeeperClient, connect, connect_arg_info, ZEND_ACC_PUBLIC)
    PHP_ME(ZookeeperClient, get, get_arg_info, ZEND_ACC_PUBLIC)

    { NULL, NULL, NULL }
};

// ---- PHP Methods ----

PHP_METHOD(ZookeeperClient, connect)
{
    zval *me = getThis();
    zookeeper_client_storage_object *storage;
    char *hosts = NULL;
    int hosts_len = 0;
    zhandle_t *zk_handle = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &hosts, &hosts_len) == FAILURE) {
        return;
    }

    zk_handle = zookeeper_init(hosts, NULL, 10, 0, NULL, 0);
    if (NULL == zk_handle) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Fail to initialize zookeeper client");
    }

    storage = zend_object_store_get_object(me TSRMLS_CC);
    storage->zk_handle = zk_handle;
}

PHP_METHOD(ZookeeperClient, get)
{
    zval *me = getThis();
    zookeeper_client_storage_object *storage;
    char *path = NULL;
    int path_len = 0;
    int response = ZOK;
    struct Stat stat;
    char *retval = NULL;
    int retval_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) {
        return;
    }

    storage = zend_object_store_get_object(me TSRMLS_CC);

    if (!storage->zk_handle) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "method 'connect' should be called before 'get'");
        return;
    }

    response = zoo_exists(storage->zk_handle, path, 1, &stat);
    if (response != ZOK) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Found error when calling zoo_exists");
        return;
    }

    /* Found NULL in node */
    if (stat.dataLength < 0)
        RETURN_NULL();

    retval = emalloc(stat.dataLength + 1);
    response = zoo_wget(storage->zk_handle, path, NULL, NULL, retval, &retval_len, &stat);
    if (response != ZOK) {
        efree(retval);
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Found error when calling zoo_wget");
        return;
    }
	/* Found NULL in node */
    if (retval_len <= 0) {
		RETURN_NULL();
    }

    retval[retval_len] = 0;
    RETURN_STRINGL(retval, retval_len, 0);
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
