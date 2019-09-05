// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    int nHeight = pindexLast->nHeight + 1;

    // initial window using the fixed value
    if (nHeight <= params.nZawyLwmaAveragingWindow) {
        // PoW limit for initial period.
        unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
        return nProofOfWorkLimit;
    }

    return LwmaCalculateNextWorkRequired(pindexLast, params);
}

// LWMA-1 for BTC/Zcash clones
// LWMA has the best response*stability.  It rises slowly & drops fast when needed.  
// Copyright (c) 2017-2018 The Bitcoin Gold developers
// Copyright (c) 2018-2019 Zawy 
// Copyright (c) 2018 iamstenman (Microbitcoin)
// MIT License
// Algorithm by Zawy, a modification of WT-144 by Tom Harding
// For any changes, patches, updates, etc see
// https://github.com/zawy12/difficulty-algorithms/issues/3#issuecomment-442129791
//  FTL should be lowered to about N*T/20.
//  FTL in BTC clones is MAX_FUTURE_BLOCK_TIME in chain.h.
//  FTL in Ignition, Numus, and others can be found in main.h as DRIFT.
//  FTL in Zcash & Dash clones need to change the 2*60*60 here:
//  if (block.GetBlockTime() > nAdjustedTime + 2 * 60 * 60)
//  which is around line 3700 in main.cpp in ZEC and validation.cpp in Dash
//  If your coin uses network time instead of node local time, lowering FTL < about 125% of the
//  "revert to node time" rule (70 minutes in BCH, ZEC, & BTC) allows 33% Sybil attack,
//  so revert rule must be ~ FTL/2 instead of 70 minutes. See:
// https://github.com/zcash/zcash/issues/4021

unsigned int Lwma3CalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const int64_t T = params.nPowTargetSpacing;

   // For T=600, 300, 150 use approximately N=60, 90, 120
    const int64_t N = params.nZawyLwmaAveragingWindow;  

    // Define a k that will be used to get a proper average after weighting the solvetimes.
    const int64_t k = N * (N + 1) * T / 2; 

    const int64_t height = pindexLast->nHeight;
    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    
   // New coins should just give away first N blocks before using this algorithm.
    if (height < N) { return powLimit.GetCompact(); }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);

        // Prevent solvetimes from being negative in a safe way. It must be done like this. 
        // In particular, do not attempt anything like  if(solvetime < 0) {solvetime=0;}
        // The +1 ensures new coins do not calculate nextTarget = 0.
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

       // A 6*T limit will prevent large drops in difficulty from long solvetimes.
//        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);
        int64_t solvetime = thisTimestamp - previousTimestamp;

       // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;
       
       // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.
    }
   // Desired equation in next line was nextTarget = avgTarget * sumWeightSolvetimes / k
   // but 1/k was moved to line above to prevent overflow in new coins

    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powLimit) { nextTarget = powLimit; }

    return nextTarget.GetCompact();
}


unsigned int LwmaCalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting) {
        return pindexLast->nBits;
    }

    const int height = pindexLast->nHeight + 1;
    const int64_t T = params.nPowTargetSpacing;  // 2x60
    const int N = params.nZawyLwmaAveragingWindow; // 45 
    const int k = params.ZawyLwmaAdjustedWeight(height); // 13722 
    const int dnorm = params.ZawyLwmaMinDenominator(height); // 10
    const bool limit_st = params.bZawyLwmaSolvetimeLimitation;
    assert(height > N);

    arith_uint256 sum_target;
    int t = 0, j = 0;

    // Loop through N most recent blocks.
    for (int i = height - N; i < height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);
        const CBlockIndex* block_Prev = block->GetAncestor(i - 1);
        int64_t solvetime = block->GetBlockTime() - block_Prev->GetBlockTime();

        if (limit_st && solvetime > 6 * T) {
            solvetime = 6 * T;
        }

        j++;
        t += solvetime * j;  // Weighted solvetime sum.

        // Target sum divided by a factor, (k N^2).
        // The factor is a part of the final equation. However we divide sum_target here to avoid
        // potential overflow.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sum_target += target / (k * N * N);
    }
    // Keep t reasonable in case strange solvetimes occurred.
    if (t < N * k / dnorm) {
        t = N * k / dnorm;
    }

    const arith_uint256 pow_limit = UintToArith256(params.powLimit);
    arith_uint256 next_target = t * sum_target;
    if (next_target > pow_limit) {
        next_target = pow_limit;
    }

    return next_target.GetCompact();
}


unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    // temp increases adjustment steps to 50% and 200%
    /*******
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan * 83 / 100) {
        nActualTimespan = params.nPowTargetTimespan  * 83 / 100;
    }
    if (nActualTimespan > params.nPowTargetTimespan * 132 / 100) {
        nActualTimespan = params.nPowTargetTimespan * 132 / 100;
    }
    *******/
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan * 50 / 100) {
        nActualTimespan = params.nPowTargetTimespan  * 50 / 100;
    }
    if (nActualTimespan > params.nPowTargetTimespan * 200 / 100) {
        nActualTimespan = params.nPowTargetTimespan * 200 / 100;
    }

    
    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
