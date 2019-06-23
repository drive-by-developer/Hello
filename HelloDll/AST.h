#pragma once
// Copyright (c) Shaun O'Kane, 2010, 2018

#include <map>
#include <algorithm>
#include "script/script.h"
#include "Internals.h"

using namespace std;  // naughty.

struct unary_op : public AST_node
{
	shared_ptr<AST_node> a;
	unary_op(string op, AST_node_ptr a) {}
	bool generate(class Hello_compiler& compiler, istream& f) override { return true; };
};

struct binary_op : public AST_node
{
	string op;
	shared_ptr<AST_node> a, b;
	binary_op(string _op, AST_node_ptr _a, AST_node_ptr _b);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

struct assign_op : public AST_node
{
	string variable_name;
	shared_ptr<AST_node> expr;
	streampoint streampos1, streampos2;

	assign_op(string variable_name, AST_node_ptr v, const streampoint& streampos1, const streampoint& streampos2);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

struct if_then_else : public AST_node
{
	shared_ptr<AST_node> cond, a, b;
	streampoint streampos1, streampos2;

	if_then_else(AST_node_ptr cond, AST_node_ptr a, AST_node_ptr b, const streampoint& p1, const streampoint& p2);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

struct for_loop : public AST_node
{
	string loop_variable_name;
	int first_val, last_val; // lower and upper bounds.
	shared_ptr<AST_node> block;
	streampoint streampos1, streampos2;

	for_loop(string _variable_name, int first_val, int last_val, AST_node_ptr block, const streampoint& streampos1, const streampoint& streampos2);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

struct native_function : public AST_node
{
	string function_name; opcodetype opcode;
	shared_ptr<AST_node> args;
	native_function(const string& function_name, opcodetype opcode);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

struct assertion : public AST_node
{
	shared_ptr<AST_node> cond;
	streampoint streampos1, streampos2;
	assertion(AST_node_ptr cond, const streampoint& p1, const streampoint& p2);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

struct rvalue_node : public AST_node
{
	bool is_extern;
	string variable_name;
	rvalue_node(string variable_name, bool is_extern = false);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

struct const_node : public AST_node  // a bignum constant 
{
	string value;
	value_type type;
	const_node(string value, value_type type);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

// Needed to unroll for loops.
struct sequence : public AST_node
{
	AST_node_ptr a, b;
	sequence() {}
	sequence(AST_node_ptr a, AST_node_ptr b);
	void append(AST_node_ptr s);
	bool generate(class Hello_compiler& compiler, istream& f) override;
};

