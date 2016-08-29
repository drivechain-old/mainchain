// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include "primitives/block.h"
#include "primitives/sidechain.h"

#include <stdint.h>

class CBlockIndex;
class CChainParams;
class CReserveKey;
class CScript;
class CWallet;
namespace Consensus { struct Params; };

static const bool DEFAULT_PRINTPRIORITY = false;

struct CBlockTemplate
{
    CBlock block;
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOps;
};

/** Check validity of WT^ (not the workscore, just format) */
bool CheckWithdraw(sidechainWithdraw withdraw, sidechainSidechain sidechain);
/** Get sidechain WT^ payout transaction if there is one */
CTransaction GetSidechainWTJoin(sidechainSidechain sidechain, uint32_t height);
/** Increment / decrement WT^ workscore */
CTransaction GetSidechainScoreTX(sidechainSidechain sidechain, uint32_t height);
/** Calculate nHeight of the last tau (end of period) */
uint32_t GetLastTauHeight(sidechainSidechain sidechain, uint32_t height);
/** Generate a new block, without valid proof-of-work */
CBlockTemplate* CreateNewBlock(const CChainParams& chainparams, const CScript& scriptPubKeyIn);
/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

#endif // BITCOIN_MINER_H
