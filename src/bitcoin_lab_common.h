#ifndef BITCOIN_LAB_COMMON_H
#define BITCOIN_LAB_COMMON_H

#include <stdio.h>

typedef struct {
    char url[256];
    char auth[256];
    char wallet[128];
} RpcConfig;

typedef struct {
    char txid[128];
    int vout;
    double amount;
    char script_pub_key[512];
} UTXO;

RpcConfig rpc_config_from_env(const char *wallet_name);
void ensure_wallet_loaded(const RpcConfig *base_cfg, const char *wallet_name);

char *rpc_call_text(const RpcConfig *cfg, const char *method, const char *params_json);
char *rpc_result_string(const RpcConfig *cfg, const char *method, const char *params_json);
double rpc_result_number(const RpcConfig *cfg, const char *method, const char *params_json);

char *getnewaddress_rpc(const RpcConfig *cfg, const char *label, const char *addr_type);
void generatetoaddress_rpc(const RpcConfig *cfg, int blocks, const char *address);
char *sendtoaddress_rpc(const RpcConfig *cfg, const char *address, double amount);

UTXO find_largest_utxo_for_address(const RpcConfig *cfg, const char *address);
char *createrawtransaction_single(const RpcConfig *cfg,
                                  const char *txid,
                                  int vout,
                                  const char *to_addr,
                                  double send_amount,
                                  const char *change_addr,
                                  double change_amount);

char *decoderawtransaction_rpc(const RpcConfig *cfg, const char *rawhex);
char *signrawtransactionwithwallet_rpc(const RpcConfig *cfg, const char *rawhex);
char *sendrawtransaction_rpc(const RpcConfig *cfg, const char *signed_hex);
char *gettransaction_hex_rpc(const RpcConfig *cfg, const char *txid);

void print_tx_summary(const char *decoded_json_text, const char *tag);
void print_vout_locking_script_for_address(const char *decoded_json_text, const char *address);
void print_first_input_unlocking_info(const char *decoded_json_text);

void must_free(char *p);
void die(const char *msg);

#endif
