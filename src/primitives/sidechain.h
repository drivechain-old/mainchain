// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_SIDECHAIN_H
#define BITCOIN_PRIMITIVES_SIDECHAIN_H

#include <limits.h>
#include <string>
#include <vector>
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

using namespace std;

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
 * Create sidechain object
 */
sidechainObj *sidechainObjCtr(const CScript &);

/**
 * Sidechain added to database
 */
struct sidechainSidechain : public sidechainObj {
    uint16_t waitPeriod;
    uint16_t verificationPeriod;
    uint16_t minWorkScore;

    sidechainSidechain(void) : sidechainObj() { sidechainop = 'S'; }
    virtual ~sidechainSidechain(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
        READWRITE(waitPeriod);
        READWRITE(verificationPeriod);
        READWRITE(minWorkScore);
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

/**
 * Get sidechainWithdraw object
 */
sidechainWithdraw *GetWT(const CScript &script);

// TODO finish

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

bool CheckVerifications(vector<sidechainVerify> vec);

#endif // BITCOIN_PRIMITIVES_SIDECHAIN_H
