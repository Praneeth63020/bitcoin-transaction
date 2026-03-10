#include "bitcoin_lab_common.h"
#include <stdlib.h>

int main(void) {
    const char *WALLET = "cs216_legacy_wallet";

    RpcConfig base = rpc_config_from_env(NULL);
    ensure_wallet_loaded(&base, WALLET);

    RpcConfig cfg = rpc_config_from_env(WALLET);

    printf("=== PART 1: LEGACY P2PKH ===\n");

    char *miner = getnewaddress_rpc(&cfg, "miner", "legacy");
    char *A = getnewaddress_rpc(&cfg, "A", "legacy");
    char *B = getnewaddress_rpc(&cfg, "B", "legacy");
    char *C = getnewaddress_rpc(&cfg, "C", "legacy");

    printf("Miner: %s\n", miner);
    printf("A    : %s\n", A);
    printf("B    : %s\n", B);
    printf("C    : %s\n", C);

    printf("\n[1] Mining 101 blocks so wallet has mature regtest coins...\n");
    generatetoaddress_rpc(&cfg, 101, miner);

    printf("[2] Funding A using sendtoaddress A...\n");
    char *fund_txid = sendtoaddress_rpc(&cfg, A, 10.00000000);
    printf("fund_txid(A funded) = %s\n", fund_txid);

    printf("[3] Mining 1 block to confirm funding of A...\n");
    generatetoaddress_rpc(&cfg, 1, miner);

    printf("[4] Finding UTXO for A...\n");
    UTXO a_utxo = find_largest_utxo_for_address(&cfg, A);
    printf("A UTXO: txid=%s vout=%d amount=%.8f\n", a_utxo.txid, a_utxo.vout, a_utxo.amount);

    double send_ab = 4.00000000;
    double fee_ab  = 0.00010000;
    double change_ab = a_utxo.amount - send_ab - fee_ab;
    if (change_ab <= 0.0) die("A->B change <= 0, adjust amounts");

    printf("[5] Creating raw transaction A -> B ...\n");
    char *raw_ab = createrawtransaction_single(&cfg, a_utxo.txid, a_utxo.vout, B, send_ab, A, change_ab);
    printf("raw_ab = %s\n", raw_ab);

    printf("[6] Decoding unsigned raw A -> B ...\n");
    char *decoded_raw_ab = decoderawtransaction_rpc(&cfg, raw_ab);
    print_tx_summary(decoded_raw_ab, "Unsigned A->B");
    print_vout_locking_script_for_address(decoded_raw_ab, B);

    printf("[7] Signing A -> B with wallet...\n");
    char *signed_ab = signrawtransactionwithwallet_rpc(&cfg, raw_ab);
    char *decoded_signed_ab = decoderawtransaction_rpc(&cfg, signed_ab);
    print_tx_summary(decoded_signed_ab, "Signed A->B");
    print_first_input_unlocking_info(decoded_signed_ab);

    printf("[8] Broadcasting A -> B ...\n");
    char *txid_ab = sendrawtransaction_rpc(&cfg, signed_ab);
    printf("txid_ab = %s\n", txid_ab);

    printf("[9] Mining 1 block to confirm A -> B ...\n");
    generatetoaddress_rpc(&cfg, 1, miner);

    printf("[10] Finding UTXO for B from previous transaction...\n");
    UTXO b_utxo = find_largest_utxo_for_address(&cfg, B);
    printf("B UTXO: txid=%s vout=%d amount=%.8f\n", b_utxo.txid, b_utxo.vout, b_utxo.amount);

    double send_bc = 2.50000000;
    double fee_bc  = 0.00010000;
    double change_bc = b_utxo.amount - send_bc - fee_bc;
    if (change_bc <= 0.0) die("B->C change <= 0, adjust amounts");

    printf("[11] Creating raw transaction B -> C ...\n");
    char *raw_bc = createrawtransaction_single(&cfg, b_utxo.txid, b_utxo.vout, C, send_bc, B, change_bc);
    printf("raw_bc = %s\n", raw_bc);

    printf("[12] Decoding unsigned raw B -> C ...\n");
    char *decoded_raw_bc = decoderawtransaction_rpc(&cfg, raw_bc);
    print_tx_summary(decoded_raw_bc, "Unsigned B->C");
    print_vout_locking_script_for_address(decoded_raw_bc, C);

    printf("[13] Signing B -> C with wallet...\n");
    char *signed_bc = signrawtransactionwithwallet_rpc(&cfg, raw_bc);
    char *decoded_signed_bc = decoderawtransaction_rpc(&cfg, signed_bc);
    print_tx_summary(decoded_signed_bc, "Signed B->C");
    print_first_input_unlocking_info(decoded_signed_bc);

    printf("[14] Broadcasting B -> C ...\n");
    char *txid_bc = sendrawtransaction_rpc(&cfg, signed_bc);
    printf("txid_bc = %s\n", txid_bc);

    printf("[15] Mining 1 block to confirm B -> C ...\n");
    generatetoaddress_rpc(&cfg, 1, miner);

    printf("\n=== LEGACY PART DONE ===\n");
    printf("Important txids for report:\n");
    printf("A -> B : %s\n", txid_ab);
    printf("B -> C : %s\n", txid_bc);

    must_free(miner);
    must_free(A);
    must_free(B);
    must_free(C);
    must_free(fund_txid);
    must_free(raw_ab);
    must_free(decoded_raw_ab);
    must_free(signed_ab);
    must_free(decoded_signed_ab);
    must_free(txid_ab);
    must_free(raw_bc);
    must_free(decoded_raw_bc);
    must_free(signed_bc);
    must_free(decoded_signed_bc);
    must_free(txid_bc);

    return 0;
}