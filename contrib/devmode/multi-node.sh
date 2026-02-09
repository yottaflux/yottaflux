#!/usr/bin/env bash
# Copyright (c) 2024-2025 The Yottaflux developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Multi-node regtest setup script for Yottaflux development.
#
# Launches N regtest nodes (default 3), connects them in a mesh,
# mines 400 blocks on node 0 so assets are usable (DGW activates at
# block 200, coinbase maturity is 100), and prints a status summary
# with helper commands.
#
# Usage:
#   ./contrib/devmode/multi-node.sh              # start 3 nodes
#   ./contrib/devmode/multi-node.sh --nodes=4    # start 4 nodes
#   ./contrib/devmode/multi-node.sh --stop       # stop all devmode nodes

set -euo pipefail

# ---------------------------------------------------------------------------
# Binary paths
# ---------------------------------------------------------------------------
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
YOTTAFLUXD=${YOTTAFLUXD:-$SRCDIR/yottafluxd}
YOTTAFLUXCLI=${YOTTAFLUXCLI:-$SRCDIR/yottaflux-cli}

# ---------------------------------------------------------------------------
# Configuration defaults
# ---------------------------------------------------------------------------
DATADIR_BASE="/tmp/yottaflux-devmode"
NUM_NODES=3
STOP_MODE=false
BASE_P2P_PORT=18560
BASE_RPC_PORT=18561
RPC_USER="devuser"
RPC_PASS="devpass"
BLOCKS_TO_MINE=400

# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------
for arg in "$@"; do
    case "$arg" in
        --nodes=*)
            NUM_NODES="${arg#*=}"
            if ! [[ "$NUM_NODES" =~ ^[0-9]+$ ]] || [ "$NUM_NODES" -lt 2 ] || [ "$NUM_NODES" -gt 5 ]; then
                echo "Error: --nodes must be between 2 and 5 (got: $NUM_NODES)"
                exit 1
            fi
            ;;
        --stop)
            STOP_MODE=true
            ;;
        --help|-h)
            echo "Usage: $0 [--nodes=N] [--stop]"
            echo ""
            echo "  --nodes=N   Number of nodes to launch (2-5, default 3)"
            echo "  --stop      Stop all running devmode nodes"
            echo "  --help      Show this help"
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Run '$0 --help' for usage."
            exit 1
            ;;
    esac
done

# ---------------------------------------------------------------------------
# Colour helpers (degrade gracefully when not on a terminal)
# ---------------------------------------------------------------------------
if [ -t 1 ]; then
    C_GREEN='\033[0;32m'
    C_YELLOW='\033[1;33m'
    C_RED='\033[0;31m'
    C_CYAN='\033[0;36m'
    C_BOLD='\033[1m'
    C_RESET='\033[0m'
else
    C_GREEN='' C_YELLOW='' C_RED='' C_CYAN='' C_BOLD='' C_RESET=''
fi

info()  { echo -e "${C_GREEN}[info]${C_RESET}  $*"; }
warn()  { echo -e "${C_YELLOW}[warn]${C_RESET}  $*"; }
error() { echo -e "${C_RED}[error]${C_RESET} $*" >&2; }

# ---------------------------------------------------------------------------
# Helper: RPC wrapper for a given node index
# ---------------------------------------------------------------------------
node_cli() {
    local idx=$1; shift
    local rpc_port=$(( BASE_RPC_PORT + 2 * idx ))
    "$YOTTAFLUXCLI" -regtest -rpcuser="$RPC_USER" -rpcpassword="$RPC_PASS" \
        -rpcport="$rpc_port" -rpcconnect=127.0.0.1 "$@"
}

# ---------------------------------------------------------------------------
# Stop mode: kill running devmode nodes and clean up
# ---------------------------------------------------------------------------
if [ "$STOP_MODE" = true ]; then
    info "Stopping all Yottaflux devmode nodes..."
    stopped=0
    for dir in "$DATADIR_BASE"/node*/; do
        [ -d "$dir" ] || continue
        idx=$(basename "$dir" | sed 's/node//')
        rpc_port=$(( BASE_RPC_PORT + 2 * idx ))
        if "$YOTTAFLUXCLI" -regtest -rpcuser="$RPC_USER" -rpcpassword="$RPC_PASS" \
                -rpcport="$rpc_port" -rpcconnect=127.0.0.1 stop 2>/dev/null; then
            info "Stopped node $idx (RPC $rpc_port)"
            stopped=$(( stopped + 1 ))
        fi
    done
    if [ "$stopped" -eq 0 ]; then
        warn "No running devmode nodes found."
    else
        info "Stopped $stopped node(s)."
    fi
    exit 0
fi

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
if [ ! -x "$YOTTAFLUXD" ]; then
    error "yottafluxd not found at $YOTTAFLUXD"
    error "Build first:  cd $TOPDIR && ./autogen.sh && ./configure && make -j\$(nproc)"
    exit 1
fi
if [ ! -x "$YOTTAFLUXCLI" ]; then
    error "yottaflux-cli not found at $YOTTAFLUXCLI"
    exit 1
fi

# ---------------------------------------------------------------------------
# Collect PIDs so cleanup can stop everything
# ---------------------------------------------------------------------------
declare -a NODE_PIDS=()

cleanup() {
    echo ""
    info "Shutting down all devmode nodes..."
    for i in $(seq 0 $(( NUM_NODES - 1 ))); do
        node_cli "$i" stop 2>/dev/null || true
    done
    # Wait briefly for graceful shutdown
    for pid in "${NODE_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            wait "$pid" 2>/dev/null || true
        fi
    done
    info "All nodes stopped."
}
trap cleanup EXIT INT TERM

# ---------------------------------------------------------------------------
# Create data directories and per-node configuration
# ---------------------------------------------------------------------------
info "Setting up $NUM_NODES regtest nodes under $DATADIR_BASE ..."

for i in $(seq 0 $(( NUM_NODES - 1 ))); do
    datadir="$DATADIR_BASE/node${i}"
    mkdir -p "$datadir"

    p2p_port=$(( BASE_P2P_PORT + 2 * i ))
    rpc_port=$(( BASE_RPC_PORT + 2 * i ))

    cat > "$datadir/yottaflux.conf" <<CONF
# Auto-generated devmode config for node $i
regtest=1
server=1
txindex=1
listen=1
port=$p2p_port
rpcport=$rpc_port
rpcuser=$RPC_USER
rpcpassword=$RPC_PASS
rpcallowip=127.0.0.0/8
rpcbind=127.0.0.1
CONF

    # Add all other nodes as peers
    for j in $(seq 0 $(( NUM_NODES - 1 ))); do
        [ "$j" -eq "$i" ] && continue
        peer_port=$(( BASE_P2P_PORT + 2 * j ))
        echo "addnode=127.0.0.1:$peer_port" >> "$datadir/yottaflux.conf"
    done
done

# ---------------------------------------------------------------------------
# Start nodes
# ---------------------------------------------------------------------------
info "Starting nodes..."

for i in $(seq 0 $(( NUM_NODES - 1 ))); do
    datadir="$DATADIR_BASE/node${i}"
    rpc_port=$(( BASE_RPC_PORT + 2 * i ))

    "$YOTTAFLUXD" -datadir="$datadir" -regtest >"$datadir/stdout.log" 2>&1 &
    NODE_PIDS+=($!)

    info "  Node $i  PID=${NODE_PIDS[-1]}  P2P=$(( BASE_P2P_PORT + 2 * i ))  RPC=$rpc_port"
done

# ---------------------------------------------------------------------------
# Wait for each node's RPC to become ready
# ---------------------------------------------------------------------------
info "Waiting for RPC interfaces..."

for i in $(seq 0 $(( NUM_NODES - 1 ))); do
    rpc_port=$(( BASE_RPC_PORT + 2 * i ))
    retries=0
    max_retries=60
    while ! node_cli "$i" getblockchaininfo &>/dev/null; do
        retries=$(( retries + 1 ))
        if [ "$retries" -ge "$max_retries" ]; then
            error "Node $i (RPC $rpc_port) failed to start after ${max_retries}s"
            exit 1
        fi
        sleep 1
    done
    info "  Node $i RPC ready (${retries}s)"
done

# ---------------------------------------------------------------------------
# Connect nodes in a mesh via addnode
# ---------------------------------------------------------------------------
info "Connecting nodes..."

for i in $(seq 0 $(( NUM_NODES - 1 ))); do
    for j in $(seq 0 $(( NUM_NODES - 1 ))); do
        [ "$j" -eq "$i" ] && continue
        peer_port=$(( BASE_P2P_PORT + 2 * j ))
        node_cli "$i" addnode "127.0.0.1:$peer_port" onetry 2>/dev/null || true
    done
done

# Give connections a moment to establish
sleep 2

# ---------------------------------------------------------------------------
# Mine initial blocks on node 0
# ---------------------------------------------------------------------------
info "Mining $BLOCKS_TO_MINE blocks on node 0 (DGW activates at 200, maturity at 100)..."

# Get an address from node 0 for mining rewards
MINE_ADDR=$(node_cli 0 getnewaddress)
node_cli 0 generatetoaddress "$BLOCKS_TO_MINE" "$MINE_ADDR" >/dev/null

node0_height=$(node_cli 0 getblockcount)
info "  Node 0 block height: $node0_height"

# ---------------------------------------------------------------------------
# Wait for blocks to propagate to all nodes
# ---------------------------------------------------------------------------
info "Verifying block propagation..."

for i in $(seq 1 $(( NUM_NODES - 1 ))); do
    retries=0
    max_retries=30
    while true; do
        height=$(node_cli "$i" getblockcount 2>/dev/null || echo 0)
        if [ "$height" -ge "$node0_height" ]; then
            info "  Node $i synced at height $height"
            break
        fi
        retries=$(( retries + 1 ))
        if [ "$retries" -ge "$max_retries" ]; then
            warn "  Node $i only at height $height after ${max_retries}s (expected $node0_height)"
            break
        fi
        sleep 1
    done
done

# ---------------------------------------------------------------------------
# Print status summary
# ---------------------------------------------------------------------------
echo ""
echo -e "${C_BOLD}============================================================${C_RESET}"
echo -e "${C_BOLD} Yottaflux Devmode Multi-Node Network${C_RESET}"
echo -e "${C_BOLD}============================================================${C_RESET}"
echo ""

for i in $(seq 0 $(( NUM_NODES - 1 ))); do
    p2p_port=$(( BASE_P2P_PORT + 2 * i ))
    rpc_port=$(( BASE_RPC_PORT + 2 * i ))
    height=$(node_cli "$i" getblockcount 2>/dev/null || echo "?")
    peers=$(node_cli "$i" getconnectioncount 2>/dev/null || echo "?")
    balance=$(node_cli "$i" getbalance 2>/dev/null || echo "?")

    echo -e "  ${C_CYAN}Node $i${C_RESET}  P2P=$p2p_port  RPC=$rpc_port  Height=$height  Peers=$peers  Balance=$balance"
done

echo ""
echo -e "${C_BOLD}Helper commands:${C_RESET}"
echo ""
for i in $(seq 0 $(( NUM_NODES - 1 ))); do
    rpc_port=$(( BASE_RPC_PORT + 2 * i ))
    echo "  cli${i}() { $YOTTAFLUXCLI -regtest -rpcuser=$RPC_USER -rpcpassword=$RPC_PASS -rpcport=$rpc_port -rpcconnect=127.0.0.1 \"\$@\"; }"
done

echo ""
echo -e "${C_BOLD}Quick start:${C_RESET}"
echo ""
echo "  # Source helper functions into your shell:"
echo "  eval \"\$(sed -n '/^  cli[0-9]/s/^  //p' <<< \"\$(cat /dev/stdin)\")\""
echo ""
echo "  # Or copy-paste the cli*() functions above, then:"
echo "  cli0 getblockchaininfo          # query node 0"
echo "  cli1 getbalance                 # query node 1"
echo "  cli0 sendtoaddress \$(cli1 getnewaddress) 10   # send coins"
echo "  cli0 generatetoaddress 1 \$(cli0 getnewaddress) # mine a block"
echo ""
echo -e "${C_BOLD}Data directories:${C_RESET}  $DATADIR_BASE/node{0..$(( NUM_NODES - 1 ))}/"
echo -e "${C_BOLD}Stop all nodes:${C_RESET}    $0 --stop"
echo ""
echo -e "${C_BOLD}============================================================${C_RESET}"
echo ""

# ---------------------------------------------------------------------------
# Keep running so the cleanup trap fires on Ctrl-C
# ---------------------------------------------------------------------------
info "Network is running. Press Ctrl-C to stop all nodes."
echo ""

# Wait for any node process to exit (indicates a problem) or for user interrupt
wait -n "${NODE_PIDS[@]}" 2>/dev/null || true

# If we reach here without the trap, a node exited unexpectedly
warn "A node process exited. Cleaning up..."
