# CLAUDE.md

## Project Overview

Yottaflux is a peer-to-peer blockchain network for transferring digital assets (tokens, NFTs, STOs). It is a Bitcoin Core fork extended with asset creation and management capabilities. Version 4.6.1, released under the MIT license.

## Build System

This project uses GNU Autotools (autoconf/automake/libtool) with C++17 (no GNU extensions).

### Build Commands

```bash
./autogen.sh
./configure
make -j$(nproc)
```

### Build Outputs

- `src/yottafluxd` — Daemon/node
- `src/yottaflux-cli` — CLI tool
- `src/yottaflux-tx` — Transaction utility
- `src/yottaflux-qt` — Qt GUI wallet (if built with `--with-gui=qt5`)

### Common Configure Options

```bash
--with-gui=qt5           # Build Qt5 GUI (default)
--disable-wallet         # Disable wallet functionality
--enable-zmq             # Enable ZMQ notifications
--enable-debug           # Enable debug symbols & DEBUG_LOCKORDER
```

## Testing

### Unit Tests (Boost-based)

```bash
make check
# Or run directly:
src/test/test_yottaflux
# Single test suite:
src/test/test_yottaflux --run_test=<suite_name>
```

### Functional Tests (Python 3.4+)

```bash
test/functional/test_runner.py                     # All base tests
test/functional/test_runner.py --extended           # Extended tests
test/functional/test_runner.py feature_assets.py   # Single test
test/functional/test_runner.py --jobs=4            # Parallel
```

## Linting & Code Quality

```bash
contrib/devtools/lint-all.sh                  # Run all lint checks
contrib/devtools/clang-format-diff.py         # Format changed code
contrib/devtools/check-doc.py                 # Check documentation
contrib/devtools/check-rpc-mappings.py .      # Validate RPC mappings
```

## Code Style

Formatting is governed by `src/.clang-format`:
- **Indentation**: 4 spaces, no tabs
- **Column limit**: None
- **Braces**: New line for functions/classes/namespaces; same line for control flow

### Naming Conventions

| Element | Convention | Example |
|---|---|---|
| Variables | `snake_case` | `block_height` |
| Member variables | `m_` prefix | `m_name` |
| Global variables | `g_` prefix | `g_connman` |
| Constants | `UPPER_SNAKE_CASE` | `MAX_BLOCK_SIZE` |
| Classes | `PascalCase` | `CTransaction` |
| Functions | `PascalCase` | `GetBalance()` |
| RPC methods | `lowercase` | `getrawtransaction` |
| RPC params | `snake_case` | `fee_delta` |
| Namespaces | `lowercase` | `namespace consensus {}` |
| Test files | `foo_tests.cpp` | `asset_tests.cpp` |

### Header Guards

```cpp
#ifndef YOTTAFLUX_MODULE_H
#define YOTTAFLUX_MODULE_H
// ...
#endif // YOTTAFLUX_MODULE_H
```

### Key Conventions

- Use `nullptr` (not `NULL`)
- Prefer `++i` over `i++`
- Avoid `using namespace std;`
- Use `unique_ptr` for heap allocations (RAII)
- Use `.find()` over `[]` when reading from maps
- Use `vch.data()` instead of `&vch[0]`
- Every `.cpp`/`.h` must include all headers it directly uses
- End namespace blocks with `// namespace name` comment

### Thread Safety

```cpp
LOCK(cs_wallet);           // Scoped lock
TRY_LOCK(cs_main, lock);  // Try lock with boolean result
```

Build with `-DDEBUG_LOCKORDER` to detect lock-order violations.

### Serialization

```cpp
ADD_SERIALIZE_METHODS;
template <typename Stream, typename Operation>
inline void SerializationOp(Stream& s, Operation ser_action) {
    READWRITE(field);
}
```

### Logging

```cpp
LogPrintf("Message %d\n", value);             // No category
LogPrint(BCLog::NET, "Network msg\n");        // With category
```

## Architecture

```
src/
├── assets/       # Asset system (token creation, management, rewards)
├── consensus/    # Consensus-critical code (tx verification, validation)
├── wallet/       # Wallet (BerkeleyDB, encryption, BIP39)
├── rpc/          # JSON-RPC interface
├── script/       # Script interpreter and signing
├── primitives/   # Block and transaction data structures
├── crypto/       # Cryptographic primitives
├── policy/       # Fee and transaction policies
├── qt/           # Qt GUI (optional)
├── zmq/          # ZMQ notifications (optional)
├── test/         # Unit tests (Boost)
├── leveldb/      # Embedded LevelDB (subtree)
├── secp256k1/    # Elliptic curve crypto (subtree)
└── univalue/     # JSON library (subtree)
```

Key libraries built from source: `libyottaflux_crypto`, `libyottaflux_util`, `libyottaflux_common`, `libyottaflux_consensus`, `libyottaflux_server`, `libyottaflux_wallet`.

## Dependencies

- **Boost** — Core system library (thread, filesystem, test)
- **OpenSSL** — Cryptography and TLS
- **LevelDB** — Block/chainstate storage (embedded)
- **libsecp256k1** — Elliptic curve crypto (embedded)
- **BerkeleyDB 4.8** — Wallet database
- **libevent** — Async I/O
- **Qt5** — GUI (optional)
- **ZMQ** — Messaging (optional)
- **Protobuf** — Serialization
