#!/usr/bin/env bash
#
# setup.sh — Bootstrap a Yottaflux regtest development environment.
#
# Creates a private single-node regtest chain with pre-mined coins and a
# sample asset so you can start developing immediately.
#
# Usage:
#   contrib/devmode/setup.sh           # Start / bootstrap devmode node
#   contrib/devmode/setup.sh --stop    # Stop the devmode node
#   contrib/devmode/setup.sh --clean   # Stop and remove all devmode data
#   contrib/devmode/setup.sh --help    # Show this help
#

set -euo pipefail

# ---------------------------------------------------------------------------
# Colors & helpers
# ---------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

info()  { echo -e "${CYAN}[INFO]${NC}  $*"; }
ok()    { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
YOTTAFLUXD=${YOTTAFLUXD:-$SRCDIR/yottafluxd}
YOTTAFLUXCLI=${YOTTAFLUXCLI:-$SRCDIR/yottaflux-cli}
DATADIR=${DATADIR:-/tmp/yottaflux-devmode/node0}
REGTEST_CONF=${REGTEST_CONF:-$TOPDIR/scripts/yottaflux_regtest.conf}
RPC_PORT=18561

# Common CLI flags for every yottaflux-cli invocation.
CLI_FLAGS="-regtest -datadir=$DATADIR"

# ---------------------------------------------------------------------------
# Functions
# ---------------------------------------------------------------------------

usage() {
    cat <<EOF
${BOLD}Yottaflux Devmode Bootstrap${NC}

Usage:
  $(basename "$0") [--help | --stop | --clean]

Options:
  (none)    Start the devmode node and bootstrap the chain.
  --help    Show this help message.
  --stop    Stop the running devmode node.
  --clean   Stop the node and remove the data directory.

Environment variables:
  TOPDIR       Project root       (default: git repo root)
  SRCDIR       Source directory    (default: \$TOPDIR/src)
  YOTTAFLUXD   Path to yottafluxd (default: \$SRCDIR/yottafluxd)
  YOTTAFLUXCLI Path to yottaflux-cli (default: \$SRCDIR/yottaflux-cli)
  DATADIR      Data directory      (default: /tmp/yottaflux-devmode/node0)
EOF
}

stop_node() {
    if $YOTTAFLUXCLI $CLI_FLAGS stop >/dev/null 2>&1; then
        ok "Devmode node stopped."
    else
        warn "Node does not appear to be running."
    fi
}

clean_datadir() {
    stop_node
    if [ -d "$DATADIR" ]; then
        rm -rf "$DATADIR"
        ok "Removed data directory: $DATADIR"
    else
        warn "Data directory does not exist: $DATADIR"
    fi
}

check_binary() {
    local bin=$1
    if [ ! -x "$bin" ]; then
        error "Binary not found or not executable: $bin"
        error "Have you built the project?  Run: make -j\$(nproc)"
        exit 1
    fi
}

wait_for_rpc() {
    local max_attempts=30
    local attempt=0
    info "Waiting for RPC to become ready..."
    while [ $attempt -lt $max_attempts ]; do
        if $YOTTAFLUXCLI $CLI_FLAGS getblockchaininfo >/dev/null 2>&1; then
            ok "RPC is ready."
            return 0
        fi
        attempt=$((attempt + 1))
        sleep 1
    done
    error "RPC did not become ready within ${max_attempts}s."
    exit 1
}

# Cleanup trap — stop the daemon if the script fails partway through.
cleanup_on_error() {
    error "Setup failed. Attempting to stop the node..."
    $YOTTAFLUXCLI $CLI_FLAGS stop >/dev/null 2>&1 || true
}

# ---------------------------------------------------------------------------
# Argument handling
# ---------------------------------------------------------------------------
case "${1:-}" in
    --help|-h)
        usage
        exit 0
        ;;
    --stop)
        stop_node
        exit 0
        ;;
    --clean)
        clean_datadir
        exit 0
        ;;
    "")
        # Default: bootstrap
        ;;
    *)
        error "Unknown option: $1"
        usage
        exit 1
        ;;
esac

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
echo ""
echo -e "${BOLD}=== Yottaflux Devmode Bootstrap ===${NC}"
echo ""

check_binary "$YOTTAFLUXD"
check_binary "$YOTTAFLUXCLI"
ok "Binaries found."

# ---------------------------------------------------------------------------
# Check if already running
# ---------------------------------------------------------------------------
if $YOTTAFLUXCLI $CLI_FLAGS getblockchaininfo >/dev/null 2>&1; then
    warn "Devmode node is already running on RPC port $RPC_PORT."
    BLOCK_HEIGHT=$($YOTTAFLUXCLI $CLI_FLAGS getblockchaininfo | grep -o '"blocks":[0-9]*' | grep -o '[0-9]*')
    info "Current block height: $BLOCK_HEIGHT"
    info "Use '$0 --stop' to stop or '$0 --clean' to reset."
    exit 0
fi

# ---------------------------------------------------------------------------
# Set up data directory
# ---------------------------------------------------------------------------
info "Setting up data directory: $DATADIR"
mkdir -p "$DATADIR"

if [ ! -f "$REGTEST_CONF" ]; then
    error "Regtest config not found: $REGTEST_CONF"
    exit 1
fi

cp "$REGTEST_CONF" "$DATADIR/yottaflux.conf"
ok "Copied regtest config to $DATADIR/yottaflux.conf"

# ---------------------------------------------------------------------------
# Start the node
# ---------------------------------------------------------------------------
trap cleanup_on_error ERR

info "Starting yottafluxd in regtest mode..."
$YOTTAFLUXD -regtest -datadir="$DATADIR" -conf="$DATADIR/yottaflux.conf" -daemon

wait_for_rpc

# ---------------------------------------------------------------------------
# Mine initial blocks
# ---------------------------------------------------------------------------
info "Mining 400 blocks (DGW activation + coinbase maturity + buffer)..."
$YOTTAFLUXCLI $CLI_FLAGS generate 400 >/dev/null
BLOCK_HEIGHT=$($YOTTAFLUXCLI $CLI_FLAGS getblockchaininfo | grep -o '"blocks":[0-9]*' | grep -o '[0-9]*')
ok "Mined to block height: $BLOCK_HEIGHT"

# ---------------------------------------------------------------------------
# Get wallet info
# ---------------------------------------------------------------------------
WALLET_ADDRESS=$($YOTTAFLUXCLI $CLI_FLAGS getnewaddress)
BALANCE=$($YOTTAFLUXCLI $CLI_FLAGS getbalance)
ok "Wallet address: $WALLET_ADDRESS"
ok "YFX balance:    $BALANCE"

# ---------------------------------------------------------------------------
# Create a sample asset
# ---------------------------------------------------------------------------
info "Creating sample asset DEVTOKEN (1000 units)..."
ASSET_TXID=$($YOTTAFLUXCLI $CLI_FLAGS issue "DEVTOKEN" 1000 2>/dev/null | grep -o '"[a-f0-9]\{64\}"' | head -1 | tr -d '"') || true

if [ -n "${ASSET_TXID:-}" ]; then
    # Mine a block to confirm the asset transaction.
    $YOTTAFLUXCLI $CLI_FLAGS generate 1 >/dev/null
    ok "Asset DEVTOKEN created (txid: ${ASSET_TXID:0:16}...)"
else
    warn "Could not create DEVTOKEN (asset may already exist or issue command differs)."
fi

# Disable the error trap now that setup is complete.
trap - ERR

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
echo -e "${BOLD}=== Devmode Environment Ready ===${NC}"
echo ""
echo -e "  ${CYAN}Node status:${NC}    Running (regtest, block height $BLOCK_HEIGHT)"
echo -e "  ${CYAN}RPC endpoint:${NC}   127.0.0.1:$RPC_PORT  (user: devuser / pass: devpass)"
echo -e "  ${CYAN}Data directory:${NC} $DATADIR"
echo -e "  ${CYAN}Wallet address:${NC} $WALLET_ADDRESS"
echo -e "  ${CYAN}YFX balance:${NC}    $BALANCE YFX"
if [ -n "${ASSET_TXID:-}" ]; then
echo -e "  ${CYAN}Sample asset:${NC}   DEVTOKEN (1000 units)"
fi
echo ""
echo -e "${BOLD}Interact with the node:${NC}"
echo ""
echo "  # Check blockchain info"
echo "  $YOTTAFLUXCLI $CLI_FLAGS getblockchaininfo"
echo ""
echo "  # Mine more blocks"
echo "  $YOTTAFLUXCLI $CLI_FLAGS generate 10"
echo ""
echo "  # List assets"
echo "  $YOTTAFLUXCLI $CLI_FLAGS listmyassets"
echo ""
echo "  # Get wallet balance"
echo "  $YOTTAFLUXCLI $CLI_FLAGS getbalance"
echo ""
echo -e "${BOLD}Stop the node:${NC}"
echo ""
echo "  $0 --stop"
echo "  # or: $YOTTAFLUXCLI $CLI_FLAGS stop"
echo ""
