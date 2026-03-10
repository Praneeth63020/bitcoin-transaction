# bitcoin-transaction
Bitcoin Transaction Lab

Legacy P2PKH and Nested SegWit (P2SH-P2WPKH) transaction creation using Bitcoin Core RPC in C.

This project demonstrates the creation, signing, broadcasting, and analysis of Bitcoin transactions using a local Bitcoin Core node running in regtest mode. The goal of the assignment is to understand how Bitcoin transactions work internally and to compare the behavior and efficiency of Legacy transactions and SegWit transactions.

The project is implemented in the C programming language and communicates with Bitcoin Core through RPC calls. Using these RPC calls, the programs generate addresses, create raw transactions, sign them with the wallet, broadcast them to the network, and analyze their scripts and structures.

The lab is divided into three main parts.

Tools and Technologies Used

The following tools and technologies were used to implement and analyze the Bitcoin transactions:

Programming Language

C

Bitcoin Environment

Bitcoin Core

Regtest Mode

Libraries

libcurl (for RPC communication with Bitcoin Core)

cJSON (for parsing JSON responses)

Development Tools

MSYS2 / MinGW GCC compiler

Visual Studio Code

Version Control

Git

GitHub

Part 1 – Legacy P2PKH Transactions

The first program creates and broadcasts a chain of Legacy P2PKH transactions. In this process, three addresses are generated and transactions are created between them.

The transaction flow in this part is:

Miner → Address A → Address B → Address C

First, blocks are mined so that the wallet has mature coins in the regtest environment. The miner address then sends coins to Address A. After confirming the transaction by mining another block, the program identifies the UTXO belonging to Address A.

Using this UTXO, a raw transaction is created from Address A to Address B. The raw transaction is then signed by the wallet and broadcast to the network. After confirmation, the program identifies the UTXO belonging to Address B and creates another transaction from Address B to Address C.

During this process the raw transactions are decoded to analyze the structure of the transaction scripts. The locking script used in Legacy transactions follows the Pay-to-Public-Key-Hash (P2PKH) structure, which requires a valid signature and public key to unlock the funds.

Screenshots of the execution output and decoded script information are included in the screenshots folder.

Part 2 – Nested SegWit (P2SH-P2WPKH) Transactions

The second program implements transactions using SegWit wrapped inside P2SH addresses. This is called Nested SegWit.

The transaction flow in this part is:

Miner → Address A' → Address B' → Address C'

The process is similar to Part 1. First, blocks are mined to generate spendable coins. The miner funds Address A'. Once the funding transaction is confirmed, the program finds the UTXO belonging to Address A'.

A raw transaction is then created from Address A' to Address B'. This transaction is signed using the wallet and broadcast to the network. After confirmation, another transaction is created from Address B' to Address C'.

The major difference between Legacy and SegWit transactions appears in how signatures are stored. In SegWit transactions, the signature and public key are placed in a separate witness structure instead of inside the scriptSig field. This reduces the effective size of the transaction and improves efficiency.

Screenshots showing SegWit transaction execution and witness stack data are included in the screenshots folder.

Part 3 – Transaction Size Comparison

The third part of the lab compares the Legacy and SegWit transactions created in the previous parts. The comparison is performed using the transaction IDs generated during execution.

The analysis focuses on three key metrics:

Transaction size

Virtual size (vbytes)

Transaction weight

Legacy transactions store signatures inside the main transaction body. This increases the overall size of the transaction and leads to higher fees.

SegWit transactions move signature data into the witness structure. This reduces the effective size of the transaction and lowers the virtual size and weight. As a result, SegWit transactions are more efficient and allow more transactions to fit into a block.

The comparison results demonstrate that SegWit transactions achieve better scalability and lower transaction costs.

Screenshots of the comparison results are available in the screenshots folder.

Key Observations

Legacy transactions place signature data directly inside the transaction scripts. This increases transaction size and affects scalability.

SegWit separates signature data from the transaction body and stores it in the witness structure. This reduces the effective size of the transaction and improves network efficiency.

SegWit also solves the transaction malleability problem and enables additional improvements such as the Lightning Network.

Repository Structure

The repository is organized to clearly separate the source code, screenshots, and report.

bitcoin-transaction
│
├── README.md
│
├── src
│ ├── part1_legacy.c
│ ├── part2_p2sh_p2wpkh.c
│ ├── part3_compare.c
│ ├── bitcoin_lab_common.c
│ └── bitcoin_lab_common.h
│
├── screenshots
│ ├── part1_execution.png
│ ├── part1_scriptsig.png
│ ├── part2_execution.png
│ ├── part2_witness.png
│ └── part3_comparison.png
│
└── report
└── bitcoin_transaction_lab_report.pdf

The src folder contains the C source files used to implement the transaction programs.

The screenshots folder contains images showing execution outputs, script analysis, and transaction comparison results.

The report folder contains the detailed report for the lab assignment.

Conclusion

This lab provided practical experience with Bitcoin transaction construction and analysis. By implementing both Legacy and SegWit transaction chains, it becomes clear how different transaction formats affect efficiency and scalability.

The comparison between Legacy and SegWit transactions shows that SegWit provides clear advantages in terms of reduced transaction size, lower fees, and improved network performance.

Through this project, the internal structure of Bitcoin transactions, script execution, and the benefits of SegWit were explored in detail.
