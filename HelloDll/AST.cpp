// Copyright (c) Shaun O'Kane, 2010, 2018
#include "Hello_compiler.h"
#include <ostream>
#include <fstream>
#include "utilstrencodings.h"
#include "../Common/Internals.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
//
//  const_node
//
///////////////////////////////////////////////////////////////////////////////

const_node::const_node(string _value, value_type _type)
	: value(_value),
	type(_type)
{
}

bool const_node::generate(class Hello_compiler& compiler, istream& f) 
{
	auto& pipe = compiler.pipe;
	switch(type)
	{
	case _hex:
		pipe << ParseHex(value.c_str());
		break;
	case _integer:
		pipe << atoi(value.c_str());
		break;
	default:
		assert(false); // temp
	}
	return true; 
};

///////////////////////////////////////////////////////////////////////////////
//
//  rvalue_node
//
///////////////////////////////////////////////////////////////////////////////

rvalue_node::rvalue_node(string _variable_name, bool _is_extern) 
	: variable_name(_variable_name),
	  is_extern(_is_extern)
{
}

bool rvalue_node::generate(class Hello_compiler& compiler, istream& f)
{
	instruction_pipeline& pipe = compiler.pipe;
	if(is_extern)
		pipe << compiler.get_extern_value(variable_name);
	else if(variable_name != "tos")
		compiler.copy_to_top_of_stack(variable_name);
	else
		pipe << OP_DUP;
	return true; 
};

///////////////////////////////////////////////////////////////////////////////
//
//  assign_op
//
///////////////////////////////////////////////////////////////////////////////

assign_op::assign_op(string _variable_name, AST_node_ptr _v, const streampoint& _streampos1, const streampoint& _streampos2) 
	: variable_name(_variable_name), expr(_v), streampos1(_streampos1), streampos2(_streampos2)
{
}

// Should probably move this logic into lvalue_node...
bool assign_op::generate(class Hello_compiler& compiler, istream& f)
{
	auto& pipe = compiler.pipe;
	pipe.declare_stmt({streampos1, streampos2});

	// 1. Put the result of the RHS expression on top of the stack.
	assert(expr);
	expr->generate(compiler, f);

	// 2. Move the value into the slot in the alt-stack reserved for the variable.
	if(variable_name != "tos")
		compiler.assign_from_top_of_stack(variable_name);
	return true; 
};

///////////////////////////////////////////////////////////////////////////////
//
//  binary_op
//
///////////////////////////////////////////////////////////////////////////////

binary_op::binary_op(string _op, AST_node_ptr _a, AST_node_ptr _b) 
	: op(_op), a(_a), b(_b)
{
}

bool binary_op::generate(class Hello_compiler& compiler, istream& f)
{
	auto& pipe = compiler.pipe;
	bool is_ok = a->generate(compiler, f);
	if(is_ok) is_ok = b->generate(compiler, f);
	if(op == "+")
	{
		pipe << OP_ADD;
	}
	else if(op == "-")
	{
		pipe << OP_SUB;
	}
	else if(op == "*")
	{
		pipe << OP_MUL;
	}
	else if(op == "/")
	{
		pipe << OP_DIV;
	}
	else if(op == "%")
	{
		pipe << OP_MOD;
	}
	else if(op == "<")
	{
		pipe << OP_LESSTHAN;
	}
	else if(op == "<=")
	{
		pipe << OP_LESSTHANOREQUAL;
	}
	else if(op == "==")
	{
		pipe << OP_EQUAL;
	}
	else if(op == "!=")
	{
		pipe << OP_0NOTEQUAL;
	}
	else if(op == ">=")
	{
		pipe << OP_GREATERTHANOREQUAL;
	}
	else if(op == ">")
	{
		pipe << OP_GREATERTHAN;
	}
	else if(op == "||")   // follows Crypto conventions
	{
		pipe << OP_CAT;
	}
	else
	{
		assert(false);
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  if_then_else
//
///////////////////////////////////////////////////////////////////////////////

if_then_else::if_then_else(AST_node_ptr _cond, AST_node_ptr _a, AST_node_ptr _b, const streampoint& _p1, const streampoint& _p2) 
	: cond(_cond), a(_a), b(_b), streampos1(_p1), streampos2(_p2)
{
}

bool if_then_else::generate(class Hello_compiler& compiler, istream& f) 
{
	auto& pipe = compiler.pipe;
	compiler.pipe.declare_stmt({streampos1, streampos2});
	bool is_ok = cond->generate(compiler, f);
	pipe << OP_IF;
	if(a)
	{
		is_ok = is_ok && a->generate(compiler, f);
	}
	if(b)
	{
		pipe << OP_ELSE;
		is_ok = is_ok && b->generate(compiler, f);
	}
	pipe << OP_ENDIF;
	return is_ok; 
};

///////////////////////////////////////////////////////////////////////////////
//
// for_loop
//
///////////////////////////////////////////////////////////////////////////////

for_loop::for_loop(string _loop_variable_name, int _first_val, int _last_val, AST_node_ptr _block, const streampoint& _streampos1, const streampoint& _streampos2)
	: loop_variable_name(_loop_variable_name), 
	first_val(_first_val), 
	last_val(_last_val), 
	block(_block),
	streampos1(_streampos1),
	streampos2(_streampos2)
{
}

bool for_loop::generate(class Hello_compiler& compiler, istream& f)
{
	compiler.pipe.declare_stmt({streampos1, streampos2});
	bool ok = true;
	if(first_val < last_val)
	{
		for(int i=first_val; i <= last_val; i++)
		{
			compiler.assign_value_to_variable("i", i);
			if(block)
				ok = ok && block->generate(compiler, f);
		}
	}
	else 
	{
		for(int i=last_val; i >= first_val; i--)
		{
			compiler.assign_value_to_variable("i", i);
			if(block)
				ok = ok && block->generate(compiler, f);
		}
	}
	return ok;
}

///////////////////////////////////////////////////////////////////////////////
//
// sequence
//
///////////////////////////////////////////////////////////////////////////////

sequence::sequence(AST_node_ptr _a, AST_node_ptr _b) 
	: a(_a), b(_b)
{
}

void sequence::append(AST_node_ptr s) 
{ 
	if(!a)
		a = s;
	else if(!b)
		b = s;
	else
		b = make_shared<sequence>(b, s);
}

bool sequence::generate(class Hello_compiler& compiler, istream& f)
{
	bool is_ok = true;
	if(a) is_ok = a->generate(compiler, f);
	if(b && is_ok) is_ok = b->generate(compiler, f);
	return is_ok;
};

///////////////////////////////////////////////////////////////////////////////
//
// native_function
//
///////////////////////////////////////////////////////////////////////////////

native_function::native_function(const string& _function_name, opcodetype _opcode)
	: function_name(_function_name),
	  opcode(_opcode)
{
}

bool native_function::generate(class Hello_compiler& compiler, istream& f)
{
	if(opcode == OP_RETURN)
	{
		compiler.pipe << OP_RETURN;
		args->generate(compiler, f);
	}
	else
	{
		args->generate(compiler, f);
		compiler.pipe << opcode;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//
// assertion
//
///////////////////////////////////////////////////////////////////////////////

assertion::assertion(AST_node_ptr _cond, const streampoint& _p1, const streampoint& _p2)
	: cond(_cond),
	  streampos1(_p1),
	  streampos2(_p2)
{
}

bool assertion::generate(class Hello_compiler& compiler, istream& f)
{
	if((compiler.options & ASSERTS_ON) == ASSERTS_ON)
	{
		auto& pipe = compiler.pipe;
		compiler.pipe.declare_stmt({streampos1, streampos2});
		bool ok = cond->generate(compiler, f);
		// If the SCRIPT_VERIFY_DISCOUNRAGE_UPGRADABLE_NOPS is set, OP_NOP4 will cause a detectable error.
		pipe << OP_NOTIF << OP_NOP4 << OP_ENDIF;
		return ok;
	}
	return true;
}

