// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_SIDECHAIN_H
#define BITCOIN_PRIMITIVES_SIDECHAIN_H

#include <limits.h>
#include <string>
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
struct sidechainAdd : public sidechainObj {
    sidechainAdd(void) : sidechainObj() { sidechainop = 'A'; }
    virtual ~sidechainAdd(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
    }

    string ToString(void) const;
};

/**
 * Sidechain withdraw proposal added to database
 */
struct sidechainWithdraw : public sidechainObj {
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
 * Sidechain proposal verification (verified if verify = true, rejected otherwise)
 */
struct sidechainVerify : public sidechainObj {
    bool verify;

    sidechainVerify(void) : sidechainObj() { sidechainop = 'V'; }
    virtual ~sidechainVerify(void) { }

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(sidechainop);
        READWRITE(verify);
    }

    string ToString(void) const;
};

#endif // BITCOIN_PRIMITIVES_SIDECHAIN_H
