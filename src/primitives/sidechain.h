// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_SIDECHAIN_H
#define BITCOIN_PRIMITIVES_SIDECHAIN_H

#include "primitives/transaction.h"
#include "pubkey.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"
#include "utilstrencodings.h"

#include <limits.h>
#include <string>
#include <vector>

using namespace std;

// TODO miner checks list before accepting createSidechain TX
// TODO change valid sidechain list to pairs of (uint256 sidechainid, str name)
/**
 * List of sidechains considered valid
 */
const uint256 sidechains[] = {
    // Test Sidechain
    uint256S("0xca85db47c45dfccfa9f5562f7383c7b3fe1746017327371771ed3f70345b72d4")
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
    CScript depositScript;

    sidechainSidechain(void) : sidechainObj() { sidechainop = 'S'; }
    virtual ~sidechainSidechain(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
        READWRITE(waitPeriod);
        READWRITE(verificationPeriod);
        READWRITE(minWorkScore);
        READWRITE(*(CScriptBase*)(&depositScript));
    }

    string ToString(void) const;
};

/**
 * Sidechain withdraw proposal added to database
 */
struct sidechainWithdraw : public sidechainObj {
    uint256 sidechainid;
    CTransaction wt;

    sidechainWithdraw(void) : sidechainObj() { sidechainop = 'W'; }
    virtual ~sidechainWithdraw(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
        READWRITE(sidechainid);
        READWRITE(wt);
    }

    string ToString(void) const;
};

/**
 * Sidechain deposit added to database
 */
struct sidechainDeposit : public sidechainObj {
    uint256 sidechainid;
    CTransaction dt;
    CKeyID keyID;

    sidechainDeposit(void) : sidechainObj() { sidechainop = 'D'; }
    virtual ~sidechainDeposit(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
        READWRITE(sidechainid);
        READWRITE(dt);
        READWRITE(keyID);
    }

    string ToString(void) const;
};

// TODO

/**
 * Sidechain proposal verification
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
