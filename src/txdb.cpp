// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "chainparams.h"
#include "hash.h"
#include "pow.h"
#include "uint256.h"

#include <stdint.h>

#include <boost/thread.hpp>

#include "dbwrapper.h"

using namespace std;

static const char DB_COINS = 'c';
static const char DB_BLOCK_FILES = 'f';
static const char DB_TXINDEX = 't';
static const char DB_BLOCK_INDEX = 'b';

static const char DB_BEST_BLOCK = 'B';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_LAST_BLOCK = 'l';


CCoinsViewDB::CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe, true)
{
}

bool CCoinsViewDB::GetCoins(const uint256 &txid, CCoins &coins) const {
    return db.Read(make_pair(DB_COINS, txid), coins);
}

bool CCoinsViewDB::HaveCoins(const uint256 &txid) const {
    return db.Exists(make_pair(DB_COINS, txid));
}

uint256 CCoinsViewDB::GetBestBlock() const {
    uint256 hashBestChain;
    if (!db.Read(DB_BEST_BLOCK, hashBestChain))
        return uint256();
    return hashBestChain;
}

bool CCoinsViewDB::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) {
    CDBBatch batch(db);
    size_t count = 0;
    size_t changed = 0;
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) {
            if (it->second.coins.IsPruned())
                batch.Erase(make_pair(DB_COINS, it->first));
            else
                batch.Write(make_pair(DB_COINS, it->first), it->second.coins);
            changed++;
        }
        count++;
        CCoinsMap::iterator itOld = it++;
        mapCoins.erase(itOld);
    }
    if (!hashBlock.IsNull())
        batch.Write(DB_BEST_BLOCK, hashBlock);

    LogPrint("coindb", "Committing %u changed transactions (out of %u) to coin database...\n", (unsigned int)changed, (unsigned int)count);
    return db.WriteBatch(batch);
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe) {
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return Write(DB_REINDEX_FLAG, '1');
    else
        return Erase(DB_REINDEX_FLAG);
}

bool CBlockTreeDB::ReadReindexing(bool &fReindexing) {
    fReindexing = Exists(DB_REINDEX_FLAG);
    return true;
}

bool CBlockTreeDB::ReadLastBlockFile(int &nFile) {
    return Read(DB_LAST_BLOCK, nFile);
}

CCoinsViewCursor *CCoinsViewDB::Cursor() const
{
    CCoinsViewDBCursor *i = new CCoinsViewDBCursor(const_cast<CDBWrapper*>(&db)->NewIterator(), GetBestBlock());
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    i->pcursor->Seek(DB_COINS);
    // Cache key of first record
    i->pcursor->GetKey(i->keyTmp);
    return i;
}

bool CCoinsViewDBCursor::GetKey(uint256 &key) const
{
    // Return cached key
    if (keyTmp.first == DB_COINS) {
        key = keyTmp.second;
        return true;
    }
    return false;
}

bool CCoinsViewDBCursor::GetValue(CCoins &coins) const
{
    return pcursor->GetValue(coins);
}

unsigned int CCoinsViewDBCursor::GetValueSize() const
{
    return pcursor->GetValueSize();
}

bool CCoinsViewDBCursor::Valid() const
{
    return keyTmp.first == DB_COINS;
}

void CCoinsViewDBCursor::Next()
{
    pcursor->Next();
    if (!pcursor->Valid() || !pcursor->GetKey(keyTmp))
        keyTmp.first = 0; // Invalidate cached key after last record so that Valid() and GetKey() return false
}

bool CBlockTreeDB::WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<int, const CBlockFileInfo*> >::const_iterator it=fileInfo.begin(); it != fileInfo.end(); it++) {
        batch.Write(make_pair(DB_BLOCK_FILES, it->first), *it->second);
    }
    batch.Write(DB_LAST_BLOCK, nLastFile);
    for (std::vector<const CBlockIndex*>::const_iterator it=blockinfo.begin(); it != blockinfo.end(); it++) {
        batch.Write(make_pair(DB_BLOCK_INDEX, (*it)->GetBlockHash()), CDiskBlockIndex(*it));
    }
    return WriteBatch(batch, true);
}

bool CBlockTreeDB::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return Read(make_pair(DB_TXINDEX, txid), pos);
}

bool CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256,CDiskTxPos> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(make_pair(DB_TXINDEX, it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::WriteFlag(const std::string &name, bool fValue) {
    return Write(std::make_pair(DB_FLAG, name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const std::string &name, bool &fValue) {
    char ch;
    if (!Read(std::make_pair(DB_FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CBlockTreeDB::LoadBlockIndexGuts(boost::function<CBlockIndex*(const uint256&)> insertBlockIndex)
{
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_BLOCK_INDEX, uint256()));

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_BLOCK_INDEX) {
            CDiskBlockIndex diskindex;
            if (pcursor->GetValue(diskindex)) {
                // Construct block index object
                CBlockIndex* pindexNew = insertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev          = insertBlockIndex(diskindex.hashPrev);
                pindexNew->nHeight        = diskindex.nHeight;
                pindexNew->nFile          = diskindex.nFile;
                pindexNew->nDataPos       = diskindex.nDataPos;
                pindexNew->nUndoPos       = diskindex.nUndoPos;
                pindexNew->nVersion       = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->nTime          = diskindex.nTime;
                pindexNew->nBits          = diskindex.nBits;
                pindexNew->nNonce         = diskindex.nNonce;
                pindexNew->nStatus        = diskindex.nStatus;
                pindexNew->nTx            = diskindex.nTx;

                if (!CheckProofOfWork(pindexNew->GetBlockHash(), pindexNew->nBits, Params().GetConsensus()))
                    return error("LoadBlockIndex(): CheckProofOfWork failed: %s", pindexNew->ToString());

                pcursor->Next();
            } else {
                return error("LoadBlockIndex() : failed to read value");
            }
        } else {
            break;
        }
    }

    return true;
}

CSidechainTreeDB::CSidechainTreeDB(size_t nCacheSize, bool fMemory, bool fWipe)
    : CDBWrapper(GetDataDir() / "blocks" / "sidechain", nCacheSize, fMemory, fWipe) {
}

bool CSidechainTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &fileinfo) {
    return Read(make_pair(DB_BLOCK_FILES, nFile), fileinfo);
}

bool CSidechainTreeDB::WriteReindexing(bool fReindex) {
    if (fReindex)
        return Write(DB_REINDEX_FLAG, '1');
    else
        return Erase(DB_REINDEX_FLAG);
}

bool CSidechainTreeDB::ReadReindexing(bool &fReindex) {
    fReindex = Exists(DB_REINDEX_FLAG);
    return true;
}

bool CSidechainTreeDB::ReadLastBlockFile(int &nFile) {
    return Read(DB_LAST_BLOCK, nFile);
}

bool CSidechainTreeDB::WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo *> > &fileInfo, int nLastFile, const std::vector<const CBlockIndex *> &blockinfo) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<int, const CBlockFileInfo*> >::const_iterator it=fileInfo.begin(); it != fileInfo.end(); it++) {
        batch.Write(make_pair(DB_BLOCK_FILES, it->first), *it->second);
    }
    batch.Write(DB_LAST_BLOCK, nLastFile);
    for (std::vector<const CBlockIndex*>::const_iterator it=blockinfo.begin(); it != blockinfo.end(); it++) {
        batch.Write(make_pair(DB_BLOCK_INDEX, (*it)->GetBlockHash()), CDiskBlockIndex(*it));
    }
    return WriteBatch(batch, true);
}

bool CSidechainTreeDB::WriteSidechainIndex(const std::vector<std::pair<uint256, const sidechainObj *> > &list)
{
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256, const sidechainObj *> >::const_iterator it=list.begin(); it!=list.end(); it++) {
        const uint256 &objid = it->first;
        const sidechainObj *obj = it->second;
        pair<char, uint256> key = make_pair(obj->sidechainop, objid);

        if (obj->sidechainop == 'S') {
            const sidechainSidechain *ptr = (const sidechainSidechain *) obj;
            pair<sidechainSidechain, uint256> value = make_pair(*ptr, obj->txid);
            batch.Write(key, value);
        }
        else
        if (obj->sidechainop == 'W') {
            const sidechainWithdraw *ptr = (const sidechainWithdraw *) obj;
            pair<sidechainWithdraw, uint256> value = make_pair(*ptr, obj->txid);
            batch.Write(key, value);
            batch.Write(make_pair(make_pair(make_pair('w', ptr->sidechainid), ptr->nHeight), objid), value);
        }
        else
        if (obj->sidechainop == 'D') {
            const sidechainDeposit *ptr = (const sidechainDeposit *) obj;
            pair<sidechainDeposit, uint256> value = make_pair(*ptr, obj->txid);
            batch.Write(key, value);
            batch.Write(make_pair(make_pair(make_pair('d', ptr->sidechainid), ptr->nHeight), objid), value);
        }
        else
        if (obj->sidechainop == 'V') {
            const sidechainVerify *ptr = (const sidechainVerify *) obj;
            pair<sidechainVerify, uint256> value = make_pair(*ptr, obj->txid);
            batch.Write(key, value);
            batch.Write(make_pair(make_pair('v', ptr->wtxid), objid), value);
        }
    }

    return WriteBatch(batch);
}

bool CSidechainTreeDB::WriteFlag(const std::string &name, bool fValue) {
    return Write(make_pair(DB_FLAG, name), fValue ? '1' : '0');
}


bool CSidechainTreeDB::ReadFlag(const string &name, bool &fValue)
{
    char ch;
    if (!Read(make_pair(DB_FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CSidechainTreeDB::GetSidechain(const uint256 &objid, sidechainSidechain &sidechain) {
    if (ReadSidechain(make_pair('S', objid), sidechain))
        return true;

    return false;
}

bool CSidechainTreeDB::GetJoinedWT(const uint256 &objid, sidechainWithdraw &withdraw) {
    if (ReadSidechain(make_pair('W', objid), withdraw))
        return true;

    return false;
}

bool CSidechainTreeDB::GetDeposit(const uint256 &objid, sidechainDeposit &deposit) {
    if (ReadSidechain(make_pair('D', objid), deposit))
        return true;

    return false;
}

bool CSidechainTreeDB::GetVerification(const uint256 &objid, sidechainVerify &verify) {
    if (ReadSidechain(make_pair('V', objid), verify))
        return true;

    return false;
}

vector<sidechainSidechain> CSidechainTreeDB::GetSidechains(void) {
    const char sidechainop = 'S';
    ostringstream ss;
    ss << sidechainop;

    vector<sidechainSidechain> vSidechain;
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    for (pcursor->Seek(ss.str()); pcursor->Valid(); pcursor->Next()) {
        boost::this_thread::interruption_point();

        std::pair<char, uint256> key;
        sidechainSidechain sidechain;
        if (pcursor->GetKey(key) && key.first == sidechainop) {
            if (pcursor->GetSidechainValue(sidechain))
                vSidechain.push_back(sidechain);
        }
    }

    return vSidechain;
}

vector<sidechainWithdraw> CSidechainTreeDB::GetJoinedWTs(const uint256 &objid) {
    const char withdrawop = 'W';
    ostringstream ss;
    ::Serialize(ss, make_pair(make_pair(withdrawop, objid), uint256()), SER_DISK, CLIENT_VERSION);

    vector<sidechainWithdraw> vWithdraw;
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    for (pcursor->Seek(ss.str()); pcursor->Valid(); pcursor->Next()) {
        boost::this_thread::interruption_point();

        std::pair<char, uint256> key;
        sidechainWithdraw withdraw;
        if (pcursor->GetKey(key) && key.first == withdrawop) {
            if (pcursor->GetSidechainValue(withdraw))
                vWithdraw.push_back(withdraw);
        }
    }

    return vWithdraw;
}

vector<sidechainDeposit> CSidechainTreeDB::GetDeposits(const uint256 &objid, uint32_t height) {
    // TODO filter by height
    const char depositop = 'D';
    ostringstream ss;
    ::Serialize(ss, make_pair(make_pair(depositop, objid), uint256()), SER_DISK, CLIENT_VERSION);

    vector<sidechainDeposit> vDeposit;
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    for (pcursor->Seek(ss.str()); pcursor->Valid(); pcursor->Next()) {
        boost::this_thread::interruption_point();

        std::pair<char, uint256> key;
        sidechainDeposit deposit;
        if (pcursor->GetKey(key) && key.first == depositop) {
            if (pcursor->GetSidechainValue(deposit))
                vDeposit.push_back(deposit);
        }
    }

    return vDeposit;
}

vector<sidechainVerify> CSidechainTreeDB::GetVerifications(const uint256 &objid) {
    const char verifyop = 'V';
    ostringstream ss;
    ::Serialize(ss, make_pair(make_pair(verifyop, objid), uint256()), SER_DISK, CLIENT_VERSION);

    vector<sidechainVerify> vVerify;
    boost::scoped_ptr<CDBIterator> pcursor(NewIterator());
    for (pcursor->Seek(ss.str()); pcursor->Valid(); pcursor->Next()) {
        boost::this_thread::interruption_point();

        std::pair<char, uint256> key;
        sidechainVerify verify;
        if (pcursor->GetKey(key) && key.first == verifyop) {
            if (pcursor->GetSidechainValue(verify))
                vVerify.push_back(verify);
        }
    }

    return vVerify;
}
