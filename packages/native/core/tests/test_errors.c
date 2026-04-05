#include <string.h>
#include <stdlib.h>
#include "../jig_errors.h"
#include "test_util.h"

static void test_parse_error(void) {
    jig_error *err = jig_error_parse("unexpected token");
    ASSERT(err != NULL, "parse error should not be NULL");
    ASSERT(err->code == JIG_ERROR_PARSE_ERROR, "parse error code should be -32700");
    ASSERT(strcmp(err->message, "unexpected token") == 0, "parse error message");
    ASSERT(err->data == NULL, "parse error data should be NULL");
    jig_error_free(err);
}

static void test_invalid_request(void) {
    jig_error *err = jig_error_invalid_request("missing method");
    ASSERT(err->code == JIG_ERROR_INVALID_REQUEST, "invalid request code");
    ASSERT(strcmp(err->message, "missing method") == 0, "invalid request message");
    jig_error_free(err);
}

static void test_method_not_found(void) {
    jig_error *err = jig_error_method_not_found("dom.query");
    ASSERT(err->code == JIG_ERROR_METHOD_NOT_FOUND, "method not found code");
    ASSERT(strstr(err->message, "dom.query") != NULL, "method not found should contain method name");
    jig_error_free(err);
}

static void test_invalid_params(void) {
    jig_error *err = jig_error_invalid_params("selector is required");
    ASSERT(err->code == JIG_ERROR_INVALID_PARAMS, "invalid params code");
    ASSERT(strcmp(err->message, "selector is required") == 0, "invalid params message");
    jig_error_free(err);
}

static void test_internal_error(void) {
    jig_error *err = jig_error_internal("something broke");
    ASSERT(err->code == JIG_ERROR_INTERNAL_ERROR, "internal error code");
    ASSERT(strcmp(err->message, "something broke") == 0, "internal error message");
    jig_error_free(err);
}

static void test_handshake_required(void) {
    jig_error *err = jig_error_handshake_required();
    ASSERT(err->code == JIG_ERROR_HANDSHAKE_REQUIRED, "handshake required code");
    ASSERT(err->message != NULL, "handshake required should have a message");
    ASSERT(strlen(err->message) > 0, "handshake required message not empty");
    jig_error_free(err);
}

static void test_protocol_version(void) {
    jig_error *err = jig_error_protocol_version(99, 1, 1);
    ASSERT(err->code == JIG_ERROR_PROTOCOL_VERSION, "protocol version code");
    ASSERT(strstr(err->message, "99") != NULL, "protocol version message contains requested");
    ASSERT(err->data != NULL, "protocol version should have data");
    cJSON *supported = cJSON_GetObjectItem(err->data, "supported");
    ASSERT(supported != NULL, "data should have 'supported' key");
    cJSON *min = cJSON_GetObjectItem(supported, "min");
    ASSERT(min != NULL && cJSON_IsNumber(min), "supported should have min");
    ASSERT(min->valueint == 1, "supported min should be 1");
    jig_error_free(err);
}

static void test_element_not_found(void) {
    cJSON *data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "selector", "{\"testID\":\"foo\"}");
    jig_error *err = jig_error_element_not_found("no element matched", data);
    ASSERT(err->code == JIG_ERROR_ELEMENT_NOT_FOUND, "element not found code");
    ASSERT(strcmp(err->message, "no element matched") == 0, "element not found message");
    ASSERT(err->data != NULL, "element not found should have data");
    cJSON *sel = cJSON_GetObjectItem(err->data, "selector");
    ASSERT(sel != NULL, "data should have selector key");
    jig_error_free(err);
}

static void test_element_not_found_no_data(void) {
    jig_error *err = jig_error_element_not_found("not found", NULL);
    ASSERT(err->code == JIG_ERROR_ELEMENT_NOT_FOUND, "element not found code (no data)");
    ASSERT(err->data == NULL, "element not found data should be NULL");
    jig_error_free(err);
}

static void test_timeout(void) {
    jig_error *err = jig_error_timeout("timed out after 5s");
    ASSERT(err->code == JIG_ERROR_TIMEOUT, "timeout code");
    ASSERT(strcmp(err->message, "timed out after 5s") == 0, "timeout message");
    jig_error_free(err);
}

static void test_invalid_selector(void) {
    jig_error *err = jig_error_invalid_selector("bad selector syntax");
    ASSERT(err->code == JIG_ERROR_INVALID_SELECTOR, "invalid selector code");
    jig_error_free(err);
}

static void test_unsupported_domain(void) {
    jig_error *err = jig_error_unsupported_domain("network");
    ASSERT(err->code == JIG_ERROR_UNSUPPORTED_DOMAIN, "unsupported domain code");
    ASSERT(strstr(err->message, "network") != NULL, "unsupported domain contains domain name");
    jig_error_free(err);
}

static void test_domain_not_enabled(void) {
    jig_error *err = jig_error_domain_not_enabled("dom");
    ASSERT(err->code == JIG_ERROR_DOMAIN_NOT_ENABLED, "domain not enabled code");
    ASSERT(strstr(err->message, "dom") != NULL, "domain not enabled contains domain name");
    jig_error_free(err);
}

static void test_error_to_json(void) {
    jig_error *err = jig_error_parse("bad json");
    cJSON *json = jig_error_to_json(err);
    ASSERT(json != NULL, "error_to_json should not return NULL");

    cJSON *code = cJSON_GetObjectItem(json, "code");
    ASSERT(code != NULL && cJSON_IsNumber(code), "json should have numeric code");
    ASSERT(code->valueint == JIG_ERROR_PARSE_ERROR, "json code matches");

    cJSON *message = cJSON_GetObjectItem(json, "message");
    ASSERT(message != NULL && cJSON_IsString(message), "json should have string message");
    ASSERT(strcmp(message->valuestring, "bad json") == 0, "json message matches");

    /* no data key when data is NULL */
    cJSON *data = cJSON_GetObjectItem(json, "data");
    ASSERT(data == NULL, "json should not have data when error data is NULL");

    cJSON_Delete(json);
    jig_error_free(err);
}

static void test_error_to_json_with_data(void) {
    jig_error *err = jig_error_protocol_version(2, 1, 1);
    cJSON *json = jig_error_to_json(err);

    cJSON *data = cJSON_GetObjectItem(json, "data");
    ASSERT(data != NULL, "json should have data for protocol version error");
    cJSON *supported = cJSON_GetObjectItem(data, "supported");
    ASSERT(supported != NULL, "data should have supported");

    cJSON_Delete(json);
    jig_error_free(err);
}

int main(void) {
    test_parse_error();
    test_invalid_request();
    test_method_not_found();
    test_invalid_params();
    test_internal_error();
    test_handshake_required();
    test_protocol_version();
    test_element_not_found();
    test_element_not_found_no_data();
    test_timeout();
    test_invalid_selector();
    test_unsupported_domain();
    test_domain_not_enabled();
    test_error_to_json();
    test_error_to_json_with_data();

    if (failures > 0) {
        fprintf(stderr, "\n%d test(s) FAILED\n", failures);
        return 1;
    }
    printf("All error tests passed.\n");
    return 0;
}
