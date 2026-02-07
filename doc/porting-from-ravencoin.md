# Porting Ravencoin Tools to Yottaflux — Comprehensive Guide

This guide documents every parameter, constant, and behavioral difference between Ravencoin (RVN) and Yottaflux (YAI). It is intended to be consumed by developers (or Claude Code) when adapting Ravencoin-based wallets, explorers, miners, libraries, and tools to work with the Yottaflux network.

> **Source of truth**: All values in this document are extracted directly from the Yottaflux v4.6.1 source code. When in doubt, consult `src/chainparams.cpp`, `src/amount.h`, and `src/validation.cpp`.

---

## Table of Contents

1. [Branding and Naming](#1-branding-and-naming)
2. [Network Parameters (Mainnet)](#2-network-parameters-mainnet)
3. [Network Parameters (Testnet)](#3-network-parameters-testnet)
4. [Network Parameters (Regtest)](#4-network-parameters-regtest)
5. [Address Encoding](#5-address-encoding)
6. [BIP44 Derivation Paths](#6-bip44-derivation-paths)
7. [Genesis Blocks](#7-genesis-blocks)
8. [Consensus and Difficulty](#8-consensus-and-difficulty)
9. [Block Subsidy and Halving Schedule](#9-block-subsidy-and-halving-schedule)
10. [Community Fund (Coinbase Tax)](#10-community-fund-coinbase-tax)
11. [Monetary Constants](#11-monetary-constants)
12. [Proof-of-Work Algorithm](#12-proof-of-work-algorithm)
13. [Asset System](#13-asset-system)
14. [Asset Burn Addresses](#14-asset-burn-addresses)
15. [Asset Burn Amounts](#15-asset-burn-amounts)
16. [Transaction and Policy Constants](#16-transaction-and-policy-constants)
17. [RPC Interface](#17-rpc-interface)
18. [Configuration Files and Data Directories](#18-configuration-files-and-data-directories)
19. [DNS Seeds and Fixed Seeds](#19-dns-seeds-and-fixed-seeds)
20. [Qt GUI Constants](#20-qt-gui-constants)
21. [Protocol Versioning](#21-protocol-versioning)
22. [Electrum-Specific Porting Notes](#22-electrum-specific-porting-notes)
23. [Explorer-Specific Porting Notes](#23-explorer-specific-porting-notes)
24. [Mining Software Porting Notes](#24-mining-software-porting-notes)
25. [Library/SDK Porting Notes](#25-librarysdk-porting-notes)
26. [Quick Reference: Search-and-Replace Table](#26-quick-reference-search-and-replace-table)

---

## 1. Branding and Naming

| Item | Ravencoin | Yottaflux |
|---|---|---|
| Full name | Ravencoin | Yottaflux |
| Ticker symbol | RVN | YAI |
| Smallest unit name | satoshi | corbie |
| Daemon binary | `ravend` | `yottafluxd` |
| CLI binary | `raven-cli` | `yottaflux-cli` |
| TX utility binary | `raven-tx` | `yottaflux-tx` |
| Qt GUI binary | `raven-qt` | `yottaflux-qt` |
| Organization name | Ravencoin | Yottaflux |
| Organization domain | ravencoin.org | yottaflux.ai |
| GitHub URL | github.com/RavenProject/Ravencoin | github.com/yottaflux/Yottaflux |
| Website | ravencoin.org | yottaflux.ai |
| Package name (autotools) | Raven | Yottaflux |

### Currency Denominations

| Unit | Ravencoin | Yottaflux | Value in corbies/satoshis |
|---|---|---|---|
| Base unit | RVN | YAI | 100,000,000 |
| Milli | mRVN | mYAI | 100,000 |
| Micro | μRVN | μYAI | 100 |
| Smallest | 1 satoshi | 1 corbie | 1 |

**Source**: `src/qt/yottafluxunits.h`, `src/amount.h`

---

## 2. Network Parameters (Mainnet)

| Parameter | Ravencoin | Yottaflux | Notes |
|---|---|---|---|
| **Magic bytes** | `0x5241564E` ("RAVN") | `0x594F5458` ("YOTX") | P2P message header |
| **Default port** | 8767 | 8559 | P2P listening port |
| **RPC port** | 8766 | 8558 | JSON-RPC port (typically default port - 1) |
| **Address prefix (P2PKH)** | 60 → starts with `R` | 78 → starts with `Y` | Base58 version byte |
| **Script prefix (P2SH)** | 122 → starts with `r` | 137 → starts with `x` | Base58 version byte |
| **Secret key prefix (WIF)** | 128 | 128 | Same as Ravencoin/Bitcoin |
| **Extended public key** | `0x0488B21E` | `0x0488B21E` | Same (xpub) |
| **Extended secret key** | `0x0488ADE4` | `0x0488ADE4` | Same (xprv) |
| **BIP44 coin type** | 175 | 5050 | HD derivation path |
| **nPruneAfterHeight** | 100000 | 100000 | Same |

**Source**: `src/chainparams.cpp` lines 116–289

---

## 3. Network Parameters (Testnet)

| Parameter | Ravencoin | Yottaflux | Notes |
|---|---|---|---|
| **Magic bytes** | `0x52564E54` ("RVNT") | `0x594F5454` ("YOTT") | |
| **Default port** | 18770 | 18559 | |
| **RPC port** | 18766 | 18558 | |
| **Address prefix (P2PKH)** | 111 → starts with `m`/`n` | 111 → starts with `m`/`n` | Same as Ravencoin testnet |
| **Script prefix (P2SH)** | 196 → starts with `2` | 196 → starts with `2` | Same |
| **Secret key prefix** | 239 | 239 | Same |
| **Extended public key** | `0x043587CF` | `0x043587CF` | Same (tpub) |
| **Extended secret key** | `0x04358394` | `0x04358394` | Same (tprv) |
| **BIP44 coin type** | 1 | 1 | Standard testnet coin type |
| **Network ID string** | `"test"` | `"test"` | |
| **Testnet data subdir** | `testnet7` | `testnet7` | |

**Source**: `src/chainparams.cpp` lines 295–517

---

## 4. Network Parameters (Regtest)

| Parameter | Ravencoin | Yottaflux | Notes |
|---|---|---|---|
| **Magic bytes** | `0x43524F57` ("CROW") | `0x594F5452` ("YOTR") | |
| **Default port** | 18444 | 18560 | |
| **RPC port** | 18443 | 18559 | |
| **Address prefix (P2PKH)** | 111 | 111 | Same |
| **Script prefix (P2SH)** | 196 | 196 | Same |
| **BIP44 coin type** | 1 | 1 | Standard regtest coin type |

**Source**: `src/chainparams.cpp` lines 523–738

---

## 5. Address Encoding

### Base58Check Version Bytes

| Type | Ravencoin Byte | Ravencoin Prefix | Yottaflux Byte | Yottaflux Prefix |
|---|---|---|---|---|
| P2PKH (mainnet) | 60 | `R` | 78 | `Y` |
| P2SH (mainnet) | 122 | `r` | 137 | `x` |
| WIF (mainnet) | 128 | `5`/`K`/`L` | 128 | `5`/`K`/`L` |
| P2PKH (testnet) | 111 | `m`/`n` | 111 | `m`/`n` |
| P2SH (testnet) | 196 | `2` | 196 | `2` |
| WIF (testnet) | 239 | `9`/`c` | 239 | `9`/`c` |

### Address Validation

When validating Yottaflux mainnet addresses:
- P2PKH addresses **must** start with `Y`
- P2SH addresses **must** start with `x`
- The version byte 78 constrains the second character of P2PKH addresses to the Base58 range `P`–`n` (approximately)

### Implementation Notes

In Python (e.g., Electrum):
```python
# Ravencoin
ADDRTYPE_P2PKH = 60
ADDRTYPE_P2SH = 122

# Yottaflux
ADDRTYPE_P2PKH = 78
ADDRTYPE_P2SH = 137
```

In JavaScript:
```javascript
// Ravencoin
const pubKeyHash = 0x3c;  // 60
const scriptHash = 0x7a;  // 122

// Yottaflux
const pubKeyHash = 0x4e;  // 78
const scriptHash = 0x89;  // 137
```

---

## 6. BIP44 Derivation Paths

| Network | Ravencoin | Yottaflux |
|---|---|---|
| Mainnet | `m/44'/175'/0'` | `m/44'/5050'/0'` |
| Testnet | `m/44'/1'/0'` | `m/44'/1'/0'` |

**Critical**: BIP44 coin type 5050 is Yottaflux-specific. Any HD wallet implementation must use this value for mainnet key derivation. Testnet uses the standard coin type 1 (same as Ravencoin testnet).

**Source**: `src/chainparams.cpp` — `nExtCoinType = 5050` (mainnet), `nExtCoinType = 1` (testnet/regtest)

---

## 7. Genesis Blocks

### Mainnet Genesis

| Field | Ravencoin | Yottaflux |
|---|---|---|
| **Timestamp string** | "The Times 03/Jan/2018 Bitcoin is name..." | "The Times 02/Feb/2023 US Job Growth Defies Expectations - Yottaflux powers augmented AI" |
| **nTime** | 1514999494 | 1707491747 |
| **nNonce** | 21584032 | 5751947 |
| **nBits** | 0x1e00ffff | 0x1e00ffff |
| **nVersion** | 4 | 4 |
| **Genesis reward** | 5000 RVN | 5000 YAI |
| **Hash algorithm** | GetX16RHash() | GetX16RV2Hash() |
| **Block hash** | `0x0000006b...` | `0x000000f13b9584fc6830148c59ec77e5671a5ac3ff0f26a9ae0679c0ca40f579` |
| **Merkle root** | `28ff00a867...` | `648156c6987915ad1270f7ffac0d57b7a2dd4fecaa0f59d059e0166213df2815` |

### Testnet Genesis

| Field | Yottaflux Value |
|---|---|
| **nTime** | 1707490166 |
| **nNonce** | 8439120 |
| **nBits** | 0x1e00ffff |
| **nVersion** | 2 |
| **Hash algorithm** | GetX16RV2Hash() |
| **Block hash** | `0x000000709fd50f9c838d68747b3b747da72c753ccc0733365220ff4cc5ea1c49` |
| **Merkle root** | `648156c6987915ad1270f7ffac0d57b7a2dd4fecaa0f59d059e0166213df2815` |

### Regtest Genesis

| Field | Yottaflux Value |
|---|---|
| **nTime** | 1707491605 |
| **nNonce** | 0 |
| **nBits** | 0x207fffff |
| **nVersion** | 4 |
| **Hash algorithm** | GetX16RV2Hash() |
| **Block hash** | `0x45a2ecb9ae084bcd083ac8ece09e30a239affd25ac2f3d3e7bb48463caa0d432` |
| **Merkle root** | `648156c6987915ad1270f7ffac0d57b7a2dd4fecaa0f59d059e0166213df2815` |

**Source**: `src/chainparams.cpp`

---

## 8. Consensus and Difficulty

| Parameter | Ravencoin | Yottaflux | Notes |
|---|---|---|---|
| **Block time target** | 60 seconds | 60 seconds | Same |
| **nPowTargetSpacing** | `1 * 60` | `1 * 60` | Same |
| **nPowTargetTimespan** | `2016 * 60` | `100 * 60` | **Changed**: 100 minutes vs 2016 minutes |
| **Difficulty adjustment** | DGW (Dark Gravity Wave) | DGW (Dark Gravity Wave) | Same algorithm |
| **DGW activation block** | 338778 | 1 (mainnet) | Active from the start |
| **BIP9 signaling window** | 2016 blocks | 2016 blocks | Same |
| **Activation threshold** | 1613 (~80% of 2016) | 1613 (~80% of 2016) | Same for mainnet |
| **fPowAllowMinDifficultyBlocks** | false (mainnet) | false (mainnet) | Same |
| **nMaxReorganizationDepth** | 60 | 60 | Same |
| **nMinReorganizationPeers** | 4 | 4 | Same |
| **nMinReorganizationAge** | 43200 (12 hours) | 43200 (12 hours) | Same |

### Difficulty Limits (powLimit)

| Network | Ravencoin | Yottaflux |
|---|---|---|
| Mainnet (X16RV2) | `00000fffff...` | `00ffffffffffff...` |
| Mainnet (KawPOW) | `00000000ff...` | `0000000fffff...` |
| Testnet | `00000fffff...` | `00000fffff...` |
| Regtest | `7fffff...` | `7fffff...` |

**Source**: `src/chainparams.cpp`

---

## 9. Block Subsidy and Halving Schedule

### Critical Difference

Ravencoin uses a **uniform halving interval** (`nSubsidyHalvingInterval = 2100000` blocks, ~4 years). Yottaflux uses a **hardcoded non-uniform halving table** with accelerating intervals.

### Yottaflux Halving Table

| Era | Block Range | Reward per Block | Shift | Approx. Duration |
|---|---|---|---|---|
| 0 | 0 – 129,599 | 5000 YAI | >> 0 | ~90 days |
| 1 | 129,600 – 187,199 | 2500 YAI | >> 1 | ~40 days |
| 2 | 187,200 – 270,719 | 1250 YAI | >> 2 | ~58 days |
| 3 | 270,720 – 391,679 | 625 YAI | >> 3 | ~84 days |
| 4 | 391,680 – 567,359 | 312.5 YAI | >> 4 | ~122 days |
| 5 | 567,360 – 822,239 | 156.25 YAI | >> 5 | ~177 days |
| 6 | 822,240 – 1,190,879 | 78.125 YAI | >> 6 | ~256 days |
| 7 | 1,190,880 – 1,726,559 | 39.0625 YAI | >> 7 | ~372 days |
| 8 | 1,726,560 – 2,502,719 | 19.53125 YAI | >> 8 | ~539 days |
| 9 | 2,502,720 – 3,628,799 | 9.765625 YAI | >> 9 | ~782 days |
| 10 | 3,628,800 – 5,261,759 | 4.8828125 YAI | >> 10 | ~1134 days |
| 11 | 5,261,760 – 7,629,119 | 2.44140625 YAI | >> 11 | ~1644 days |
| 12 | 7,629,120 – 11,062,079 | 1.220703125 YAI | >> 12 | ~2384 days |
| 13+ | 11,062,080+ | 0 YAI | — | No more rewards |

### Total Supply

| | Ravencoin | Yottaflux |
|---|---|---|
| **Total supply** | 21,000,000,000 RVN | ~1,160,550,000 YAI |
| **Total in smallest units** | 2,100,000,000,000,000,000 | ~116,055,000,000,000,000 |

### Implementation

The subsidy is **not** computed from `nSubsidyHalvingInterval`. It is a hardcoded if-chain in `GetBlockSubsidy()`:

```cpp
CAmount GetBlockSubsidy(int nHeight, const Consensus::Params& consensusParams)
{
    CAmount nSubsidy = 5000 * COIN;
    if (nHeight < 129600) return nSubsidy;
    if (nHeight < 187200) return nSubsidy >> 1;
    if (nHeight < 270720) return nSubsidy >> 2;
    // ... (see src/validation.cpp line 1322)
    if (nHeight < 11062080) return nSubsidy >> 12;
    return 0;
}
```

**For porting**: You must replicate this exact table. Do NOT use `nSubsidyHalvingInterval` to compute rewards — it is a vestigial field kept only for API compatibility.

**Source**: `src/validation.cpp` lines 1322–1342

---

## 10. Community Fund (Coinbase Tax)

### Overview

Yottaflux enforces a **40% community fund** deducted from every block's coinbase transaction. This is a consensus rule — blocks violating it are rejected.

| Fund | Percentage | Ravencoin Equivalent |
|---|---|---|
| Community Development | 10% of block reward | Does not exist in Ravencoin |
| Community Subsidy | 30% of block reward | Does not exist in Ravencoin |
| Miner reward | 60% of block reward | 100% in Ravencoin |

### Coinbase Transaction Structure

Every Yottaflux block coinbase **must** have at least 3 outputs (enforced at consensus):

| Output Index | Recipient | Amount |
|---|---|---|
| `vout[0]` | Miner | 60% of block reward |
| `vout[1]` | Community Development address | 10% of block reward |
| `vout[2]` | Community Subsidy address | 30% of block reward |

### Community Fund Addresses

| Network | Dev Address (10%) | Subsidy Address (30%) |
|---|---|---|
| Mainnet | `YcMZLejJUFSzXYqp5wqrqRyKg3ySMxzYte` | `YavbEessbLmxPPYMAjLH2kER4NyLoiDjcm` |
| Testnet | `n1MDTt2zQJ7vkHxnMudX7P67M76umc57FU` | `n3zrHVsrDA52JgvsnLmd7Y782VC9JtANcQ` |
| Regtest | `mhXBsq5cN3pqtXCqVACuva2CQNzHdYcmYC` | `n42rJAdeveUKP6PvQHKE6b55bYWzLeYtj7` |

### Impact on Porting

- **Block explorers**: Must display the 3-output coinbase structure and label outputs correctly
- **Mining pools**: Must construct coinbase transactions with the correct 3 outputs and correct percentages
- **Reward calculators**: Must apply the 60% miner share (not 100%)
- **Wallet software**: No impact on receiving/sending — this only affects block production and validation

**Source**: `src/validation.cpp` (ConnectBlock), `src/miner.cpp` (CreateNewBlock), `src/chainparams.cpp`

---

## 11. Monetary Constants

| Constant | Ravencoin | Yottaflux | Notes |
|---|---|---|---|
| **COIN** | 100,000,000 | 100,000,000 | Same (8 decimal places) |
| **CENT** | 1,000,000 | 1,000,000 | Same |
| **MAX_MONEY** | 21,000,000,000 × COIN | 1,200,000,000 × COIN | Reduced to match actual supply |
| **Genesis reward** | 5000 COIN | 5000 COIN | Same initial reward |

**Source**: `src/amount.h`

---

## 12. Proof-of-Work Algorithm

| Parameter | Ravencoin | Yottaflux |
|---|---|---|
| **Active PoW algorithm** | KawPOW (from block 1,219,736) | X16RV2 |
| **KawPOW status** | Active | **Disabled** (activation timestamp set to far-future: 3567587327 ≈ year 2083) |
| **X16RV2 status** | Was active before KawPOW | **Currently active** |
| **X16R status** | Was active before X16RV2 | Not used |
| **Genesis hash function** | GetX16RV2Hash() | GetX16RV2Hash() | Same |

### Key Implication

Mining software must use **X16RV2**, not KawPOW. The KawPOW activation timestamp (`nKAAAWWWPOWActivationTime = 3567587327`) is set ~57 years in the future, effectively disabling it.

On mainnet and testnet, KawPOW is disabled. On regtest, KawPOW activates immediately (`nGenesisTime + 1`).

**Source**: `src/chainparams.cpp` — `nKAAAWWWPOWActivationTime` and `nKAWPOWActivationTime`

---

## 13. Asset System

The Yottaflux asset system is inherited from Ravencoin's RIP2/RIP5 implementation. The following asset types and their properties are **identical in behavior** between Ravencoin and Yottaflux:

### Asset Types

| Type | Name Length | Divisibility | Reissuable | Has IPFS |
|---|---|---|---|---|
| Root/Main Asset | 3–30 chars | 0–8 decimals | Yes | Yes |
| Sub-Asset | parent + `/` + 1–30 chars | 0–8 decimals | Yes | Yes |
| Unique Asset | parent + `#` + 1–30 chars | 0 (indivisible) | No | Yes |
| Message Channel | parent + `~` + 1–30 chars | 0 | No | Yes |
| Qualifier | `#` + 1–30 chars | 0 | No | Yes |
| Sub-Qualifier | qualifier + `/#` + 1–30 chars | 0 | No | Yes |
| Restricted | `$` + 1–30 chars | 0–8 decimals | Yes | Yes |

### Asset Script Opcode

| | Ravencoin | Yottaflux |
|---|---|---|
| **Asset opcode** | `OP_RVN_ASSET` (0xc0) | `OP_YAI_ASSET` (0xc0) |
| **Script marker** | `rvn` (3 bytes after 0xc0) | `yai` (3 bytes after 0xc0) |

The opcode value is the same (`0xc0`), but the 3-byte identifier string following it changes from `rvn` to `yai`. This affects:
- Transaction parsing/serialization
- Script validation
- Any code that searches for the asset marker in scripts

### Asset Name Constraints

| Constraint | Value | Same as RVN? |
|---|---|---|
| MIN_ASSET_LENGTH | 3 | Yes |
| MAX_ASSET_LENGTH | 30 | Yes |
| Allowed characters | `A-Z`, `0-9`, `.`, `_` | Yes |
| Root asset min burn | 500 YAI | Yes |
| Max supply per asset | 21,000,000,000 units | Yes |

**Source**: `src/script/script.h`, `src/assets/assets.h`, `src/assets/assets.cpp`

---

## 14. Asset Burn Addresses

These are provably-unspendable addresses where fees for asset operations are sent (burned). Each address has a valid Base58Check checksum.

### Mainnet Burn Addresses

| Purpose | Ravencoin Address | Yottaflux Address |
|---|---|---|
| Issue Asset | `RXissueAssetXXXXXXXXXXXXXXXXXhhZGt` | `YissueAssetXXXXXXXXXXXXXXXXXW8oK1h` |
| Reissue Asset | `RXReissueAssetXXXXXXXXXXXXXXVEFAWu` | `YReissueAssetXXXXXXXXXXXXXXXYcNAB6` |
| Issue Sub-Asset | `RXissueSubAssetXXXXXXXXXXXXXWcwhwL` | `YissueSubAssetXXXXXXXXXXXXXXcAjBNU` |
| Issue Unique Asset | `RXissueUniqueAssetXXXXXXXXXXWEAe58` | `YissueUniqueAssetXXXXXXXXXXXZAr1F6` |
| Issue Msg Channel | `RXissueMsgChanneLAssetXXXXXXSjHvAY` | `YissueMsgChanneLAssetXXXXXXXdbjHqe` |
| Issue Qualifier | `RXissueQuaLifierXXXXXXXXXXXXUgEDbC` | `YissueQuaLifierXXXXXXXXXXXXXTQvwL8` |
| Issue Sub-Qualifier | `RXissueSubQuaLifierXXXXXXXXXVTq8mN` | `YissueSubQuaLifierXXXXXXXXXXYJchwm` |
| Issue Restricted | `RXissueRestrictedXXXXXXXXXXXXzJZ1q` | `YissueRestrictedXXXXXXXXXXXXUkSk3r` |
| Add Null Qualifier Tag | `RXaddTagBurnXXXXXXXXXXXXXXXXZQm5ya` | `YaddTagBurnXXXXXXXXXXXXXXXXXZJAYt2` |
| Global Burn | `RXBurnXXXXXXXXXXXXXXXXXXXXXXWUo9FV` | `YburnXXXXXXXXXXXXXXXXXXXXXXXYqtbxJ` |

### Testnet/Regtest Burn Addresses

| Purpose | Yottaflux Testnet/Regtest Address |
|---|---|
| Issue Asset | `n1issueAssetXXXXXXXXXXXXXXXXWdnemQ` |
| Reissue Asset | `n1ReissueAssetXXXXXXXXXXXXXXWG9NLd` |
| Issue Sub-Asset | `n1issueSubAssetXXXXXXXXXXXXXbNiH6v` |
| Issue Unique Asset | `n1issueUniqueAssetXXXXXXXXXXS4695i` |
| Issue Msg Channel | `n1issueMsgChanneLAssetXXXXXXT2PBdD` |
| Issue Qualifier | `n1issueQuaLifierXXXXXXXXXXXXUysLTj` |
| Issue Sub-Qualifier | `n1issueSubQuaLifierXXXXXXXXXYffPLh` |
| Issue Restricted | `n1issueRestrictedXXXXXXXXXXXXZVT9V` |
| Add Null Qualifier Tag | `n1addTagBurnXXXXXXXXXXXXXXXXX5oLMH` |
| Global Burn | `n1BurnXXXXXXXXXXXXXXXXXXXXXXU1qejP` |

**Generator tool**: `contrib/generate_burn_addresses.py` can regenerate these with `--version 78` (mainnet) or `--version 111` (testnet).

**Source**: `src/chainparams.cpp`

---

## 15. Asset Burn Amounts

These are identical between Ravencoin and Yottaflux:

| Operation | Burn Amount | Same as RVN? |
|---|---|---|
| Issue Asset (root) | 500 YAI | Yes |
| Reissue Asset | 100 YAI | Yes |
| Issue Sub-Asset | 100 YAI | Yes |
| Issue Unique Asset | 5 YAI | Yes |
| Issue Msg Channel | 100 YAI | Yes |
| Issue Qualifier | 1000 YAI | Yes |
| Issue Sub-Qualifier | 100 YAI | Yes |
| Issue Restricted Asset | 1500 YAI | Yes |
| Add Null Qualifier Tag | 0.1 YAI | Yes |

**Source**: `src/chainparams.cpp`

---

## 16. Transaction and Policy Constants

| Parameter | Ravencoin | Yottaflux | Same? |
|---|---|---|---|
| **MAX_BLOCK_WEIGHT** | 8,000,000 | 8,000,000 | Yes |
| **MAX_BLOCK_SERIALIZED_SIZE** | 8,000,000 | 8,000,000 | Yes |
| **DEFAULT_BLOCK_MAX_WEIGHT** | 8,000,000 | 8,000,000 | Yes |
| **MAX_STANDARD_TX_WEIGHT** | 400,000 | 400,000 | Yes |
| **DUST_RELAY_TX_FEE** | 1000 sat/kB | 1000 corbie/kB | Yes |
| **DEFAULT_MIN_RELAY_TX_FEE** | 1000 sat/kB | 1000 corbie/kB | Yes |
| **DEFAULT_TRANSACTION_MINFEE** | 1000 sat/kB | 1000 corbie/kB | Yes |
| **DEFAULT_FALLBACK_FEE** | 20000 sat/kB | 20000 corbie/kB | Yes |
| **DEFAULT_TRANSACTION_MAXFEE** | 1.0 COIN | 1.0 COIN | Yes |
| **LOCKTIME_VERIFY_SEQUENCE** | Enabled | Enabled | Yes |
| **SegWit** | Enabled | Enabled | Yes |
| **CSV** | Enabled | Enabled | Yes |

**Source**: `src/policy/policy.h`, `src/consensus/consensus.h`, `src/wallet/wallet.h`

---

## 17. RPC Interface

### Port Configuration

| Network | Ravencoin RPC Port | Yottaflux RPC Port |
|---|---|---|
| Mainnet | 8766 | 8558 |
| Testnet | 18766 | 18558 |
| Regtest | 18443 | 18561 |

### RPC Methods

All Ravencoin RPC methods are available in Yottaflux. The complete list includes standard Bitcoin Core RPCs plus the Ravencoin asset RPCs:

**Asset-specific RPCs** (unchanged from Ravencoin):
- `issue` / `issueunique`
- `reissue`
- `transfer` / `transferfromaddress` / `transferfromaddresses`
- `listassets` / `listmyassets` / `listassetbalancesbyaddress`
- `getassetdata` / `getcacheinfo`
- `listaddressesbyasset` / `listaddressesfortag`
- `isvalidverifierstring` / `checkaddressrestriction` / `checkaddresstag` / `checkglobalrestriction`
- `issuequalifierasset` / `issuerestrictedasset`
- `addtagtoaddress` / `removetagfromaddress`
- `freezeaddress` / `unfreezeaddress` / `freezerestrictedasset` / `unfreezerestrictedasset`
- `getverifierstring`
- `getsnapshot` / `purgesnapshot`
- `listtagsforaddress`
- `transferqualifier`

**Messaging RPCs**:
- `viewallmessages` / `viewallmessagechannels`
- `subscribetochannel` / `unsubscribefromchannel`
- `sendmessage` / `clearmessages`

**Reward RPCs**:
- `distributereward` / `getdistributestatus`
- `requestsnapshot` / `getsnapshotrequest` / `cancelsnapshotrequest`

### RPC Response Differences

Currency amounts in RPC responses use the same 8-decimal precision. The `getblockchaininfo` response will show the Yottaflux chain name and parameters. The `getmininginfo` response reflects the X16RV2 algorithm.

---

## 18. Configuration Files and Data Directories

| Item | Ravencoin | Yottaflux |
|---|---|---|
| **Config filename** | `raven.conf` | `yottaflux.conf` |
| **PID filename** | `ravend.pid` | `yottafluxd.pid` |
| **Data dir (Linux)** | `~/.raven/` | `~/.yottaflux/` |
| **Data dir (macOS)** | `~/Library/Application Support/Raven/` | `~/Library/Application Support/Yottaflux/` |
| **Data dir (Windows)** | `%APPDATA%\Raven\` | `%APPDATA%\Yottaflux\` |
| **Testnet subdir** | `testnet7/` | `testnet7/` |
| **Regtest subdir** | `regtest/` | `regtest/` |

### Config File Parameters

The config file format is identical. All Ravencoin config parameters work the same way. Key defaults to change:

```ini
# yottaflux.conf
rpcport=8558          # Was 8766 for Ravencoin
port=8559             # Was 8767 for Ravencoin

# Testnet
[test]
rpcport=18558         # Was 18766 for Ravencoin
port=18559            # Was 18770 for Ravencoin

# Regtest
[regtest]
rpcport=18561         # Was 18443 for Ravencoin
port=18560            # Was 18444 for Ravencoin
```

**Source**: `src/util.cpp`, `src/chainparamsbase.cpp`

---

## 19. DNS Seeds and Fixed Seeds

| Network | Ravencoin | Yottaflux |
|---|---|---|
| Mainnet | `seed-raven.bitactivate.com`, `seed-raven.ravencoin.com`, `seed-raven.ravencoin.org` | `seed.yottaflux.ai` |
| Testnet | `seed-testnet-raven.bitactivate.com`, `seed-testnet-raven.ravencoin.com` | `seed-test.yottaflux.ai` |
| Regtest | None | None |

**Note**: Yottaflux has cleared `vFixedSeeds` on mainnet (no hardcoded IP addresses). Peer discovery relies entirely on the DNS seed.

**Source**: `src/chainparams.cpp`

---

## 20. Qt GUI Constants

| Constant | Ravencoin | Yottaflux |
|---|---|---|
| **App name (mainnet)** | `Raven-Qt` | `Yottaflux-Qt` |
| **App name (testnet)** | `Raven-Qt-testnet` | `Yottaflux-Qt-testnet` |
| **Organization name** | `Raven` | `Yottaflux` |
| **Organization domain** | `ravencoin.org` | `yottaflux.ai` |
| **Window title prefix** | `Raven` | `Yottaflux` |
| **Title addition (mainnet)** | `""` | `""` |
| **Title addition (testnet)** | `"[testnet]"` | `"[testnet]"` |
| **Title addition (regtest)** | `"[regtest]"` | `"[regtest]"` |

### GUI Color Scheme

If porting a GUI wallet, update any Ravencoin-branded colors. The Yottaflux Qt wallet uses standard Bitcoin Core UI styling. Check `src/qt/res/` for icons and branding assets.

**Source**: `src/qt/guiconstants.h`, `src/qt/networkstyle.cpp`

---

## 21. Protocol Versioning

| Constant | Value | Notes |
|---|---|---|
| **PROTOCOL_VERSION** | 70028 | Current protocol version |
| **MIN_PEER_PROTO_VERSION** | 70025 | Minimum version for peer connections |
| **GETHEADERS_VERSION** | 70002 | Version that introduced getheaders |
| **INIT_PROTO_VERSION** | 209 | Initial protocol version |

Service flags and feature negotiation are the same as Ravencoin.

**Source**: `src/version.h`

---

## 22. Electrum-Specific Porting Notes

When adapting Electrum-RVN (or any Electrum fork) for Yottaflux:

### 1. Network Constants (`lib/networks.py` or equivalent)

```python
# Yottaflux Mainnet
ADDRTYPE_P2PKH = 78
ADDRTYPE_P2SH = 137
WIF_PREFIX = 128
XPUB_HEADERS = {'standard': 0x0488B21E}
XPRV_HEADERS = {'standard': 0x0488ADE4}
BIP44_COIN_TYPE = 5050
DEFAULT_PORTS = {'t': '50001', 's': '50002'}  # ElectrumX ports (configure as needed)
GENESIS_HASH = '000000f13b9584fc6830148c59ec77e5671a5ac3ff0f26a9ae0679c0ca40f579'

# Yottaflux Testnet
ADDRTYPE_P2PKH_TESTNET = 111
ADDRTYPE_P2SH_TESTNET = 196
WIF_PREFIX_TESTNET = 239
BIP44_COIN_TYPE_TESTNET = 1
GENESIS_HASH_TESTNET = '000000709fd50f9c838d68747b3b747da72c753ccc0733365220ff4cc5ea1c49'
```

### 2. Server Discovery

Replace Ravencoin ElectrumX server lists with Yottaflux servers. These will need to be set up separately.

### 3. Block Subsidy Validation

If Electrum validates block rewards, replace the uniform halving calculation with the hardcoded table from [Section 9](#9-block-subsidy-and-halving-schedule).

### 4. Asset Script Parsing

Replace `rvn` marker with `yai` in asset script detection:
```python
# Ravencoin
ASSET_SCRIPT_MARKER = b'rvn'

# Yottaflux
ASSET_SCRIPT_MARKER = b'yai'
```

### 5. Coinbase Validation

Yottaflux coinbase transactions have 3+ outputs (miner + dev fund + subsidy). Electrum typically doesn't validate coinbases, but if it displays coinbase transaction details, be aware of the structure.

### 6. Fee Defaults

Fee defaults are the same as Ravencoin (1000 corbie/kB minimum relay, 20000 corbie/kB fallback). No changes needed.

### 7. String/Branding Replacements

- Replace "Ravencoin"/"Raven" with "Yottaflux"
- Replace "RVN" with "YAI"
- Replace "satoshi" with "corbie"
- Update URLs, support links, and documentation references

---

## 23. Explorer-Specific Porting Notes

When adapting a Ravencoin block explorer:

### 1. Address Validation

- Mainnet P2PKH: version byte 78, addresses start with `Y`
- Mainnet P2SH: version byte 137, addresses start with `x`
- Testnet: Same as Ravencoin testnet (version bytes 111/196)

### 2. Coinbase Display

Every block's coinbase transaction should display:
- Output 0: Miner reward (60% of subsidy)
- Output 1: Community Development (10% of subsidy) — label as "Dev Fund"
- Output 2: Community Subsidy (30% of subsidy) — label as "Community Fund"

### 3. Asset Identification

When scanning scripts for asset operations, look for `OP_YAI_ASSET` (0xc0) followed by the `yai` marker (not `rvn`).

### 4. Burn Address Labeling

Label the burn addresses from [Section 14](#14-asset-burn-addresses) with their purpose (e.g., "Asset Issuance Burn").

### 5. Supply Calculation

Total supply follows the non-uniform halving schedule. Do not use `21B * (blocks / halving_interval)` — use the table from [Section 9](#9-block-subsidy-and-halving-schedule).

### 6. PoW Algorithm Display

Display "X16RV2" as the mining algorithm, not "KawPOW".

---

## 24. Mining Software Porting Notes

### 1. Algorithm

Use **X16RV2** exclusively. KawPOW is disabled on mainnet/testnet via far-future activation timestamp.

### 2. Coinbase Construction

Pools must construct coinbase transactions with exactly 3+ outputs:

```
vout[0]: miner_reward = floor(block_reward * 0.60)
vout[1]: dev_fund     = floor(block_reward * 0.10)  → send to CommunityDevelopmentAddress
vout[2]: subsidy_fund = block_reward - miner_reward - dev_fund  → send to CommunitySubsidyAddress
```

Use the community fund addresses from [Section 10](#10-community-fund-coinbase-tax).

### 3. Block Reward Lookup

Implement the halving table from [Section 9](#9-block-subsidy-and-halving-schedule) to determine the correct block reward for a given height.

### 4. Stratum Protocol

The stratum protocol is the same as Ravencoin's X16RV2 stratum. No changes to the mining protocol itself are needed beyond updating server addresses and ports.

### 5. Difficulty Target

Mainnet powLimit for X16RV2 is `00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff` (more permissive than Ravencoin's `00000fffff...`).

---

## 25. Library/SDK Porting Notes

### Python Libraries (e.g., python-ravencoinlib)

```python
# Network parameters to change:
NETWORK_MAGIC = bytes([0x59, 0x4F, 0x54, 0x58])  # "YOTX"
DEFAULT_PORT = 8559
PUBKEY_ADDRESS_PREFIX = 78
SCRIPT_ADDRESS_PREFIX = 137
WIF_PREFIX = 128
BIP44_COIN_TYPE = 5050
ASSET_SCRIPT_MARKER = b'yai'
```

### JavaScript Libraries (e.g., ravencore-lib)

```javascript
const networks = {
  mainnet: {
    messagePrefix: '\x19Yottaflux Signed Message:\n',
    bip32: { public: 0x0488b21e, private: 0x0488ade4 },
    pubKeyHash: 0x4e,  // 78
    scriptHash: 0x89,  // 137
    wif: 0x80,         // 128
    port: 8559,
    magic: Buffer.from([0x59, 0x4f, 0x54, 0x58]),
    bip44: 5050,
  },
  testnet: {
    messagePrefix: '\x19Yottaflux Signed Message:\n',
    bip32: { public: 0x043587cf, private: 0x04358394 },
    pubKeyHash: 0x6f,  // 111
    scriptHash: 0xc4,  // 196
    wif: 0xef,         // 239
    port: 18559,
    magic: Buffer.from([0x59, 0x4f, 0x54, 0x54]),
    bip44: 1,
  }
};
```

### Rust Libraries

```rust
pub const MAGIC_MAINNET: [u8; 4] = [0x59, 0x4F, 0x54, 0x58];
pub const MAGIC_TESTNET: [u8; 4] = [0x59, 0x4F, 0x54, 0x54];
pub const MAGIC_REGTEST: [u8; 4] = [0x59, 0x4F, 0x54, 0x52];

pub const PUBKEY_ADDRESS_PREFIX_MAIN: u8 = 78;
pub const SCRIPT_ADDRESS_PREFIX_MAIN: u8 = 137;
pub const WIF_PREFIX_MAIN: u8 = 128;

pub const PUBKEY_ADDRESS_PREFIX_TEST: u8 = 111;
pub const SCRIPT_ADDRESS_PREFIX_TEST: u8 = 196;
pub const WIF_PREFIX_TEST: u8 = 239;

pub const BIP44_COIN_TYPE_MAIN: u32 = 5050;
pub const BIP44_COIN_TYPE_TEST: u32 = 1;

pub const DEFAULT_PORT_MAIN: u16 = 8559;
pub const DEFAULT_PORT_TEST: u16 = 18559;
pub const DEFAULT_PORT_REGTEST: u16 = 18560;
```

### Message Signing Prefix

If the tool supports message signing/verification:
```
Ravencoin:  "\x19Raven Signed Message:\n"
Yottaflux:  "\x19Yottaflux Signed Message:\n"
```

Verify this in `src/validation.cpp` or the message signing code.

---

## 26. Quick Reference: Search-and-Replace Table

This table summarizes every string/value replacement needed when porting a Ravencoin tool. Use this as a checklist.

### Strings

| Search | Replace |
|---|---|
| `Ravencoin` | `Yottaflux` |
| `ravencoin` | `yottaflux` |
| `RAVENCOIN` | `YOTTAFLUX` |
| `Raven` (in app names) | `Yottaflux` |
| `raven` (in file paths) | `yottaflux` |
| `RVN` (ticker) | `YAI` |
| `rvn` (asset marker) | `yai` |
| `satoshi` (unit name) | `corbie` |
| `ravencoin.org` | `yottaflux.ai` |
| `RAVN` (magic) | `YOTX` |
| `RVNT` (testnet magic) | `YOTT` |
| `CROW` (regtest magic) | `YOTR` |
| `OP_RVN_ASSET` | `OP_YAI_ASSET` |
| `Raven Signed Message` | `Yottaflux Signed Message` |
| `raven.conf` | `yottaflux.conf` |
| `ravend.pid` | `yottafluxd.pid` |
| `.raven` (data dir) | `.yottaflux` |

### Numeric Constants

| Parameter | Ravencoin Value | Yottaflux Value |
|---|---|---|
| P2PKH version (mainnet) | 60 | 78 |
| P2SH version (mainnet) | 122 | 137 |
| Default port (mainnet) | 8767 | 8559 |
| Default port (testnet) | 18770 | 18559 |
| Default port (regtest) | 18444 | 18560 |
| RPC port (mainnet) | 8766 | 8558 |
| RPC port (testnet) | 18766 | 18558 |
| RPC port (regtest) | 18443 | 18561 |
| BIP44 coin type | 175 | 5050 |
| Magic byte 0 | 0x52 ('R') | 0x59 ('Y') |
| Magic byte 1 | 0x41 ('A') | 0x4F ('O') |
| Magic byte 2 | 0x56 ('V') | 0x54 ('T') |
| Magic byte 3 | 0x4E ('N') | 0x58 ('X') |
| nPowTargetTimespan | 2016 × 60 | 100 × 60 |
| MAX_MONEY | 21B × COIN | 1.2B × COIN |
| Total supply | 21,000,000,000 | ~1,160,550,000 |
| nSubsidyHalvingInterval | 2,100,000 | N/A (hardcoded table) |

### Genesis Block Hashes

| Network | Yottaflux Hash |
|---|---|
| Mainnet | `000000f13b9584fc6830148c59ec77e5671a5ac3ff0f26a9ae0679c0ca40f579` |
| Testnet | `000000709fd50f9c838d68747b3b747da72c753ccc0733365220ff4cc5ea1c49` |
| Regtest | `45a2ecb9ae084bcd083ac8ece09e30a239affd25ac2f3d3e7bb48463caa0d432` |
| Merkle root (all) | `648156c6987915ad1270f7ffac0d57b7a2dd4fecaa0f59d059e0166213df2815` |

---

## Appendix A: Yottaflux-Only Features

These features exist in Yottaflux but **not** in Ravencoin:

1. **Community Fund (40% coinbase tax)**: 10% dev + 30% subsidy deducted from every block
2. **Non-uniform halving schedule**: 13 eras with accelerating halving intervals
3. **X16RV2-only mining**: KawPOW is disabled via far-future activation timestamp
4. **Reduced total supply**: ~1.16B YAI vs 21B RVN
5. **Different nPowTargetTimespan**: 100 minutes vs 2016 minutes

These must be explicitly implemented when porting; they cannot be derived from Ravencoin code alone.

## Appendix B: Unchanged from Ravencoin

These components are identical and need no modification (beyond branding):

1. **Transaction format**: Same serialization, same SegWit structure
2. **Script system**: Same opcodes (except asset marker string)
3. **Asset system behavior**: Same types, rules, and limits
4. **Fee structure**: Same defaults and dust thresholds
5. **Block structure**: Same format (weight/size limits)
6. **BIP support**: Same BIPs enabled (34, 65, 66, CSV, SegWit)
7. **Wallet format**: Same BerkeleyDB wallet
8. **P2P protocol**: Same version, same messages
9. **DGW difficulty adjustment**: Same algorithm
10. **Asset burn amounts**: Same for all asset types
