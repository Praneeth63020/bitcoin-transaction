# bitcoin-transaction
Bitcoin Transaction Lab

Legacy P2PKH and Nested SegWit (P2SH-P2WPKH) transaction creation using Bitcoin Core RPC in C.

This project demonstrates how Bitcoin transactions work internally by interacting with a local Bitcoin Core node running in regtest mode.
Two types of transactions are created and analyzed:

Legacy P2PKH transaction chain

SegWit P2SH-P2WPKH transaction chain

Finally, the two transaction types are compared based on transaction size, virtual size, and weight.

Tools and Technologies

C Programming Language

Bitcoin Core (regtest mode)

Bitcoin RPC API

MSYS2 + GCC

cJSON library

curl library

Git & GitHub

Project Structure
bitcoin-transaction
│
├── part1_legacy.c
├── part2_p2sh_p2wpkh.c
├── part3_compare.c
├── bitcoin_lab_common.c
├── bitcoin_lab_common.h
│
├── screenshots
│   ├── part1_execution.png
│   ├── part1_scriptsig.png
│   ├── part2_execution.png
│   ├── part2_witness.png
│   └── part3_comparison.png
│
└── README.md
Running Bitcoin Core (Regtest)

Start the Bitcoin node in regtest mode.

bitcoind.exe -regtest

Verify the node is running:

bitcoin-cli -regtest getblockchaininfo
Compilation

Compile the programs using GCC.

gcc part1_legacy.c bitcoin_lab_common.c -o part1_legacy -lcurl -lcjson
gcc part2_p2sh_p2wpkh.c bitcoin_lab_common.c -o part2_p2sh_p2wpkh -lcurl -lcjson
gcc part3_compare.c bitcoin_lab_common.c -o part3_compare -lcurl -lcjson
Part 1 — Legacy P2PKH Transactions

This program creates and broadcasts a chain of legacy P2PKH transactions.

Transaction flow:

Miner → Address A → Address B → Address C

Steps performed by the program:

Generate addresses

Mine blocks to obtain spendable coins

Fund address A

Create raw transaction A → B

Sign transaction

Broadcast transaction

Mine block to confirm

Create transaction B → C

Broadcast and confirm

Run the program:

./part1_legacy.exe

Example output includes:

Generated addresses

Raw transaction hex

Signed transaction

Transaction IDs

Screenshot to include

Take screenshot of:

program execution

addresses generated

transaction IDs

Place here:

screenshots/part1_execution.png

Add in README:

![Part1 Execution](screenshots/part1_execution.png)
Script Analysis (Legacy)

The program decodes the raw transaction and prints the scriptSig used for unlocking.

Example script:

OP_DUP OP_HASH160 <pubKeyHash> OP_EQUALVERIFY OP_CHECKSIG

This script verifies that the provided signature matches the public key hash.

Screenshot to include

Screenshot showing:

decoded transaction

scriptSig field

screenshots/part1_scriptsig.png

Add in README:

![Legacy ScriptSig](screenshots/part1_scriptsig.png)
Part 2 — Nested SegWit (P2SH-P2WPKH)

This program creates transactions using SegWit wrapped in P2SH.

Transaction flow:

Miner → Address A' → Address B' → Address C'

SegWit separates signature data from the main transaction body, which reduces transaction size.

Steps performed:

Generate SegWit addresses

Fund address A'

Create raw transaction A' → B'

Sign transaction

Broadcast transaction

Mine block to confirm

Create transaction B' → C'

Broadcast and confirm

Run the program:

./part2_p2sh_p2wpkh.exe
Screenshot to include

Program execution with addresses and txids.

screenshots/part2_execution.png

Add in README:

![SegWit Execution](screenshots/part2_execution.png)
SegWit Witness Analysis

SegWit transactions store signature data in a witness stack instead of scriptSig.

Example witness stack:

Signature
Public Key

This allows:

reduced transaction size

improved scalability

elimination of transaction malleability

Screenshot to include

Screenshot showing witness stack output.

screenshots/part2_witness.png

Add in README:

![SegWit Witness](screenshots/part2_witness.png)
Part 3 — Transaction Size Comparison

The third program compares Legacy vs SegWit transactions.

Run:

./part3_compare.exe <legacy_txid> <segwit_txid>

Example comparison:

Metric	Legacy	SegWit
Size (bytes)	225	247
Virtual Size	225	166
Weight	900	661

Observation:

SegWit transactions have lower virtual size and weight, which reduces transaction fees.
