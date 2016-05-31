// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <sstream>
#include "hash.h"
#include "clientversion.h"
#include "primitives/sidechain.h"
#include "streams.h"

const uint32_t nType = 1;
const uint32_t nVersion = 1;

uint256 sidechainObj::GetHash(void) const
{
    uint256 ret;
    if (sidechainop == 'A')
        ret = SerializeHash(*(sidechainAdd *) this);
    else
    if (sidechainop == 'W')
        ret = SerializeHash(*(sidechainWithdraw *) this);
    else
    if (sidechainop == 'V')
        ret = SerializeHash(*(sidechainVerify *) this);

    return ret;
}

CScript sidechainObj::GetScript(void) const
{
    CDataStream ds (SER_DISK, CLIENT_VERSION);

    if (sidechainop == 'A')
        ((sidechainAdd *) this)->Serialize(ds, nType, nVersion);
    else
    if (sidechainop == 'W')
        ((sidechainWithdraw *) this)->Serialize(ds, nType, nVersion);
    else
    if (sidechainop == 'V')
        ((sidechainVerify *) this)->Serialize(ds, nType, nVersion);

    CScript script;
    script << vector<unsigned char>(ds.begin(), ds.end()) << OP_SIDECHAIN;
    return script;
}

sidechainObj *sidechainObjCtr(const CScript &script)
{
    CScript::const_iterator pc = script.begin();
    vector<unsigned char> vch;
    opcodetype opcode;

    if (!script.GetOp(pc, opcode, vch))
        return NULL;
    if (vch.size() == 0)
        return NULL;
    const char *vchOP = (const char *) &vch.begin()[0];
    CDataStream ds(vchOP, vchOP+vch.size(), SER_DISK, CLIENT_VERSION);

    if (*vchOP == 'A') {
        sidechainAdd *obj = new sidechainAdd;
        obj->Unserialize(ds, nType, nVersion);
        return obj;
    }
    else
    if (*vchOP == 'W') {
        sidechainWithdraw *obj = new sidechainWithdraw;
        obj->Unserialize(ds, nType, nVersion);
        return obj;
    }
    else
    if (*vchOP == 'V') {
        sidechainVerify *obj = new sidechainVerify;
        obj->Unserialize(ds, nType, nVersion);
        return obj;
    }

    return NULL;
}

string sidechainObj::ToString(void) const
{
    stringstream str;
    str << "sidechainop=" << sidechainop << endl;
    str << "nHeight=" << nHeight << endl;
    str << "txid=" << txid.GetHex() << endl;
    return str.str();
}

string sidechainAdd::ToString() const
{
    stringstream str;
    str << "sidechainop=" << sidechainop << endl;
    str << "nHeight=" << nHeight << endl;
    str << "txid=" << txid.GetHex() << endl;
    return str.str();
}

string sidechainWithdraw::ToString() const
{
    stringstream str;
    str << "sidechainop=" << sidechainop << endl;
    str << "nHeight=" << nHeight << endl;
    str << "txid=" << txid.GetHex() << endl;
    str << "proposaltxid=" << txid.GetHex() << endl;
    return str.str();
}

string sidechainVerify::ToString() const
{
    stringstream str;
    str << "sidechainop=" << sidechainop << endl;
    str << "nHeight=" << nHeight << endl;
    str << "txid=" << txid.GetHex() << endl;
    str << "verify=" << verify << endl;

    return str.str();
}
