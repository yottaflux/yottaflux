// Copyright (c) 2025 The Yottaflux developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef YOTTAFLUX_STAKING_H
#define YOTTAFLUX_STAKING_H

#include "pubkey.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

#include <cstdint>
#include <string>
#include <vector>

/** 9-byte magic prefix for staking OP_RETURN markers */
static const std::string YFX_STAKE_MAGIC = "YFX_STAKE";

/** Current staking marker version */
static const uint8_t YFX_STAKE_VERSION = 0x01;

/** Total size of OP_RETURN payload: magic(9) + version(1) + unlock_height(4) + lock_duration(4) + pubKeyHash(20) = 38 */
static const size_t YFX_STAKE_MARKER_SIZE = 38;

/**
 * Build a CLTV redeem script for staking.
 * Script: <unlock_height> OP_CHECKLOCKTIMEVERIFY OP_DROP OP_DUP OP_HASH160 <pubKeyHash> OP_EQUALVERIFY OP_CHECKSIG
 */
inline CScript BuildStakeRedeemScript(int64_t unlock_height, const CKeyID& pubKeyHash)
{
    CScript script;
    script << unlock_height << OP_CHECKLOCKTIMEVERIFY << OP_DROP;
    script << OP_DUP << OP_HASH160 << ToByteVector(pubKeyHash) << OP_EQUALVERIFY << OP_CHECKSIG;
    return script;
}

/**
 * Build an OP_RETURN marker script for staking identification.
 * Payload: "YFX_STAKE" (9) | version (1) | unlock_height (4 LE) | lock_duration (4 LE) | pubKeyHash (20)
 */
inline CScript BuildStakeMarkerScript(uint8_t version, uint32_t unlock_height, uint32_t lock_duration, const uint160& pubKeyHash)
{
    std::vector<unsigned char> data;
    data.reserve(YFX_STAKE_MARKER_SIZE);

    // Magic prefix
    data.insert(data.end(), YFX_STAKE_MAGIC.begin(), YFX_STAKE_MAGIC.end());

    // Version
    data.push_back(version);

    // unlock_height (little-endian 4 bytes)
    data.push_back(unlock_height & 0xFF);
    data.push_back((unlock_height >> 8) & 0xFF);
    data.push_back((unlock_height >> 16) & 0xFF);
    data.push_back((unlock_height >> 24) & 0xFF);

    // lock_duration (little-endian 4 bytes)
    data.push_back(lock_duration & 0xFF);
    data.push_back((lock_duration >> 8) & 0xFF);
    data.push_back((lock_duration >> 16) & 0xFF);
    data.push_back((lock_duration >> 24) & 0xFF);

    // pubKeyHash (20 bytes)
    const unsigned char* begin = pubKeyHash.begin();
    data.insert(data.end(), begin, begin + 20);

    CScript script;
    script << OP_RETURN << data;
    return script;
}

/**
 * Check if a script output is a YFX_STAKE OP_RETURN marker.
 */
inline bool IsStakeMarkerScript(const CScript& script)
{
    // Minimum: OP_RETURN (1) + pushdata opcode (1-2) + 38 bytes payload
    if (script.size() < 40)
        return false;

    if (script[0] != OP_RETURN)
        return false;

    // Extract the pushed data
    CScript::const_iterator it = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vData;
    ++it; // skip OP_RETURN
    if (!script.GetOp(it, opcode, vData))
        return false;

    if (vData.size() != YFX_STAKE_MARKER_SIZE)
        return false;

    // Check magic prefix
    if (std::string(vData.begin(), vData.begin() + 9) != YFX_STAKE_MAGIC)
        return false;

    return true;
}

/**
 * Parse a YFX_STAKE OP_RETURN marker script, extracting metadata fields.
 * Returns false if the script is not a valid staking marker.
 */
inline bool ParseStakeMarker(const CScript& script, uint8_t& version, uint32_t& unlock_height, uint32_t& lock_duration, uint160& pubKeyHash)
{
    if (script[0] != OP_RETURN)
        return false;

    CScript::const_iterator it = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vData;
    ++it; // skip OP_RETURN
    if (!script.GetOp(it, opcode, vData))
        return false;

    if (vData.size() != YFX_STAKE_MARKER_SIZE)
        return false;

    if (std::string(vData.begin(), vData.begin() + 9) != YFX_STAKE_MAGIC)
        return false;

    size_t pos = 9;

    // Version
    version = vData[pos++];

    // unlock_height (little-endian)
    unlock_height = vData[pos] | (vData[pos + 1] << 8) | (vData[pos + 2] << 16) | (vData[pos + 3] << 24);
    pos += 4;

    // lock_duration (little-endian)
    lock_duration = vData[pos] | (vData[pos + 1] << 8) | (vData[pos + 2] << 16) | (vData[pos + 3] << 24);
    pos += 4;

    // pubKeyHash (20 bytes)
    pubKeyHash = uint160(std::vector<unsigned char>(vData.begin() + pos, vData.begin() + pos + 20));

    return true;
}

#endif // YOTTAFLUX_STAKING_H
