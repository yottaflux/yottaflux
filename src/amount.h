// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef YOTTAFLUX_AMOUNT_H
#define YOTTAFLUX_AMOUNT_H

#include <stdint.h>

/** Amount in corbies (Can be negative) */
typedef int64_t CAmount;

static const CAmount COIN = 100000000;
static const CAmount CENT = 1000000;

/** No amount larger than this (in corbies) is valid.
 *
 * The total money supply from the Yottaflux halving schedule is approximately
 * 1,160,550,000 YAI. This constant is set slightly above that as a sanity check.
 * As this sanity check is used by consensus-critical validation code, the exact
 * value of the MAX_MONEY constant is consensus critical; in unusual circumstances
 * like an overflow bug that allowed for the creation of coins out of thin air,
 * modification could lead to a fork.
 * */
static const CAmount MAX_MONEY = 1200000000 * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

#endif //  YOTTAFLUX_AMOUNT_H
