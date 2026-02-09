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

BOOST_AUTO_TEST_CASE(stake_marker_v2_roundtrip_test)
{
    BOOST_TEST_MESSAGE("Running Stake Marker v2 Build/Parse Roundtrip Test");

    std::vector<unsigned char> vchPubKeyHash(20, 0xCD);
    uint160 pubKeyHash(vchPubKeyHash);
    uint8_t version = YFX_STAKE_VERSION_2;
    uint32_t unlock_height = 12345;
    uint32_t lock_duration = 500;
    std::string description = "a knight with a sword";

    CScript markerScript = BuildStakeMarkerScript(version, unlock_height, lock_duration, pubKeyHash, description);

    // Verify it's detected as a stake marker
    BOOST_CHECK_MESSAGE(IsStakeMarkerScript(markerScript), "Script should be recognized as a stake marker");

    // Parse it back
    uint8_t parsed_version;
    uint32_t parsed_unlock_height, parsed_lock_duration;
    uint160 parsed_pubKeyHash;
    std::string parsed_description;
    BOOST_CHECK_MESSAGE(
        ParseStakeMarker(markerScript, parsed_version, parsed_unlock_height, parsed_lock_duration, parsed_pubKeyHash, parsed_description),
        "Failed to parse stake marker"
    );

    BOOST_CHECK_EQUAL(parsed_version, version);
    BOOST_CHECK_EQUAL(parsed_unlock_height, unlock_height);
    BOOST_CHECK_EQUAL(parsed_lock_duration, lock_duration);
    BOOST_CHECK(parsed_pubKeyHash == pubKeyHash);
    BOOST_CHECK_EQUAL(parsed_description, description);
}

BOOST_AUTO_TEST_CASE(stake_marker_v2_empty_description_test)
{
    BOOST_TEST_MESSAGE("Running Stake Marker v2 Empty Description Test");

    std::vector<unsigned char> vchPubKeyHash(20, 0xCD);
    uint160 pubKeyHash(vchPubKeyHash);

    CScript markerScript = BuildStakeMarkerScript(YFX_STAKE_VERSION_2, 100, 10, pubKeyHash, "");
    BOOST_CHECK(IsStakeMarkerScript(markerScript));

    uint8_t v; uint32_t uh, ld; uint160 pk; std::string desc;
    BOOST_CHECK(ParseStakeMarker(markerScript, v, uh, ld, pk, desc));
    BOOST_CHECK_EQUAL(desc, "");
}

BOOST_AUTO_TEST_CASE(stake_marker_v1_backward_compat_test)
{
    BOOST_TEST_MESSAGE("Running Stake Marker v1 Backward Compatibility Test");

    // Build a v1 marker (no description)
    std::vector<unsigned char> vchPubKeyHash(20, 0xAA);
    uint160 pubKeyHash(vchPubKeyHash);

    // Manually build v1 payload (38 bytes, no description)
    std::vector<unsigned char> data;
    data.insert(data.end(), YFX_STAKE_MAGIC.begin(), YFX_STAKE_MAGIC.end());
    data.push_back(YFX_STAKE_VERSION_1);
    uint32_t uh = 200; uint32_t ld = 50;
    data.push_back(uh & 0xFF); data.push_back((uh >> 8) & 0xFF); data.push_back((uh >> 16) & 0xFF); data.push_back((uh >> 24) & 0xFF);
    data.push_back(ld & 0xFF); data.push_back((ld >> 8) & 0xFF); data.push_back((ld >> 16) & 0xFF); data.push_back((ld >> 24) & 0xFF);
    const unsigned char* begin = pubKeyHash.begin();
    data.insert(data.end(), begin, begin + 20);

    CScript script;
    script << OP_RETURN << data;

    BOOST_CHECK(IsStakeMarkerScript(script));

    uint8_t v; uint32_t puh, pld; uint160 pk; std::string desc;
    BOOST_CHECK(ParseStakeMarker(script, v, puh, pld, pk, desc));
    BOOST_CHECK_EQUAL(v, YFX_STAKE_VERSION_1);
    BOOST_CHECK_EQUAL(puh, 200u);
    BOOST_CHECK_EQUAL(pld, 50u);
    BOOST_CHECK_EQUAL(desc, ""); // v1 has no description
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
    entry.description = "a dragon breathing fire";
    entry.reward_txid = uint256S("0xfedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321");

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
    BOOST_CHECK_EQUAL(decoded.description, entry.description);
    BOOST_CHECK(decoded.reward_txid == entry.reward_txid);
}

BOOST_AUTO_TEST_CASE(stake_marker_large_height_test)
{
    BOOST_TEST_MESSAGE("Running Stake Marker Large Height Test");

    std::vector<unsigned char> vchPubKeyHash(20, 0x11);
    uint160 pubKeyHash(vchPubKeyHash);
    uint32_t unlock_height = 0xFFFFFFFF; // max uint32
    uint32_t lock_duration = 0x7FFFFFFF;

    CScript markerScript = BuildStakeMarkerScript(YFX_STAKE_VERSION_2, unlock_height, lock_duration, pubKeyHash);
    BOOST_CHECK(IsStakeMarkerScript(markerScript));

    uint8_t parsed_version;
    uint32_t parsed_unlock, parsed_lock;
    uint160 parsed_hash;
    std::string parsed_desc;
    BOOST_CHECK(ParseStakeMarker(markerScript, parsed_version, parsed_unlock, parsed_lock, parsed_hash, parsed_desc));
    BOOST_CHECK_EQUAL(parsed_unlock, unlock_height);
    BOOST_CHECK_EQUAL(parsed_lock, lock_duration);
}

BOOST_AUTO_TEST_CASE(reward_marker_roundtrip_test)
{
    BOOST_TEST_MESSAGE("Running Reward Marker Build/Parse Roundtrip Test");

    uint256 stake_txid = uint256S("0xabcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890");

    CScript rewardScript = BuildRewardMarkerScript(YFX_REWARD_VERSION, stake_txid);

    BOOST_CHECK(IsRewardMarkerScript(rewardScript));

    uint8_t parsed_version;
    uint256 parsed_stake_txid;
    BOOST_CHECK(ParseRewardMarker(rewardScript, parsed_version, parsed_stake_txid));
    BOOST_CHECK_EQUAL(parsed_version, YFX_REWARD_VERSION);
    BOOST_CHECK(parsed_stake_txid == stake_txid);
}

BOOST_AUTO_TEST_CASE(stake_marker_max_description_test)
{
    BOOST_TEST_MESSAGE("Running Stake Marker Max Description Length Test");

    std::vector<unsigned char> vchPubKeyHash(20, 0xBB);
    uint160 pubKeyHash(vchPubKeyHash);
    std::string maxDesc(YFX_STAKE_MAX_DESC_LEN, 'X'); // 40 chars

    CScript markerScript = BuildStakeMarkerScript(YFX_STAKE_VERSION_2, 100, 10, pubKeyHash, maxDesc);
    BOOST_CHECK(IsStakeMarkerScript(markerScript));

    uint8_t v; uint32_t uh, ld; uint160 pk; std::string desc;
    BOOST_CHECK(ParseStakeMarker(markerScript, v, uh, ld, pk, desc));
    BOOST_CHECK_EQUAL(desc.size(), YFX_STAKE_MAX_DESC_LEN);
    BOOST_CHECK_EQUAL(desc, maxDesc);
}

BOOST_AUTO_TEST_CASE(stakingdb_reward_update_test)
{
    BOOST_TEST_MESSAGE("Running StakingDB Reward Update Test");

    // Create an in-memory staking DB
    CStakingDB db(1 << 20, true);

    // Build a stake entry with null reward_txid
    CStakeEntry entry;
    entry.txid = uint256S("0x1111111111111111111111111111111111111111111111111111111111111111");
    entry.vout = 0;
    entry.amount = 50000000000;
    entry.create_height = 100;
    entry.unlock_height = 200;
    entry.lock_duration = 100;
    entry.staker_address = "YTestAddr";
    entry.pubkey_hash = uint160(std::vector<unsigned char>(20, 0xAA));
    entry.status = STAKE_ACTIVE;
    entry.description = "test stake";

    BOOST_CHECK(db.WriteStake(entry));

    // Verify reward_txid starts null
    CStakeEntry readEntry;
    BOOST_CHECK(db.ReadStake(entry.txid, readEntry));
    BOOST_CHECK(readEntry.reward_txid.IsNull());

    // Update reward_txid
    uint256 reward_txid = uint256S("0x2222222222222222222222222222222222222222222222222222222222222222");
    BOOST_CHECK(db.UpdateStakeReward(entry.txid, reward_txid));

    // Verify reward_txid is set
    CStakeEntry afterUpdate;
    BOOST_CHECK(db.ReadStake(entry.txid, afterUpdate));
    BOOST_CHECK(afterUpdate.reward_txid == reward_txid);

    // Clear reward_txid
    BOOST_CHECK(db.ClearStakeReward(entry.txid));

    // Verify reward_txid is null again
    CStakeEntry afterClear;
    BOOST_CHECK(db.ReadStake(entry.txid, afterClear));
    BOOST_CHECK(afterClear.reward_txid.IsNull());

    // UpdateStakeReward on non-existent entry should return false
    uint256 fakeTxid = uint256S("0x9999999999999999999999999999999999999999999999999999999999999999");
    BOOST_CHECK(!db.UpdateStakeReward(fakeTxid, reward_txid));

    // ClearStakeReward on non-existent entry should return false
    BOOST_CHECK(!db.ClearStakeReward(fakeTxid));
}

BOOST_AUTO_TEST_SUITE_END()
