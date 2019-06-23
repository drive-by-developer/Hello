// Copyright (c) 2015-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"

#include "crypto/sha256.h"
#include "key.h"
#include "random.h"
#include "util.h"
#include "validation.h"

int main(int argc, char **argv) {
    SHA256AutoDetect();
    RandomInit();
    ECC_Start();
    SetupEnvironment();

    // don't want to write to debug.log file
    GetLogger().fPrintToDebugLog = false;

    benchmark::BenchRunner::RunAll();

    ECC_Stop();
}
