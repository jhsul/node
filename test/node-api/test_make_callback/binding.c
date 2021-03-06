#include <node_api.h>
#include <assert.h>
#include "../../js-native-api/common.h"

#define MAX_ARGUMENTS 10

static napi_value MakeCallback(napi_env env, napi_callback_info info) {
  size_t argc = MAX_ARGUMENTS;
  size_t n;
  napi_value args[MAX_ARGUMENTS];
  // NOLINTNEXTLINE (readability/null_usage)
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

  NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

  napi_value recv = args[0];
  napi_value func = args[1];

  napi_value argv[MAX_ARGUMENTS - 2];
  for (n = 2; n < argc; n += 1) {
    argv[n - 2] = args[n];
  }

  napi_valuetype func_type;

  NAPI_CALL(env, napi_typeof(env, func, &func_type));

  napi_value resource_name;
  NAPI_CALL(env, napi_create_string_utf8(
      env, "test", NAPI_AUTO_LENGTH, &resource_name));

  napi_async_context context;
  NAPI_CALL(env, napi_async_init(env, func, resource_name, &context));

  napi_value result;
  if (func_type == napi_function) {
    NAPI_CALL(env, napi_make_callback(
        env, context, recv, func, argc - 2, argv, &result));
  } else {
    NAPI_ASSERT(env, false, "Unexpected argument type");
  }

  NAPI_CALL(env, napi_async_destroy(env, context));

  return result;
}

static void AsyncDestroyCb(napi_env env, void* data, void* hint) {
  napi_status status = napi_async_destroy(env, (napi_async_context)data);
  // We cannot use NAPI_ASSERT_RETURN_VOID because we need to have a JS stack
  // below in order to use exceptions.
  assert(status == napi_ok);
}

static napi_value CreateAsyncResource(napi_env env, napi_callback_info info) {
  napi_value object;
  NAPI_CALL(env, napi_create_object(env, &object));

  napi_value resource_name;
  NAPI_CALL(env, napi_create_string_utf8(
      env, "test_gcable", NAPI_AUTO_LENGTH, &resource_name));

  napi_async_context context;
  NAPI_CALL(env, napi_async_init(env, object, resource_name, &context));

  NAPI_CALL(env, napi_add_finalizer(
      env, object, (void*)context, AsyncDestroyCb, NULL, NULL));

  return object;
}

static
napi_value Init(napi_env env, napi_value exports) {
  napi_value fn;
  NAPI_CALL(env, napi_create_function(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, NAPI_AUTO_LENGTH, MakeCallback, NULL, &fn));
  NAPI_CALL(env, napi_set_named_property(env, exports, "makeCallback", fn));
  NAPI_CALL(env, napi_create_function(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, NAPI_AUTO_LENGTH, CreateAsyncResource, NULL, &fn));
  NAPI_CALL(env, napi_set_named_property(
      env, exports, "createAsyncResource", fn));
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
