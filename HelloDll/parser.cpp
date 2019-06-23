// Copyright (c) Shaun O'Kane, 2010, 2018
#include "tokeniser.h"
#include "parser.h"

///////////////////////////////////////////////////////////////////////////////
//
// function_info tables
//
///////////////////////////////////////////////////////////////////////////////

// These are void functions.
const function_info_table_type special_functions =
{
	{"Verify", {1, OP_VERIFY}},
	{"CheckSequenceVerify", {1, OP_CHECKSEQUENCEVERIFY}},
	{"CheckLocktimeVerify", {1, OP_CHECKLOCKTIMEVERIFY}},
	{"CheckSigVerify", {2, OP_CHECKSIGVERIFY}},
	{"CheckMultiSigVerify", {-1, OP_CHECKMULTISIGVERIFY}},
	{"Return", {1, OP_RETURN}}
};

// These return a single value (except split - which returns 2 values. The other value is accessable as tos).
// Treatment of split not good. TODO: revise.
const function_info_table_type native_functions =
{
	// Crypto
	{"RIPEMD160", {1, OP_RIPEMD160}},
	{"SHA1", {1, OP_SHA1}},
	{"SHA256", {1, OP_SHA256}},
	{"HASH160", {1, OP_HASH160}},
	{"HASH256", {1, OP_HASH256}},
	{"CheckSig", {2, OP_CHECKSIG}},
	{"CheckMultiSig", {-1, OP_CHECKMULTISIG}},
	// byte array
	{"split", {2, OP_SPLIT}},
	{"cat", {2, OP_CAT}},
	{"xor", {2, OP_XOR}},
	{"Bin2Num", {1, OP_BIN2NUM}},
	{"Num2Bin", {1, OP_NUM2BIN}},
	// numeric
	{"abs", {1, OP_ABS}},
	{"max", {2, OP_MAX}},
	{"min", {2, OP_MIN}},
	{"within", {3, OP_WITHIN}},
	// misc
	{"depth", {0, OP_DEPTH}},
	{"size", {1, OP_SIZE}},
};
