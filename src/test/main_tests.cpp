// Copyright (c) 2014-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2024 The Yottaflux Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "validation.h"
#include "net.h"

#include "test/test_yottaflux.h"

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

    // Yottaflux uses a hardcoded halving table in GetBlockSubsidy rather than
    // uniform nSubsidyHalvingInterval-based halvings. Test the actual table.
    BOOST_AUTO_TEST_CASE(block_subsidy_test)
    {
        BOOST_TEST_MESSAGE("Running Block Subsidy Test");

        const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
        const Consensus::Params& consensusParams = chainParams->GetConsensus();
        CAmount nInitialSubsidy = 5000 * COIN;

        // Hardcoded halving boundaries and expected shift amounts
        struct HalvingEntry {
            int nHeight;
            int nShift;
        };
        HalvingEntry halvings[] = {
            {0,       0},
            {129600,  1},
            {187200,  2},
            {270720,  3},
            {391680,  4},
            {567360,  5},
            {822240,  6},
            {1190880, 7},
            {1726560, 8},
            {2502720, 9},
            {3628800, 10},
            {5261760, 11},
            {7629120, 12},
        };

        for (const auto& entry : halvings) {
            CAmount nExpected = nInitialSubsidy >> entry.nShift;
            // Check at the boundary
            BOOST_CHECK_EQUAL(GetBlockSubsidy(entry.nHeight, consensusParams), nExpected);
            // Check one block before the boundary (except for genesis)
            if (entry.nHeight > 0) {
                CAmount nPrevExpected = nInitialSubsidy >> (entry.nShift - 1);
                BOOST_CHECK_EQUAL(GetBlockSubsidy(entry.nHeight - 1, consensusParams), nPrevExpected);
            }
        }

        // After the last halving range, subsidy should be 0
        BOOST_CHECK_EQUAL(GetBlockSubsidy(11062080, consensusParams), 0);
        BOOST_CHECK_EQUAL(GetBlockSubsidy(20000000, consensusParams), 0);
    }

    BOOST_AUTO_TEST_CASE(subsidy_limit_test)
    {
        BOOST_TEST_MESSAGE("Running Subsidy Limit Test");

        const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
        CAmount nSum = 0;
        for (int nHeight = 0; nHeight < 14000000; nHeight += 1000)
        {
            CAmount nSubsidy = GetBlockSubsidy(nHeight, chainParams->GetConsensus());
            BOOST_CHECK(nSubsidy <= 5000 * COIN);
            nSum += nSubsidy * 1000;
            BOOST_CHECK(MoneyRange(nSum));
        }
        // Total supply from Yottaflux hardcoded halving table (sampled at 1000-block intervals)
        BOOST_CHECK_EQUAL(nSum, (int64_t)116300415037346000LL);
    }

    bool ReturnFalse()
    { return false; }

    bool ReturnTrue()
    { return true; }

    BOOST_AUTO_TEST_CASE(combiner_all_test)
    {
        BOOST_TEST_MESSAGE("Running Combiner All Test");

        boost::signals2::signal<bool(), CombinerAll> Test;
        BOOST_CHECK(Test());
        Test.connect(&ReturnFalse);
        BOOST_CHECK(!Test());
        Test.connect(&ReturnTrue);
        BOOST_CHECK(!Test());
        Test.disconnect(&ReturnFalse);
        BOOST_CHECK(Test());
        Test.disconnect(&ReturnTrue);
        BOOST_CHECK(Test());
    }

BOOST_AUTO_TEST_SUITE_END()
