// Copyright (c) 2025 The Yottaflux developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "staking/stakingdb.h"
#include "util.h"

#include <boost/thread.hpp>

/** DB key prefixes */
static const char STAKE_FLAG = 'K';   // Primary: ('K', txid) -> CStakeEntry
static const char HEIGHT_FLAG = 'H';  // Secondary: ('H', (unlock_height, txid)) -> empty

CStakingDB::CStakingDB(size_t nCacheSize, bool fMemory, bool fWipe)
    : CDBWrapper(GetDataDir() / "staking" / "index", nCacheSize, fMemory, fWipe)
{
}

bool CStakingDB::WriteStake(const CStakeEntry& entry)
{
    LogPrint(BCLog::STAKING, "%s : Writing stake: txid=%s, amount=%lld, unlock_height=%d\n",
        __func__, entry.txid.GetHex(), entry.amount, entry.unlock_height);

    CDBBatch batch(*this);

    // Primary key
    batch.Write(std::make_pair(STAKE_FLAG, entry.txid), entry);

    // Height index for unlock lookups
    batch.Write(std::make_pair(HEIGHT_FLAG, std::make_pair(entry.unlock_height, entry.txid)), char('1'));

    return WriteBatch(batch);
}

bool CStakingDB::ReadStake(const uint256& txid, CStakeEntry& entry)
{
    return Read(std::make_pair(STAKE_FLAG, txid), entry);
}

bool CStakingDB::EraseStake(const uint256& txid)
{
    CStakeEntry entry;
    if (!ReadStake(txid, entry))
        return false;

    LogPrint(BCLog::STAKING, "%s : Erasing stake: txid=%s\n", __func__, txid.GetHex());

    CDBBatch batch(*this);

    // Remove primary key
    batch.Erase(std::make_pair(STAKE_FLAG, txid));

    // Remove height index
    batch.Erase(std::make_pair(HEIGHT_FLAG, std::make_pair(entry.unlock_height, txid)));

    return WriteBatch(batch);
}

bool CStakingDB::UpdateStakeStatus(const uint256& txid, uint8_t newStatus)
{
    CStakeEntry entry;
    if (!ReadStake(txid, entry))
        return false;

    LogPrint(BCLog::STAKING, "%s : Updating stake status: txid=%s, old=%d, new=%d\n",
        __func__, txid.GetHex(), entry.status, newStatus);

    entry.status = newStatus;
    return Write(std::make_pair(STAKE_FLAG, txid), entry);
}

bool CStakingDB::UpdateStakeReward(const uint256& txid, const uint256& reward_txid)
{
    CStakeEntry entry;
    if (!ReadStake(txid, entry))
        return false;

    LogPrint(BCLog::STAKING, "%s : Setting reward_txid on stake: txid=%s, reward_txid=%s\n",
        __func__, txid.GetHex(), reward_txid.GetHex());

    entry.reward_txid = reward_txid;
    return Write(std::make_pair(STAKE_FLAG, txid), entry);
}

bool CStakingDB::ClearStakeReward(const uint256& txid)
{
    CStakeEntry entry;
    if (!ReadStake(txid, entry))
        return false;

    LogPrint(BCLog::STAKING, "%s : Clearing reward_txid on stake: txid=%s\n",
        __func__, txid.GetHex());

    entry.reward_txid.SetNull();
    return Write(std::make_pair(STAKE_FLAG, txid), entry);
}

bool CStakingDB::GetAllStakes(std::vector<CStakeEntry>& entries, int filterStatus)
{
    entries.clear();

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(STAKE_FLAG, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;

        if (pcursor->GetKey(key) && key.first == STAKE_FLAG) {
            CStakeEntry entry;
            if (pcursor->GetValue(entry)) {
                if (filterStatus < 0 || entry.status == (uint8_t)filterStatus) {
                    entries.push_back(entry);
                }
            } else {
                LogPrint(BCLog::STAKING, "%s : Failed to read stake entry\n", __func__);
            }
        } else {
            break;
        }

        pcursor->Next();
    }

    return true;
}

bool CStakingDB::GetStakesUnlockingAtHeight(int height, std::vector<CStakeEntry>& entries)
{
    entries.clear();

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(HEIGHT_FLAG, std::make_pair(height, uint256())));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, std::pair<int, uint256>> key;

        if (pcursor->GetKey(key) && key.first == HEIGHT_FLAG && key.second.first == height) {
            CStakeEntry entry;
            if (ReadStake(key.second.second, entry)) {
                entries.push_back(entry);
            }
        } else {
            break;
        }

        pcursor->Next();
    }

    return true;
}

bool CStakingDB::GetStakesCreatedAtHeight(int height, std::vector<CStakeEntry>& entries)
{
    entries.clear();

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(STAKE_FLAG, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;

        if (pcursor->GetKey(key) && key.first == STAKE_FLAG) {
            CStakeEntry entry;
            if (pcursor->GetValue(entry)) {
                if (entry.create_height == height) {
                    entries.push_back(entry);
                }
            }
        } else {
            break;
        }

        pcursor->Next();
    }

    return true;
}
