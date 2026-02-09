// Copyright (c) 2025 The Yottaflux developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef YOTTAFLUX_STAKINGDB_H
#define YOTTAFLUX_STAKINGDB_H

#include <dbwrapper.h>
#include "amount.h"
#include "serialize.h"
#include "uint256.h"

#include <string>
#include <vector>

/** Stake status values */
enum StakeStatus : uint8_t {
    STAKE_ACTIVE = 0,
    STAKE_UNLOCKED = 1,
};

/** A single staking index entry */
class CStakeEntry
{
public:
    uint256 txid;
    uint32_t vout;
    CAmount amount;
    int create_height;
    int unlock_height;
    int lock_duration;
    std::string staker_address;
    uint160 pubkey_hash;
    uint8_t status;
    std::string description;
    uint256 reward_txid;

    CStakeEntry()
    {
        SetNull();
    }

    void SetNull()
    {
        txid.SetNull();
        vout = 0;
        amount = 0;
        create_height = 0;
        unlock_height = 0;
        lock_duration = 0;
        staker_address = "";
        pubkey_hash.SetNull();
        status = STAKE_ACTIVE;
        description = "";
        reward_txid.SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(txid);
        READWRITE(vout);
        READWRITE(amount);
        READWRITE(create_height);
        READWRITE(unlock_height);
        READWRITE(lock_duration);
        READWRITE(staker_address);
        READWRITE(pubkey_hash);
        READWRITE(status);
        READWRITE(description);
        READWRITE(reward_txid);
    }
};

/** LevelDB-backed staking index */
class CStakingDB : public CDBWrapper
{
public:
    explicit CStakingDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CStakingDB(const CStakingDB&) = delete;
    CStakingDB& operator=(const CStakingDB&) = delete;

    /** Write a new stake entry (primary + height index) */
    bool WriteStake(const CStakeEntry& entry);

    /** Read a stake entry by txid */
    bool ReadStake(const uint256& txid, CStakeEntry& entry);

    /** Erase a stake entry (primary + height index) */
    bool EraseStake(const uint256& txid);

    /** Update status of an existing stake */
    bool UpdateStakeStatus(const uint256& txid, uint8_t newStatus);

    /** Set the reward_txid on an existing stake entry */
    bool UpdateStakeReward(const uint256& txid, const uint256& reward_txid);

    /** Clear the reward_txid on an existing stake entry (set to null) */
    bool ClearStakeReward(const uint256& txid);

    /** Get all stakes, optionally filtered by status (-1 = all) */
    bool GetAllStakes(std::vector<CStakeEntry>& entries, int filterStatus = -1);

    /** Get all stakes whose unlock_height matches the given height */
    bool GetStakesUnlockingAtHeight(int height, std::vector<CStakeEntry>& entries);

    /** Get all stakes whose create_height matches the given height */
    bool GetStakesCreatedAtHeight(int height, std::vector<CStakeEntry>& entries);
};

#endif // YOTTAFLUX_STAKINGDB_H
