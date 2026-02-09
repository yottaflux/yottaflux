// Copyright (c) 2025 The Yottaflux developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_yottaflux.h>
#include <boost/test/unit_test.hpp>

#include <staking/staking.h>
#include <staking/stakingdb.h>
#include <pubkey.h>
#include <uint256.h>
#include <streams.h>
#include <script/script.h>

BOOST_FIXTURE_TEST_SUITE(staking_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(stake_redeem_script_test)
{
    BOOST_TEST_MESSAGE("Running Stake Redeem Script Test");

    // Create a dummy key ID
    std::vector<unsigned char> vchPubKeyHash(20, 0xAB);
    uint160 hash160(vchPubKeyHash);
    CKeyID keyID(hash160);

    int64_t unlock_height = 1000;
    CScript redeemScript = BuildStakeRedeemScript(unlock_height, keyID);

    // Verify the script is non-empty
    BOOST_CHECK(redeemScript.size() > 0);

    // Verify it contains CLTV
    bool hasCLTV = false;
    CScript::const_iterator it = redeemScript.begin();
    while (it < redeemScript.end()) {
        opcodetype opcode;
        std::vector<unsigned char> data;
        if (!redeemScript.GetOp(it, opcode, data))
            break;
        if (opcode == OP_CHECKLOCKTIMEVERIFY)
            hasCLTV = true;
    }
    BOOST_CHECK_MESSAGE(hasCLTV, "Redeem script must contain OP_CHECKLOCKTIMEVERIFY");
}

BOOST_AUTO_TEST_CASE(stake_marker_build_parse_roundtrip_test)
{
    BOOST_TEST_MESSAGE("Running Stake Marker Build/Parse Roundtrip Test");

    std::vector<unsigned char> vchPubKeyHash(20, 0xCD);
    uint160 pubKeyHash(vchPubKeyHash);
    uint8_t version = YFX_STAKE_VERSION;
    uint32_t unlock_height = 12345;
    uint32_t lock_duration = 500;

    CScript markerScript = BuildStakeMarkerScript(version, unlock_height, lock_duration, pubKeyHash);

    // Verify it's detected as a stake marker
    BOOST_CHECK_MESSAGE(IsStakeMarkerScript(markerScript), "Script should be recognized as a stake marker");

    // Parse it back
    uint8_t parsed_version;
    uint32_t parsed_unlock_height, parsed_lock_duration;
    uint160 parsed_pubKeyHash;
    BOOST_CHECK_MESSAGE(
        ParseStakeMarker(markerScript, parsed_version, parsed_unlock_height, parsed_lock_duration, parsed_pubKeyHash),
        "Failed to parse stake marker"
    );

    BOOST_CHECK_EQUAL(parsed_version, version);
    BOOST_CHECK_EQUAL(parsed_unlock_height, unlock_height);
    BOOST_CHECK_EQUAL(parsed_lock_duration, lock_duration);
    BOOST_CHECK(parsed_pubKeyHash == pubKeyHash);
}

BOOST_AUTO_TEST_CASE(stake_marker_detection_negative_test)
{
    BOOST_TEST_MESSAGE("Running Stake Marker Detection Negative Test");

    // A normal OP_RETURN script should not be detected as a stake marker
    CScript normalOpReturn;
    normalOpReturn << OP_RETURN << std::vector<unsigned char>{0x01, 0x02, 0x03};
    BOOST_CHECK(!IsStakeMarkerScript(normalOpReturn));

    // A P2PKH script should not be detected
    CScript p2pkh;
    p2pkh << OP_DUP << OP_HASH160 << std::vector<unsigned char>(20, 0xAA) << OP_EQUALVERIFY << OP_CHECKSIG;
    BOOST_CHECK(!IsStakeMarkerScript(p2pkh));

    // Empty script
    CScript empty;
    BOOST_CHECK(!IsStakeMarkerScript(empty));
}

BOOST_AUTO_TEST_CASE(stake_entry_serialization_roundtrip_test)
{
    BOOST_TEST_MESSAGE("Running CStakeEntry Serialization Roundtrip Test");

    CStakeEntry entry;
    entry.txid = uint256S("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    entry.vout = 0;
    entry.amount = 10000000000; // 100 YFX
    entry.create_height = 500;
    entry.unlock_height = 550;
    entry.lock_duration = 50;
    entry.staker_address = "YTestAddress123";
    entry.pubkey_hash = uint160(std::vector<unsigned char>(20, 0xEF));
    entry.status = STAKE_ACTIVE;

    // Serialize
    CDataStream ss(SER_DISK, CLIENT_VERSION);
    ss << entry;

    // Deserialize
    CStakeEntry decoded;
    ss >> decoded;

    BOOST_CHECK(decoded.txid == entry.txid);
    BOOST_CHECK_EQUAL(decoded.vout, entry.vout);
    BOOST_CHECK_EQUAL(decoded.amount, entry.amount);
    BOOST_CHECK_EQUAL(decoded.create_height, entry.create_height);
    BOOST_CHECK_EQUAL(decoded.unlock_height, entry.unlock_height);
    BOOST_CHECK_EQUAL(decoded.lock_duration, entry.lock_duration);
    BOOST_CHECK_EQUAL(decoded.staker_address, entry.staker_address);
    BOOST_CHECK(decoded.pubkey_hash == entry.pubkey_hash);
    BOOST_CHECK_EQUAL(decoded.status, entry.status);
}

BOOST_AUTO_TEST_CASE(stake_marker_large_height_test)
{
    BOOST_TEST_MESSAGE("Running Stake Marker Large Height Test");

    std::vector<unsigned char> vchPubKeyHash(20, 0x11);
    uint160 pubKeyHash(vchPubKeyHash);
    uint32_t unlock_height = 0xFFFFFFFF; // max uint32
    uint32_t lock_duration = 0x7FFFFFFF;

    CScript markerScript = BuildStakeMarkerScript(YFX_STAKE_VERSION, unlock_height, lock_duration, pubKeyHash);
    BOOST_CHECK(IsStakeMarkerScript(markerScript));

    uint8_t parsed_version;
    uint32_t parsed_unlock, parsed_lock;
    uint160 parsed_hash;
    BOOST_CHECK(ParseStakeMarker(markerScript, parsed_version, parsed_unlock, parsed_lock, parsed_hash));
    BOOST_CHECK_EQUAL(parsed_unlock, unlock_height);
    BOOST_CHECK_EQUAL(parsed_lock, lock_duration);
}

BOOST_AUTO_TEST_SUITE_END()
