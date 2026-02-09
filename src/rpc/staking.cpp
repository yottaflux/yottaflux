// Copyright (c) 2025 The Yottaflux developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"
#include "base58.h"
#include "chain.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "validation.h"
#include "net.h"
#include "rpc/safemode.h"
#include "rpc/server.h"
#include "script/standard.h"
#include "staking/staking.h"
#include "staking/stakingdb.h"
#include "util.h"
#include "utilmoneystr.h"

#ifdef ENABLE_WALLET
#include "wallet/coincontrol.h"
#include "wallet/wallet.h"
#include "wallet/rpcwallet.h"
#endif

#include <univalue.h>

/** Helper: serialize a CStakeEntry to a UniValue object (shared by list/info RPCs) */
static UniValue StakeEntryToJSON(const CStakeEntry& entry)
{
    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("txid", entry.txid.GetHex()));
    obj.push_back(Pair("vout", (int)entry.vout));
    obj.push_back(Pair("amount", ValueFromAmount(entry.amount)));
    obj.push_back(Pair("create_height", entry.create_height));
    obj.push_back(Pair("unlock_height", entry.unlock_height));
    obj.push_back(Pair("lock_duration", entry.lock_duration));
    obj.push_back(Pair("staker_address", entry.staker_address));
    obj.push_back(Pair("status", entry.status == STAKE_ACTIVE ? "active" : "unlocked"));
    obj.push_back(Pair("description", entry.description));
    if (!entry.reward_txid.IsNull())
        obj.push_back(Pair("reward_txid", entry.reward_txid.GetHex()));
    return obj;
}

#ifdef ENABLE_WALLET
UniValue stakecreate(const JSONRPCRequest& request)
{
    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3)
        throw std::runtime_error(
            "stakecreate amount lock_blocks ( \"description\" )\n"
            "\nCreate a staking transaction that locks coins for a specified number of blocks using CLTV.\n"
            "\nArguments:\n"
            "1. amount        (numeric, required) The amount in " + CURRENCY_UNIT + " to lock for staking.\n"
            "2. lock_blocks   (numeric, required) The number of blocks to lock the coins for (min 360 = 6 hours, max 525600 = 1 year).\n"
            "3. \"description\" (string, optional) A short text description for NFT generation (max 40 chars).\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\":           (string) The staking transaction id\n"
            "  \"p2sh_address\":   (string) The P2SH address holding the locked funds\n"
            "  \"unlock_height\":  (numeric) The block height at which funds become spendable\n"
            "  \"lock_blocks\":    (numeric) The number of blocks the coins are locked for\n"
            "  \"amount\":         (numeric) The amount locked\n"
            "  \"description\":    (string) The NFT description (if provided)\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("stakecreate", "100 50")
            + HelpExampleCli("stakecreate", "100 50 \"a knight with a sword\"")
            + HelpExampleRpc("stakecreate", "100, 50, \"a knight with a sword\"")
        );

    ObserveSafeMode();
    LOCK2(cs_main, pwallet->cs_wallet);

    // Parse parameters
    CAmount nAmount = AmountFromValue(request.params[0]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for staking");

    int lock_blocks = request.params[1].get_int();
    if (lock_blocks < MIN_STAKE_LOCK_BLOCKS)
        throw JSONRPCError(RPC_INVALID_PARAMETER,
            strprintf("lock_blocks must be at least %d (6 hours)", MIN_STAKE_LOCK_BLOCKS));
    if (lock_blocks > MAX_STAKE_LOCK_BLOCKS)
        throw JSONRPCError(RPC_INVALID_PARAMETER,
            strprintf("lock_blocks must be at most %d (1 year)", MAX_STAKE_LOCK_BLOCKS));

    std::string description;
    if (!request.params[2].isNull()) {
        description = request.params[2].get_str();
        if (description.size() > YFX_STAKE_MAX_DESC_LEN)
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                strprintf("description exceeds maximum length of %d characters", YFX_STAKE_MAX_DESC_LEN));
    }

    int current_height = chainActive.Height();
    int64_t unlock_height = current_height + lock_blocks;

    // Get a new key from the wallet
    CPubKey newKey;
    if (!pwallet->GetKeyFromPool(newKey)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    }
    CKeyID keyID = newKey.GetID();

    // Build the CLTV redeem script
    CScript redeemScript = BuildStakeRedeemScript(unlock_height, keyID);

    // Store the redeem script in the wallet so it can spend later
    pwallet->AddCScript(redeemScript);

    // Create P2SH output
    CScriptID innerID(redeemScript);
    CScript p2shScript = GetScriptForDestination(innerID);

    // Label the address in the wallet
    pwallet->SetAddressBook(innerID, "", "stake");

    // Build OP_RETURN marker (v2 with description)
    CScript opReturnScript = BuildStakeMarkerScript(
        YFX_STAKE_VERSION,
        (uint32_t)unlock_height,
        (uint32_t)lock_blocks,
        keyID,
        description
    );

    // Create the transaction with two outputs: P2SH (locked coins) + OP_RETURN (marker)
    std::vector<CRecipient> vecSend;
    vecSend.push_back({p2shScript, nAmount, false});
    vecSend.push_back({opReturnScript, 0, false});

    EnsureWalletIsUnlocked(pwallet);

    CWalletTx wtx;
    CReserveKey reservekey(pwallet);
    CAmount nFeeRequired;
    int nChangePosRet = -1;
    std::string strError;
    CCoinControl coin_control;

    if (!pwallet->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if (nAmount + nFeeRequired > pwallet->GetBalance())
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    CValidationState state;
    if (!pwallet->CommitTransaction(wtx, reservekey, g_connman.get(), state)) {
        strError = strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason());
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("txid", wtx.GetHash().GetHex()));
    result.push_back(Pair("p2sh_address", EncodeDestination(innerID)));
    result.push_back(Pair("unlock_height", unlock_height));
    result.push_back(Pair("lock_blocks", lock_blocks));
    result.push_back(Pair("amount", ValueFromAmount(nAmount)));
    result.push_back(Pair("description", description));
    return result;
}

UniValue stakereward(const JSONRPCRequest& request)
{
    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "stakereward \"stake_txid\" amount\n"
            "\nSend a reward payment for an unlocked stake. Atomically records the reward\n"
            "to prevent double-payment. The reward transaction includes an OP_RETURN marker\n"
            "linking it to the original stake for on-chain auditability.\n"
            "\nArguments:\n"
            "1. \"stake_txid\" (string, required) The txid of the unlocked stake to reward.\n"
            "2. amount         (numeric, required) The reward amount in " + CURRENCY_UNIT + " to send.\n"
            "\nResult:\n"
            "{\n"
            "  \"reward_txid\":   (string) The reward transaction id\n"
            "  \"stake_txid\":    (string) The original stake transaction id\n"
            "  \"staker_address\":(string) The address that received the reward\n"
            "  \"amount\":        (numeric) The reward amount sent\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("stakereward", "\"txid\" 5.0")
            + HelpExampleRpc("stakereward", "\"txid\", 5.0")
        );

    ObserveSafeMode();
    LOCK2(cs_main, pwallet->cs_wallet);

    if (!pStakingDb)
        throw JSONRPCError(RPC_DATABASE_ERROR, "Staking index not available");

    uint256 stake_txid = ParseHashV(request.params[0], "stake_txid");
    CAmount nRewardAmount = AmountFromValue(request.params[1]);
    if (nRewardAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid reward amount");

    // Read stake entry and validate
    CStakeEntry entry;
    if (!pStakingDb->ReadStake(stake_txid, entry))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Staking transaction not found in index");

    if (entry.status != STAKE_UNLOCKED)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Stake is not yet unlocked");

    if (!entry.reward_txid.IsNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER,
            strprintf("Stake already rewarded with txid %s", entry.reward_txid.GetHex()));

    // Build reward transaction: payment to staker + OP_RETURN marker
    CTxDestination dest = DecodeDestination(entry.staker_address);
    if (!IsValidDestination(dest))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid staker address in stake entry");

    CScript payScript = GetScriptForDestination(dest);
    CScript opReturnScript = BuildRewardMarkerScript(YFX_REWARD_VERSION, stake_txid);

    std::vector<CRecipient> vecSend;
    vecSend.push_back({payScript, nRewardAmount, false});
    vecSend.push_back({opReturnScript, 0, false});

    EnsureWalletIsUnlocked(pwallet);

    CWalletTx wtx;
    CReserveKey reservekey(pwallet);
    CAmount nFeeRequired;
    int nChangePosRet = -1;
    std::string strError;
    CCoinControl coin_control;

    if (!pwallet->CreateTransaction(vecSend, wtx, reservekey, nFeeRequired, nChangePosRet, strError, coin_control)) {
        if (nRewardAmount + nFeeRequired > pwallet->GetBalance())
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    CValidationState state;
    if (!pwallet->CommitTransaction(wtx, reservekey, g_connman.get(), state)) {
        strError = strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason());
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    // Atomically record the reward txid in the staking DB
    entry.reward_txid = wtx.GetHash();
    if (!pStakingDb->WriteStake(entry))
        LogPrintf("WARNING: stakereward: failed to write reward_txid to staking DB for %s\n", stake_txid.GetHex());

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("reward_txid", wtx.GetHash().GetHex()));
    result.push_back(Pair("stake_txid", stake_txid.GetHex()));
    result.push_back(Pair("staker_address", entry.staker_address));
    result.push_back(Pair("amount", ValueFromAmount(nRewardAmount)));
    return result;
}
#endif // ENABLE_WALLET

UniValue liststakes(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "liststakes ( \"status\" )\n"
            "\nList all staking entries from the staking index.\n"
            "\nArguments:\n"
            "1. \"status\"   (string, optional, default=\"all\") Filter by status: \"active\", \"unlocked\", or \"all\"\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\":           (string) The staking transaction id\n"
            "    \"vout\":           (numeric) The output index\n"
            "    \"amount\":         (numeric) The locked amount\n"
            "    \"create_height\":  (numeric) Block height when the stake was created\n"
            "    \"unlock_height\":  (numeric) Block height when the stake becomes spendable\n"
            "    \"lock_duration\":  (numeric) Original lock duration in blocks\n"
            "    \"staker_address\": (string) The staker's address\n"
            "    \"status\":         (string) \"active\" or \"unlocked\"\n"
            "    \"description\":    (string) NFT description text\n"
            "    \"reward_txid\":    (string) Reward transaction id (if rewarded)\n"
            "  }, ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("liststakes", "")
            + HelpExampleCli("liststakes", "\"active\"")
            + HelpExampleRpc("liststakes", "\"active\"")
        );

    ObserveSafeMode();

    if (!pStakingDb)
        throw JSONRPCError(RPC_DATABASE_ERROR, "Staking index not available");

    int filterStatus = -1; // all
    if (!request.params[0].isNull()) {
        std::string statusStr = request.params[0].get_str();
        if (statusStr == "active")
            filterStatus = STAKE_ACTIVE;
        else if (statusStr == "unlocked")
            filterStatus = STAKE_UNLOCKED;
        else if (statusStr != "all")
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid status filter. Use \"active\", \"unlocked\", or \"all\"");
    }

    std::vector<CStakeEntry> entries;
    if (!pStakingDb->GetAllStakes(entries, filterStatus))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to read staking index");

    UniValue result(UniValue::VARR);
    for (const auto& entry : entries) {
        result.push_back(StakeEntryToJSON(entry));
    }

    return result;
}

UniValue getstakesatheight(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getstakesatheight height\n"
            "\nGet stakes created at a specific block height.\n"
            "\nArguments:\n"
            "1. height   (numeric, required) The block height to query\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\":           (string) The staking transaction id\n"
            "    \"vout\":           (numeric) The output index\n"
            "    \"amount\":         (numeric) The locked amount\n"
            "    \"create_height\":  (numeric) Block height when the stake was created\n"
            "    \"unlock_height\":  (numeric) Block height when the stake becomes spendable\n"
            "    \"lock_duration\":  (numeric) Original lock duration in blocks\n"
            "    \"staker_address\": (string) The staker's address\n"
            "    \"status\":         (string) \"active\" or \"unlocked\"\n"
            "    \"description\":    (string) NFT description text\n"
            "  }, ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getstakesatheight", "100")
            + HelpExampleRpc("getstakesatheight", "100")
        );

    ObserveSafeMode();

    if (!pStakingDb)
        throw JSONRPCError(RPC_DATABASE_ERROR, "Staking index not available");

    int height = request.params[0].get_int();

    std::vector<CStakeEntry> entries;
    if (!pStakingDb->GetStakesCreatedAtHeight(height, entries))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to read staking index");

    UniValue result(UniValue::VARR);
    for (const auto& entry : entries) {
        result.push_back(StakeEntryToJSON(entry));
    }

    return result;
}

UniValue getunlocksatheight(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getunlocksatheight height\n"
            "\nGet stakes that unlock at a specific block height.\n"
            "\nUses the height index for efficient lookup.\n"
            "\nArguments:\n"
            "1. height   (numeric, required) The unlock block height to query\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\":           (string) The staking transaction id\n"
            "    \"vout\":           (numeric) The output index\n"
            "    \"amount\":         (numeric) The locked amount\n"
            "    \"create_height\":  (numeric) Block height when the stake was created\n"
            "    \"unlock_height\":  (numeric) Block height when the stake becomes spendable\n"
            "    \"lock_duration\":  (numeric) Original lock duration in blocks\n"
            "    \"staker_address\": (string) The staker's address\n"
            "    \"status\":         (string) \"active\" or \"unlocked\"\n"
            "  }, ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getunlocksatheight", "200")
            + HelpExampleRpc("getunlocksatheight", "200")
        );

    ObserveSafeMode();

    if (!pStakingDb)
        throw JSONRPCError(RPC_DATABASE_ERROR, "Staking index not available");

    int height = request.params[0].get_int();

    std::vector<CStakeEntry> entries;
    if (!pStakingDb->GetStakesUnlockingAtHeight(height, entries))
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to read staking index");

    UniValue result(UniValue::VARR);
    for (const auto& entry : entries) {
        result.push_back(StakeEntryToJSON(entry));
    }

    return result;
}

UniValue getstakeinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getstakeinfo \"txid\"\n"
            "\nGet detailed information about a specific staking transaction.\n"
            "\nArguments:\n"
            "1. \"txid\"   (string, required) The staking transaction id\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\":             (string) The staking transaction id\n"
            "  \"vout\":             (numeric) The output index\n"
            "  \"amount\":           (numeric) The locked amount\n"
            "  \"create_height\":    (numeric) Block height when the stake was created\n"
            "  \"unlock_height\":    (numeric) Block height when the stake becomes spendable\n"
            "  \"lock_duration\":    (numeric) Original lock duration in blocks\n"
            "  \"staker_address\":   (string) The staker's address\n"
            "  \"status\":           (string) \"active\" or \"unlocked\"\n"
            "  \"description\":      (string) NFT description text\n"
            "  \"reward_txid\":      (string) Reward transaction id (if rewarded)\n"
            "  \"blocks_remaining\": (numeric) Blocks until unlock (0 if already unlocked)\n"
            "  \"confirmations\":    (numeric) Number of confirmations\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getstakeinfo", "\"txid\"")
            + HelpExampleRpc("getstakeinfo", "\"txid\"")
        );

    ObserveSafeMode();

    if (!pStakingDb)
        throw JSONRPCError(RPC_DATABASE_ERROR, "Staking index not available");

    uint256 txid = ParseHashV(request.params[0], "txid");

    CStakeEntry entry;
    if (!pStakingDb->ReadStake(txid, entry))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Staking transaction not found in index");

    int current_height = chainActive.Height();
    int blocks_remaining = std::max(0, entry.unlock_height - current_height);
    int confirmations = (entry.create_height <= current_height) ? (current_height - entry.create_height + 1) : 0;

    UniValue result = StakeEntryToJSON(entry);
    result.push_back(Pair("blocks_remaining", blocks_remaining));
    result.push_back(Pair("confirmations", confirmations));
    return result;
}

static const CRPCCommand commands[] =
{   //  category    name                  actor (function)     argNames
    //  ----------- --------------------- -------------------- ----------
#ifdef ENABLE_WALLET
    { "staking",    "stakecreate",        &stakecreate,        {"amount", "lock_blocks", "description"} },
    { "staking",    "stakereward",        &stakereward,        {"stake_txid", "amount"} },
#endif
    { "staking",    "liststakes",         &liststakes,         {"status"} },
    { "staking",    "getstakeinfo",       &getstakeinfo,       {"txid"} },
    { "staking",    "getstakesatheight",  &getstakesatheight,  {"height"} },
    { "staking",    "getunlocksatheight", &getunlocksatheight, {"height"} },
};

void RegisterStakingRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
