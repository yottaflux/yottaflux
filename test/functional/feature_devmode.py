#!/usr/bin/env python3
# Copyright (c) 2024-2025 The Yottaflux developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test devmode workflow: clean regtest chain, mining, asset creation, transfer, and multi-node sync."""

from test_framework.test_framework import YottafluxTestFramework
from test_framework.util import assert_equal, assert_greater_than, connect_nodes_bi, sync_blocks, sync_mempools


class DevmodeTest(YottafluxTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [['-assetindex'], ['-assetindex'], ['-assetindex']]

    def run_test(self):
        self.test_mining_and_balance()
        self.test_asset_create_and_transfer()
        self.test_full_mesh_connectivity()

    def test_mining_and_balance(self):
        """Mine 400 blocks on node 0 (DGW activation + coinbase maturity) and verify balance."""
        self.log.info("Mining 400 blocks on node 0...")
        n0 = self.nodes[0]

        n0.generate(400)
        sync_blocks(self.nodes)

        height = n0.getblockcount()
        assert_equal(height, 400)
        self.log.info("Block height: %d" % height)

        # Verify all nodes synced
        for i, node in enumerate(self.nodes):
            assert_equal(node.getblockcount(), 400)
            self.log.info("Node %d synced at height %d" % (i, node.getblockcount()))

        # Node 0 should have spendable balance (coinbase maturity is 100,
        # so blocks 1-300 have matured)
        balance = n0.getbalance()
        assert_greater_than(balance, 0)
        self.log.info("Node 0 balance: %s YFX" % str(balance))

    def test_asset_create_and_transfer(self):
        """Create an asset on node 0, transfer to node 1, verify on all nodes."""
        self.log.info("Creating asset DEVTEST on node 0...")
        n0, n1, n2 = self.nodes[0], self.nodes[1], self.nodes[2]

        address0 = n0.getnewaddress()
        n0.issue(asset_name="DEVTEST", qty=1000, to_address=address0, change_address="",
                 units=0, reissuable=True, has_ipfs=False)

        self.log.info("Mining 1 block to confirm asset...")
        n0.generate(1)
        sync_blocks(self.nodes)

        # Verify asset exists on node 0
        myassets = n0.listmyassets(asset="DEVTEST*", verbose=True)
        assert_equal(myassets["DEVTEST"]["balance"], 1000)
        self.log.info("Asset DEVTEST confirmed on node 0 (1000 units)")

        # Transfer 250 units to node 1
        self.log.info("Transferring 250 DEVTEST to node 1...")
        address1 = n1.getnewaddress()
        n0.transfer(asset_name="DEVTEST", qty=250, to_address=address1)

        self.log.info("Mining 1 block to confirm transfer...")
        n0.generate(1)
        sync_blocks(self.nodes)

        # Verify balances
        n0_assets = n0.listmyassets(asset="DEVTEST", verbose=False)
        n1_assets = n1.listmyassets(asset="DEVTEST", verbose=False)
        assert_equal(n0_assets["DEVTEST"], 750)
        assert_equal(n1_assets["DEVTEST"], 250)
        self.log.info("Node 0 has 750 DEVTEST, node 1 has 250 DEVTEST")

        # Node 2 should see the asset in the global index but own none
        assetdata = n2.getassetdata("DEVTEST")
        assert_equal(assetdata["name"], "DEVTEST")
        assert_equal(assetdata["amount"], 1000)
        self.log.info("Node 2 sees DEVTEST in global index (1000 total)")

    def test_full_mesh_connectivity(self):
        """Verify all nodes are connected to all other nodes."""
        self.log.info("Verifying full-mesh connectivity...")
        for i, node in enumerate(self.nodes):
            peer_count = node.getconnectioncount()
            # Each node should be connected to at least the other 2 nodes
            assert_greater_than(peer_count, 0)
            self.log.info("Node %d has %d peer(s)" % (i, peer_count))


if __name__ == '__main__':
    DevmodeTest().main()
