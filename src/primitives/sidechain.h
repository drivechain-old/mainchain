// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_SIDECHAIN_H
#define BITCOIN_PRIMITIVES_SIDECHAIN_H

#include "primitives/transaction.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"
#include "utilstrencodings.h"

#include <limits.h>
#include <string>
#include <vector>

using namespace std;

// TODO miner checks list before accepting createSidechain TX
/**
 * List of sidechains considered valid
 */
const uint256 sidechains[] = {
    // Test Sidechain
    uint256S("0x0aaaff52e6459150950173750de9493eb1157cd3ee270cd33519bed5a6d07e7a"),
    // Test Sidechain 2
    uint256S("0x8147bcc9e3268d2d42e851e73efcd872fbb3e0c649876419e86615681c7a580a")
};

const std::set<uint256> validSidechains(sidechains, sidechains + ARRAYLEN(sidechains));

/**
 * Sidechain object for database
 */
struct sidechainObj {
    char sidechainop;
    uint32_t nHeight;
    uint256 txid;

    sidechainObj(void): nHeight(INT_MAX) { }
    virtual ~sidechainObj(void) { }

    uint256 GetHash(void) const;
    CScript GetScript(void) const;
    virtual string ToString(void) const;
};

/**
 * Sidechain added to database
 */
struct sidechainSidechain : public sidechainObj {
    uint16_t waitPeriod;
    uint16_t verificationPeriod;
    uint16_t minWorkScore;
    CScript depositPubKey;

    sidechainSidechain(void) : sidechainObj() { sidechainop = 'S'; }
    virtual ~sidechainSidechain(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
        READWRITE(waitPeriod);
        READWRITE(verificationPeriod);
        READWRITE(minWorkScore);
        READWRITE(*(CScriptBase*)(&depositPubKey));
    }

    string ToString(void) const;
};

/**
 * Sidechain withdraw proposal added to database
 */
struct sidechainWithdraw : public sidechainObj {
    uint256 sidechainid;
    uint256 proposaltxid;

    sidechainWithdraw(void) : sidechainObj() { sidechainop = 'W'; }
    virtual ~sidechainWithdraw(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
        READWRITE(proposaltxid);
    }

    string ToString(void) const;
};

// TODO

/**
 * Sidechain proposal verification (verified if verify = true, rejected otherwise)
 * Two verification types:
 * (1) "full vote" with the full serialization of the sidechain / WT^
 * (2) "compressed vote" with the head of the serialization data (and score).
 */
struct sidechainVerify : public sidechainObj {
    uint32_t workScore; // To be checked by OP_CHECKWORKSCOREVERIFY
    uint256 wtxid;

    sidechainVerify(void) : sidechainObj() { sidechainop = 'V'; }
    virtual ~sidechainVerify(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
        READWRITE(workScore);
        READWRITE(wtxid);
    }

    string ToString(void) const;
};

/**
 * Create sidechain object
 */
sidechainObj *sidechainObjCtr(const CScript &);

/**
 * Get sidechainWithdraw object
 */
sidechainWithdraw *GetWT(const CScript &script);

#endif // BITCOIN_PRIMITIVES_SIDECHAIN_H
