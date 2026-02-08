Developer Mode
==============

Developer mode provides a private, local Yottaflux blockchain (regtest) that
you fully control.  Blocks are mined on demand, there are no external
dependencies, and the chain starts from genesis — making it ideal for wallet
development, explorer integration, asset workflow testing, and general
experimentation.

Prerequisites
-------------

1. **Built binaries** — `yottafluxd` and `yottaflux-cli` must be compiled.
   See [build-unix.md](build-unix.md) (Linux), [build-osx.md](build-osx.md)
   (macOS), or [build-windows.md](build-windows.md) (Windows) for
   instructions.  A minimal build:

   ```bash
   ./autogen.sh
   ./configure --disable-wallet-tool --without-gui
   make -j$(nproc)
   ```

2. **Python 3** (optional) — required only if you plan to use the asset tools
   in `assets/tools/`.

Quick Start
-----------

A single command bootstraps a ready-to-use development environment:

```bash
contrib/devmode/setup.sh
```

This will:

- Start `yottafluxd` in regtest mode.
- Mine 400 blocks (enough for DGW activation and coinbase maturity).
- Create a wallet address with a spendable YFX balance.
- Issue a sample `DEVTOKEN` asset (1000 units).
- Print connection details and helper commands.

Stop the node at any time:

```bash
contrib/devmode/setup.sh --stop
```

Reset the environment completely (stop + delete data):

```bash
contrib/devmode/setup.sh --clean
```

### Environment variables

| Variable       | Default                            | Description                 |
|----------------|------------------------------------|-----------------------------|
| `TOPDIR`       | git repo root                      | Project root directory      |
| `SRCDIR`       | `$TOPDIR/src`                      | Source / binary directory    |
| `YOTTAFLUXD`   | `$SRCDIR/yottafluxd`               | Path to the daemon binary   |
| `YOTTAFLUXCLI` | `$SRCDIR/yottaflux-cli`            | Path to the CLI binary      |
| `DATADIR`      | `/tmp/yottaflux-devmode/node0`     | Data directory for the node |

Regtest Configuration
---------------------

The regtest configuration lives at `scripts/yottaflux_regtest.conf` and is
copied into the data directory by the setup script.  Key parameters:

```
regtest=1              # Enable regression-test mode
server=1               # Accept RPC commands
txindex=1              # Full transaction index
rpcuser=devuser        # RPC username
rpcpassword=devpass    # RPC password
rpcport=18561          # JSON-RPC port
port=18560             # P2P listen port
fallbackfee=0.0001     # Fallback fee (no estimation data on fresh chain)
debug=1                # All debug log categories
printtoconsole=1       # Log to stdout as well as debug.log
```

Edit the file or override settings on the command line:

```bash
yottafluxd -regtest -rpcport=19000 ...
```

Network Parameters (regtest)
----------------------------

| Parameter                    | Value                                        |
|------------------------------|----------------------------------------------|
| P2P port                     | 18560                                        |
| RPC port                     | 18561                                        |
| Magic bytes                  | `YOTR` (0x594f5452)                          |
| DNS seeds                    | None (fully local)                           |
| Asset activation height      | Block 0                                      |
| DGW activation height        | Block 200                                    |
| Coinbase maturity            | 100 blocks                                   |
| Community fund dev address   | `mhXBsq5cN3pqtXCqVACuva2CQNzHdYcmYC`        |
| Community fund subsidy addr  | `n42rJAdeveUKP6PvQHKE6b55bYWzLeYtj7`         |

Multi-Node Setup
----------------

For testing P2P behaviour, block propagation, or multi-wallet scenarios, use
the multi-node script:

```bash
contrib/devmode/multi-node.sh              # launch 3 nodes (default)
contrib/devmode/multi-node.sh --nodes=4    # launch 4 nodes
contrib/devmode/multi-node.sh --stop       # stop all nodes
```

Nodes are created under `/tmp/yottaflux-devmode/node{0,1,...}` with ports
assigned in pairs (P2P, RPC) starting from the base ports:

| Node | P2P port | RPC port |
|------|----------|----------|
| 0    | 18560    | 18561    |
| 1    | 18562    | 18563    |
| 2    | 18564    | 18565    |
| 3    | 18566    | 18567    |

All nodes are connected in a full mesh via `addnode`.  Node 0 mines the
initial 400 blocks; the script waits for all other nodes to sync before
printing a status summary.

The script stays in the foreground.  Press `Ctrl-C` to shut down all nodes
gracefully.

### Interacting with individual nodes

The multi-node script prints shell helper functions.  Copy them into your
terminal:

```bash
cli0() { src/yottaflux-cli -regtest -rpcuser=devuser -rpcpassword=devpass -rpcport=18561 -rpcconnect=127.0.0.1 "$@"; }
cli1() { src/yottaflux-cli -regtest -rpcuser=devuser -rpcpassword=devpass -rpcport=18563 -rpcconnect=127.0.0.1 "$@"; }
cli2() { src/yottaflux-cli -regtest -rpcuser=devuser -rpcpassword=devpass -rpcport=18565 -rpcconnect=127.0.0.1 "$@"; }
```

Then interact with any node:

```bash
cli0 getblockchaininfo
cli1 getbalance
cli0 sendtoaddress $(cli1 getnewaddress) 10
cli0 generatetoaddress 1 $(cli0 getnewaddress)
```

Common Operations
-----------------

All examples below assume the single-node setup script was used.  Adjust
`-datadir` and `-rpcport` if using multi-node or a custom data directory.

```bash
# Shorthand — set once per terminal session
alias ycli='src/yottaflux-cli -regtest -datadir=/tmp/yottaflux-devmode/node0'
```

### Mining blocks

```bash
ycli generate 10
```

### Creating assets

```bash
ycli issue "MYTOKEN" 1000          # 1000 units, default 8 decimals
ycli issue "MYTOKEN" 1000 8 1 1    # 1000 units, 8 decimals, reissuable, has IPFS
ycli generate 1                    # confirm the transaction
```

### Transferring assets

```bash
ycli transfer "MYTOKEN" 100 "DESTINATION_ADDRESS"
ycli generate 1
```

### Checking balances

```bash
ycli getbalance               # YFX balance
ycli listmyassets             # all owned assets
ycli listmyassets "MYTOKEN"   # specific asset balance
```

### Getting blockchain info

```bash
ycli getblockchaininfo
ycli getblockcount
ycli getbestblockhash
ycli getblock $(ycli getbestblockhash)
```

### Sending YFX

```bash
ycli sendtoaddress $(ycli getnewaddress) 50
ycli generate 1
```

IPFS Configuration
------------------

The asset tool `assets/tools/ipfs_pinner.py` fetches IPFS metadata for
assets.  By default it uses `https://ipfs.io/ipfs/` as the gateway.

To use a local IPFS node or an alternative gateway, set the `IPFS_GATEWAY`
environment variable:

```bash
# Local IPFS daemon
export IPFS_GATEWAY="http://127.0.0.1:8080/ipfs/"

# Alternative public gateway
export IPFS_GATEWAY="https://dweb.link/ipfs/"
```

The pinner also requires a local IPFS daemon (port 5001) for `ipfs get` and
`ipfs pin` operations.  Install and start IPFS:

```bash
# Install
# See https://docs.ipfs.tech/install/

# Start the daemon
ipfs daemon
```

Differences from Mainnet
------------------------

Regtest is a private, isolated chain.  Key differences:

| Aspect              | Mainnet                          | Regtest                        |
|---------------------|----------------------------------|--------------------------------|
| Mining              | Proof-of-work (KAWPOW)           | Instant via `generate` RPC     |
| Block time          | ~1 minute target                 | On demand                      |
| Coins               | Real value                       | No value (test only)           |
| Address prefix      | `Y` (pubkey), `y` (script)       | `m`/`n` (pubkey), `2` (script) |
| Network peers       | Public DNS seeds                 | None (manual `addnode` only)   |
| Asset activation    | Specific block height            | Block 0                        |
| DGW activation      | Specific block height            | Block 200                      |
| Magic bytes         | Mainnet magic                    | `YOTR` (0x594f5452)            |
| Data directory      | `~/.yottaflux/`                  | Custom (`/tmp/...`)            |

Troubleshooting
---------------

### Node fails to start

- **"Cannot obtain a lock"** — another `yottafluxd` instance is already using
  the data directory.  Run `contrib/devmode/setup.sh --stop` or check with
  `ps aux | grep yottafluxd`.

- **"Error: Specified -datadir does not exist"** — the data directory was
  removed.  Run `contrib/devmode/setup.sh` (it creates it automatically).

### RPC connection refused

- Confirm the node is running: `ps aux | grep yottafluxd`
- Check the port: `ss -tlnp | grep 18561`
- Verify credentials match: default is `devuser` / `devpass`.
- If using multi-node, ensure you are connecting to the correct RPC port for
  the desired node.

### "generate" RPC returns error

The `generate` RPC requires a wallet.  If you built with `--disable-wallet`,
use `generatetoaddress` with an explicit address instead.

### Blocks not propagating in multi-node setup

- Check that nodes are connected: `cli0 getpeerinfo | grep addr`
- Manually reconnect: `cli0 addnode "127.0.0.1:18562" onetry`
- Allow a few seconds for propagation after mining.

### Asset creation fails

- Ensure you have enough YFX balance for the burn fee (500 YFX for a root
  asset).  Mine more blocks if needed: `ycli generate 100`.
- Asset names must be uppercase alphanumeric (3-30 characters) and unique on
  the chain.

### "fallbackfee" warning

This is expected on a fresh regtest chain with no fee estimation data.  The
regtest config already sets `fallbackfee=0.0001` to suppress it.
