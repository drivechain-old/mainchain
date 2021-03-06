// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/sidechain.h"

#include "clientversion.h"
#include "core_io.h"
#include "hash.h"
#include "streams.h"

#include <sstream>

const uint32_t nType = 1;
const uint32_t nVersion = 1;

uint256 sidechainObj::GetHash(void) const
{
    uint256 ret;
    if (sidechainop == 'S')
        ret = SerializeHash(*(sidechainSidechain *) this);
    else
    if (sidechainop == 'W')
        ret = SerializeHash(*(sidechainWithdraw *) this);
    else
    if (sidechainop == 'D')
        ret = SerializeHash(*(sidechainDeposit *) this);
    else
    if (sidechainop == 'V')
        ret = SerializeHash(*(sidechainVerify *) this);

    return ret;
}

CScript sidechainObj::GetScript(void) const
{
    CDataStream ds (SER_DISK, CLIENT_VERSION);

    if (sidechainop == 'S')
        ((sidechainSidechain *) this)->Serialize(ds, nType, nVersion);
    else
    if (sidechainop == 'W')
        ((sidechainWithdraw *) this)->Serialize(ds, nType, nVersion);
    else
    if (sidechainop == 'D')
        ((sidechainDeposit *) this)->Serialize(ds, nType, nVersion);
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
    const char *vch0 = (const char *) &vch.begin()[0];
    CDataStream ds(vch0, vch0+vch.size(), SER_DISK, CLIENT_VERSION);

    if (*vch0 == 'S') {
        sidechainSidechain *obj = new sidechainSidechain;
        obj->Unserialize(ds, nType, nVersion);
        return obj;
    }
    else
    if (*vch0 == 'W') {
        sidechainWithdraw *obj = new sidechainWithdraw;
        obj->Unserialize(ds, nType, nVersion);
        return obj;
    }
    else
    if (*vch0 == 'D') {
        sidechainDeposit *obj = new sidechainDeposit;
        obj->Unserialize(ds, nType, nVersion);
        return obj;
    }
    else
    if (*vch0 == 'V') {
        sidechainVerify *obj = new sidechainVerify;
        obj->Unserialize(ds, nType, nVersion);
        return obj;
    }

    return NULL;
}

// TODO remove function (use casting & sidechainObjCreator)
sidechainWithdraw *GetWT(const CScript &script)
{
    CScript::const_iterator pc = script.begin();
    vector<unsigned char> vch;
    opcodetype opcode;

    if (!script.GetOp(pc, opcode, vch))
        return NULL;
    if (vch.size() == 0)
        return NULL;
    const char *vch0 = (const char *) &vch.begin()[0];
    CDataStream ds(vch0, vch0+vch.size(), SER_DISK, CLIENT_VERSION);

    if (*vch0 == 'W') {
        sidechainWithdraw *obj = new sidechainWithdraw;
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

string sidechainSidechain::ToString() const
{
    stringstream str;
    str << "sidechainop=" << sidechainop << endl;
    str << "nHeight=" << nHeight << endl;
    str << "txid=" << txid.GetHex() << endl;
    str << "waitPeriod=" << waitPeriod << endl;
    str << "verificationPeriod=" << verificationPeriod << endl;
    str << "minWorkScore=" << minWorkScore << endl;
    str << "depositScript=" << HexStr(depositScript) << endl;
    return str.str();
}

string sidechainWithdraw::ToString() const
{
    stringstream str;
    str << "sidechainop=" << sidechainop << endl;
    str << "nHeight=" << nHeight << endl;
    str << "txid=" << txid.GetHex() << endl;
    return str.str();
}

string sidechainDeposit::ToString() const
{
    stringstream str;
    str << "sidechainop=" << sidechainop << endl;
    str << "nHeight=" << nHeight << endl;
    str << "txid=" << txid.GetHex() << endl;
    str << "sidechainid=" << sidechainid.GetHex() << endl;
    str << "dt=" << dt.ToString() << endl;
    str << "keyID=" << keyID.GetHex() << endl;
    return str.str();
}

string sidechainVerify::ToString() const
{
    stringstream str;
    str << "sidechainop=" << sidechainop << endl;
    str << "nHeight=" << nHeight << endl;
    str << "wtxid=" << wtxid.GetHex() << endl;
    str << "workScore=" << workScore << endl;
    return str.str();
}
