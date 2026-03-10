#include "bitcoin_lab_common.h"
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

static void compare_one(const char *title, const char *decoded_json_text) {
    cJSON *tx = cJSON_Parse(decoded_json_text);
    if (!tx) die("decoded JSON parse failed in compare");

    cJSON *txid = cJSON_GetObjectItem(tx, "txid");
    cJSON *size = cJSON_GetObjectItem(tx, "size");
    cJSON *vsize = cJSON_GetObjectItem(tx, "vsize");
    cJSON *weight = cJSON_GetObjectItem(tx, "weight");

    printf("\n=== %s ===\n", title);
    if (txid && cJSON_IsString(txid))   printf("txid   : %s\n", txid->valuestring);
    if (size && cJSON_IsNumber(size))   printf("size   : %d bytes\n", size->valueint);
    if (vsize && cJSON_IsNumber(vsize)) printf("vsize  : %d vbytes\n", vsize->valueint);
    if (weight && cJSON_IsNumber(weight)) printf("weight : %d wu\n", weight->valueint);

    cJSON_Delete(tx);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <legacy_txid> <segwit_txid>\n", argv[0]);
        fprintf(stderr, "This program assumes wallets:\n");
        fprintf(stderr, "  legacy wallet = cs216_legacy_wallet\n");
        fprintf(stderr, "  segwit wallet = cs216_segwit_wallet\n");
        return 1;
    }

    RpcConfig legacy_cfg = rpc_config_from_env("cs216_legacy_wallet");
    RpcConfig segwit_cfg = rpc_config_from_env("cs216_segwit_wallet");

    char *legacy_hex = gettransaction_hex_rpc(&legacy_cfg, argv[1]);
    char *segwit_hex = gettransaction_hex_rpc(&segwit_cfg, argv[2]);

    char *legacy_decoded = decoderawtransaction_rpc(&legacy_cfg, legacy_hex);
    char *segwit_decoded = decoderawtransaction_rpc(&segwit_cfg, segwit_hex);

    compare_one("LEGACY P2PKH", legacy_decoded);
    compare_one("P2SH-P2WPKH", segwit_decoded);

    printf("\nObservation:\n");
    printf("- SegWit usually has lower vsize/weight than legacy for similar spending logic.\n");
    printf("- In nested SegWit, most signature material moves to witness.\n");
    printf("- That is the main reason the effective size is reduced.\n");

    must_free(legacy_hex);
    must_free(segwit_hex);
    must_free(legacy_decoded);
    must_free(segwit_decoded);

    return 0;
}
