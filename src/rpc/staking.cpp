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

#ifdef ENABLE_WALLET
UniValue stakecreate(const JSONRPCRequest& request)
{
    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "stakecreate amount lock_blocks\n"
            "\nCreate a staking transaction that locks coins for a specified number of blocks using CLTV.\n"
            "\nArguments:\n"
            "1. amount       (numeric, required) The amount in " + CURRENCY_UNIT + " to lock for staking.\n"
            "2. lock_blocks  (numeric, required) The number of blocks to lock the coins for.\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\":           (string) The staking transaction id\n"
            "  \"p2sh_address\":   (string) The P2SH address holding the locked funds\n"
            "  \"unlock_height\":  (numeric) The block height at which funds become spendable\n"
            "  \"lock_blocks\":    (numeric) The number of blocks the coins are locked for\n"
            "  \"amount\":         (numeric) The amount locked\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("stakecreate", "100 50")
            + HelpExampleRpc("stakecreate", "100, 50")
        );

    ObserveSafeMode();
    LOCK2(cs_main, pwallet->cs_wallet);

    // Parse parameters
    CAmount nAmount = AmountFromValue(request.params[0]);
    if (nAmount <= 0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for staking");

    int lock_blocks = request.params[1].get_int();
    if (lock_blocks <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "lock_blocks must be a positive integer");

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

    // Build OP_RETURN marker
    CScript opReturnScript = BuildStakeMarkerScript(
        YFX_STAKE_VERSION,
        (uint32_t)unlock_height,
        (uint32_t)lock_blocks,
        keyID
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
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair("txid", entry.txid.GetHex()));
        obj.push_back(Pair("vout", (int)entry.vout));
        obj.push_back(Pair("amount", ValueFromAmount(entry.amount)));
        obj.push_back(Pair("create_height", entry.create_height));
        obj.push_back(Pair("unlock_height", entry.unlock_height));
        obj.push_back(Pair("lock_duration", entry.lock_duration));
        obj.push_back(Pair("staker_address", entry.staker_address));
        obj.push_back(Pair("status", entry.status == STAKE_ACTIVE ? "active" : "unlocked"));
        result.push_back(obj);
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

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("txid", entry.txid.GetHex()));
    result.push_back(Pair("vout", (int)entry.vout));
    result.push_back(Pair("amount", ValueFromAmount(entry.amount)));
    result.push_back(Pair("create_height", entry.create_height));
    result.push_back(Pair("unlock_height", entry.unlock_height));
    result.push_back(Pair("lock_duration", entry.lock_duration));
    result.push_back(Pair("staker_address", entry.staker_address));
    result.push_back(Pair("status", entry.status == STAKE_ACTIVE ? "active" : "unlocked"));
    result.push_back(Pair("blocks_remaining", blocks_remaining));
    result.push_back(Pair("confirmations", confirmations));
    return result;
}

static const CRPCCommand commands[] =
{   //  category    name                actor (function)     argNames
    //  ----------- ------------------- -------------------- ----------
#ifdef ENABLE_WALLET
    { "staking",    "stakecreate",      &stakecreate,        {"amount", "lock_blocks"} },
#endif
    { "staking",    "liststakes",       &liststakes,         {"status"} },
    { "staking",    "getstakeinfo",     &getstakeinfo,       {"txid"} },
};

void RegisterStakingRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
