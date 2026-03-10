#include "bitcoin_lab_common.h"

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

typedef struct {
    char *data;
    size_t size;
} Memory;

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real = size * nmemb;
    Memory *mem = (Memory *)userp;

    char *ptr = realloc(mem->data, mem->size + real + 1);
    if (!ptr) return 0;

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real);
    mem->size += real;
    mem->data[mem->size] = '\0';
    return real;
}

void die(const char *msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

void must_free(char *p) {
    if (p) free(p);
}

RpcConfig rpc_config_from_env(const char *wallet_name) {
    RpcConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    const char *user = getenv("BITCOIN_RPC_USER");
    const char *pass = getenv("BITCOIN_RPC_PASS");
    const char *host = getenv("BITCOIN_RPC_HOST");
    const char *port = getenv("BITCOIN_RPC_PORT");

    if (!user) user = "yourrpcuser";
    if (!pass) pass = "yourrpcpassword";
    if (!host) host = "127.0.0.1";
    if (!port) port = "18443";

    snprintf(cfg.auth, sizeof(cfg.auth), "%s:%s", user, pass);

    if (wallet_name && wallet_name[0] != '\0') {
        snprintf(cfg.wallet, sizeof(cfg.wallet), "%s", wallet_name);
        snprintf(cfg.url, sizeof(cfg.url), "http://%s:%s/wallet/%s", host, port, wallet_name);
    } else {
        cfg.wallet[0] = '\0';
        snprintf(cfg.url, sizeof(cfg.url), "http://%s:%s/", host, port);
    }

    return cfg;
}

char *rpc_call_text(const RpcConfig *cfg, const char *method, const char *params_json) {
    CURL *curl = curl_easy_init();
    if (!curl) die("curl_easy_init failed");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "jsonrpc", "1.0");
    cJSON_AddStringToObject(root, "id", "cs216");
    cJSON_AddStringToObject(root, "method", method);

    cJSON *params = NULL;
    if (params_json && strlen(params_json) > 0) {
        params = cJSON_Parse(params_json);
        if (!params) {
            cJSON_Delete(root);
            curl_easy_cleanup(curl);
            die("invalid params_json");
        }
    } else {
        params = cJSON_CreateArray();
    }
    cJSON_AddItemToObject(root, "params", params);

    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    Memory chunk;
    chunk.data = malloc(1);
    chunk.size = 0;
    if (!chunk.data) {
        free(body);
        curl_easy_cleanup(curl);
        die("malloc failed");
    }
    chunk.data[0] = '\0';

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "content-type: text/plain;");

    curl_easy_setopt(curl, CURLOPT_URL, cfg->url);
    curl_easy_setopt(curl, CURLOPT_USERPWD, cfg->auth);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
        free(body);
        free(chunk.data);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        die("RPC call failed");
    }

    free(body);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return chunk.data;
}

static cJSON *rpc_parse_and_check(const char *response_text) {
    cJSON *resp = cJSON_Parse(response_text);
    if (!resp) die("failed to parse RPC response");

    cJSON *err = cJSON_GetObjectItem(resp, "error");
    if (err && !cJSON_IsNull(err)) {
        char *errtxt = cJSON_Print(err);
        fprintf(stderr, "RPC returned error: %s\n", errtxt ? errtxt : "(unknown)");
        if (errtxt) free(errtxt);
        cJSON_Delete(resp);
        return NULL;
    }
    return resp;
}

char *rpc_result_string(const RpcConfig *cfg, const char *method, const char *params_json) {
    char *text = rpc_call_text(cfg, method, params_json);
    cJSON *resp = rpc_parse_and_check(text);
    free(text);

    if (!resp) return NULL;

    cJSON *result = cJSON_GetObjectItem(resp, "result");
    if (!cJSON_IsString(result)) {
        cJSON_Delete(resp);
        die("RPC result is not a string");
    }

    char *out = strdup(result->valuestring);
    cJSON_Delete(resp);
    return out;
}

double rpc_result_number(const RpcConfig *cfg, const char *method, const char *params_json) {
    char *text = rpc_call_text(cfg, method, params_json);
    cJSON *resp = rpc_parse_and_check(text);
    free(text);

    if (!resp) die("RPC numeric result failed");

    cJSON *result = cJSON_GetObjectItem(resp, "result");
    if (!cJSON_IsNumber(result)) {
        cJSON_Delete(resp);
        die("RPC result is not a number");
    }

    double out = result->valuedouble;
    cJSON_Delete(resp);
    return out;
}

void ensure_wallet_loaded(const RpcConfig *base_cfg, const char *wallet_name) {
    char params[256];

    snprintf(params, sizeof(params), "[\"%s\"]", wallet_name);
    char *resp = rpc_call_text(base_cfg, "loadwallet", params);
    cJSON *json = cJSON_Parse(resp);
    free(resp);

    if (!json) die("loadwallet parse failed");

    cJSON *err = cJSON_GetObjectItem(json, "error");
    if (!err || cJSON_IsNull(err)) {
        cJSON_Delete(json);
        return;
    }

    cJSON *code = cJSON_GetObjectItem(err, "code");
    cJSON *msg  = cJSON_GetObjectItem(err, "message");

    int errcode = code && cJSON_IsNumber(code) ? code->valueint : 0;
    const char *errmsg = msg && cJSON_IsString(msg) ? msg->valuestring : "";

    if (errcode == -35 && strstr(errmsg, "already loaded")) {
        cJSON_Delete(json);
        return;
    }

    if (errcode == -18 || strstr(errmsg, "not found")) {
        cJSON_Delete(json);

        snprintf(params, sizeof(params), "[\"%s\"]", wallet_name);
        char *create_resp = rpc_call_text(base_cfg, "createwallet", params);
        cJSON *create_json = rpc_parse_and_check(create_resp);
        free(create_resp);

        if (!create_json) die("createwallet failed");
        cJSON_Delete(create_json);
        return;
    }

    char *errtxt = cJSON_Print(err);
    fprintf(stderr, "Wallet error: %s\n", errtxt ? errtxt : "(unknown)");
    if (errtxt) free(errtxt);
    cJSON_Delete(json);
    die("ensure_wallet_loaded failed");
}

char *getnewaddress_rpc(const RpcConfig *cfg, const char *label, const char *addr_type) {
    char params[512];
    snprintf(params, sizeof(params), "[\"%s\", \"%s\"]", label, addr_type);
    return rpc_result_string(cfg, "getnewaddress", params);
}

void generatetoaddress_rpc(const RpcConfig *cfg, int blocks, const char *address) {
    char params[512];
    snprintf(params, sizeof(params), "[%d, \"%s\"]", blocks, address);

    char *resp = rpc_call_text(cfg, "generatetoaddress", params);
    cJSON *json = rpc_parse_and_check(resp);
    free(resp);
    if (!json) die("generatetoaddress failed");
    cJSON_Delete(json);
}

char *sendtoaddress_rpc(const RpcConfig *cfg, const char *address, double amount) {
    char params[512];
    snprintf(params, sizeof(params), "[\"%s\", %.8f]", address, amount);
    return rpc_result_string(cfg, "sendtoaddress", params);
}

UTXO find_largest_utxo_for_address(const RpcConfig *cfg, const char *address) {
    UTXO best;
    memset(&best, 0, sizeof(best));

    char params[512];
    snprintf(params, sizeof(params), "[1, 9999999, [\"%s\"]]", address);

    char *resp = rpc_call_text(cfg, "listunspent", params);
    cJSON *json = rpc_parse_and_check(resp);
    free(resp);

    if (!json) die("listunspent failed");

    cJSON *result = cJSON_GetObjectItem(json, "result");
    if (!cJSON_IsArray(result)) {
        cJSON_Delete(json);
        die("listunspent result not array");
    }

    double max_amt = -1.0;
    int n = cJSON_GetArraySize(result);
    for (int i = 0; i < n; i++) {
        cJSON *u = cJSON_GetArrayItem(result, i);
        cJSON *txid = cJSON_GetObjectItem(u, "txid");
        cJSON *vout = cJSON_GetObjectItem(u, "vout");
        cJSON *amt  = cJSON_GetObjectItem(u, "amount");
        cJSON *spk  = cJSON_GetObjectItem(u, "scriptPubKey");

        if (cJSON_IsString(txid) && cJSON_IsNumber(vout) && cJSON_IsNumber(amt)) {
            if (amt->valuedouble > max_amt) {
                max_amt = amt->valuedouble;
                snprintf(best.txid, sizeof(best.txid), "%s", txid->valuestring);
                best.vout = vout->valueint;
                best.amount = amt->valuedouble;
                if (spk && cJSON_IsString(spk)) {
                    snprintf(best.script_pub_key, sizeof(best.script_pub_key), "%s", spk->valuestring);
                } else {
                    best.script_pub_key[0] = '\0';
                }
            }
        }
    }

    cJSON_Delete(json);

    if (max_amt < 0.0) die("no UTXO found for address");
    return best;
}

char *createrawtransaction_single(const RpcConfig *cfg,
                                  const char *txid,
                                  int vout,
                                  const char *to_addr,
                                  double send_amount,
                                  const char *change_addr,
                                  double change_amount) {
    (void)cfg;

    cJSON *inputs = cJSON_CreateArray();
    cJSON *in = cJSON_CreateObject();
    cJSON_AddStringToObject(in, "txid", txid);
    cJSON_AddNumberToObject(in, "vout", vout);
    cJSON_AddItemToArray(inputs, in);

    cJSON *outputs = cJSON_CreateObject();
    cJSON_AddNumberToObject(outputs, to_addr, send_amount);
    if (change_amount > 0.0) {
        cJSON_AddNumberToObject(outputs, change_addr, change_amount);
    }

    char *inputs_txt = cJSON_PrintUnformatted(inputs);
    char *outputs_txt = cJSON_PrintUnformatted(outputs);

    cJSON_Delete(inputs);
    cJSON_Delete(outputs);

    char params[2048];
    snprintf(params, sizeof(params), "[%s, %s]", inputs_txt, outputs_txt);

    free(inputs_txt);
    free(outputs_txt);

    return rpc_result_string(cfg, "createrawtransaction", params);
}

char *decoderawtransaction_rpc(const RpcConfig *cfg, const char *rawhex) {
    char *escaped = malloc(strlen(rawhex) + 16);
    if (!escaped) die("malloc failed");
    snprintf(escaped, strlen(rawhex) + 16, "[\"%s\"]", rawhex);

    char *resp = rpc_call_text(cfg, "decoderawtransaction", escaped);
    free(escaped);

    cJSON *json = rpc_parse_and_check(resp);
    free(resp);
    if (!json) die("decoderawtransaction failed");

    cJSON *result = cJSON_GetObjectItem(json, "result");
    char *out = cJSON_Print(result);
    cJSON_Delete(json);
    return out;
}

char *signrawtransactionwithwallet_rpc(const RpcConfig *cfg, const char *rawhex) {
    char params[4096];
    snprintf(params, sizeof(params), "[\"%s\"]", rawhex);

    char *resp = rpc_call_text(cfg, "signrawtransactionwithwallet", params);
    cJSON *json = rpc_parse_and_check(resp);
    free(resp);
    if (!json) die("signrawtransactionwithwallet failed");

    cJSON *result = cJSON_GetObjectItem(json, "result");
    cJSON *hex = cJSON_GetObjectItem(result, "hex");
    cJSON *complete = cJSON_GetObjectItem(result, "complete");

    if (!cJSON_IsString(hex) || !cJSON_IsBool(complete) || !cJSON_IsTrue(complete)) {
        cJSON_Delete(json);
        die("signed transaction incomplete");
    }

    char *out = strdup(hex->valuestring);
    cJSON_Delete(json);
    return out;
}

char *sendrawtransaction_rpc(const RpcConfig *cfg, const char *signed_hex) {
    char params[4096];
    snprintf(params, sizeof(params), "[\"%s\"]", signed_hex);
    return rpc_result_string(cfg, "sendrawtransaction", params);
}

char *gettransaction_hex_rpc(const RpcConfig *cfg, const char *txid) {
    char params[512];
    snprintf(params, sizeof(params), "[\"%s\"]", txid);

    char *resp = rpc_call_text(cfg, "gettransaction", params);
    cJSON *json = rpc_parse_and_check(resp);
    free(resp);
    if (!json) die("gettransaction failed");

    cJSON *result = cJSON_GetObjectItem(json, "result");
    cJSON *hex = cJSON_GetObjectItem(result, "hex");
    if (!cJSON_IsString(hex)) {
        cJSON_Delete(json);
        die("gettransaction did not return hex");
    }

    char *out = strdup(hex->valuestring);
    cJSON_Delete(json);
    return out;
}

static const char *find_output_address(cJSON *script_pub_key) {
    cJSON *addr = cJSON_GetObjectItem(script_pub_key, "address");
    if (addr && cJSON_IsString(addr)) return addr->valuestring;

    cJSON *addrs = cJSON_GetObjectItem(script_pub_key, "addresses");
    if (addrs && cJSON_IsArray(addrs) && cJSON_GetArraySize(addrs) > 0) {
        cJSON *a0 = cJSON_GetArrayItem(addrs, 0);
        if (a0 && cJSON_IsString(a0)) return a0->valuestring;
    }
    return NULL;
}

void print_tx_summary(const char *decoded_json_text, const char *tag) {
    cJSON *tx = cJSON_Parse(decoded_json_text);
    if (!tx) die("decoded JSON parse failed");

    cJSON *txid = cJSON_GetObjectItem(tx, "txid");
    cJSON *size = cJSON_GetObjectItem(tx, "size");
    cJSON *vsize = cJSON_GetObjectItem(tx, "vsize");
    cJSON *weight = cJSON_GetObjectItem(tx, "weight");

    printf("\n===== %s =====\n", tag);
    if (txid && cJSON_IsString(txid))   printf("txid   : %s\n", txid->valuestring);
    if (size && cJSON_IsNumber(size))   printf("size   : %d\n", size->valueint);
    if (vsize && cJSON_IsNumber(vsize)) printf("vsize  : %d\n", vsize->valueint);
    if (weight && cJSON_IsNumber(weight)) printf("weight : %d\n", weight->valueint);

    cJSON_Delete(tx);
}

void print_vout_locking_script_for_address(const char *decoded_json_text, const char *address) {
    cJSON *tx = cJSON_Parse(decoded_json_text);
    if (!tx) die("decoded JSON parse failed");

    cJSON *vout_arr = cJSON_GetObjectItem(tx, "vout");
    if (!cJSON_IsArray(vout_arr)) {
        cJSON_Delete(tx);
        die("vout missing");
    }

    int n = cJSON_GetArraySize(vout_arr);
    for (int i = 0; i < n; i++) {
        cJSON *vout = cJSON_GetArrayItem(vout_arr, i);
        cJSON *spk = cJSON_GetObjectItem(vout, "scriptPubKey");
        if (!spk) continue;

        const char *out_addr = find_output_address(spk);
        if (out_addr && strcmp(out_addr, address) == 0) {
            cJSON *asmv = cJSON_GetObjectItem(spk, "asm");
            cJSON *hexv = cJSON_GetObjectItem(spk, "hex");
            cJSON *typev = cJSON_GetObjectItem(spk, "type");

            printf("\nLocking script for output address %s\n", address);
            if (typev && cJSON_IsString(typev)) printf("type : %s\n", typev->valuestring);
            if (asmv && cJSON_IsString(asmv))   printf("asm  : %s\n", asmv->valuestring);
            if (hexv && cJSON_IsString(hexv))   printf("hex  : %s\n", hexv->valuestring);
        }
    }

    cJSON_Delete(tx);
}

void print_first_input_unlocking_info(const char *decoded_json_text) {
    cJSON *tx = cJSON_Parse(decoded_json_text);
    if (!tx) die("decoded JSON parse failed");

    cJSON *vin_arr = cJSON_GetObjectItem(tx, "vin");
    if (!cJSON_IsArray(vin_arr) || cJSON_GetArraySize(vin_arr) == 0) {
        cJSON_Delete(tx);
        die("vin missing");
    }

    cJSON *vin0 = cJSON_GetArrayItem(vin_arr, 0);

    cJSON *scriptSig = cJSON_GetObjectItem(vin0, "scriptSig");
    if (scriptSig) {
        cJSON *asmv = cJSON_GetObjectItem(scriptSig, "asm");
        cJSON *hexv = cJSON_GetObjectItem(scriptSig, "hex");
        printf("\nInput 0 scriptSig\n");
        if (asmv && cJSON_IsString(asmv)) printf("asm  : %s\n", asmv->valuestring);
        if (hexv && cJSON_IsString(hexv)) printf("hex  : %s\n", hexv->valuestring);
    }

    cJSON *witness = cJSON_GetObjectItem(vin0, "txinwitness");
    if (witness && cJSON_IsArray(witness)) {
        printf("\nInput 0 witness stack\n");
        int wn = cJSON_GetArraySize(witness);
        for (int i = 0; i < wn; i++) {
            cJSON *item = cJSON_GetArrayItem(witness, i);
            if (item && cJSON_IsString(item)) {
                printf("[%d] %s\n", i, item->valuestring);
            }
        }
    }

    cJSON_Delete(tx);
}
