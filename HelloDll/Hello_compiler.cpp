// Copyright (c) Shaun O'Kane, 2010, 2018
#include "Hello_compiler.h"
#include <ostream>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include "utilstrencodings.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
//
//  instruction / instruction_pipeline
//
///////////////////////////////////////////////////////////////////////////////

instruction::instruction(opcodetype _op) 
	: type(instruction_type::opcode) 
{
	opcode = _op;
}

instruction::instruction(const valtype& _v)
	: type(instruction_type::data) 
{
	new(&v) valtype(_v);
}

instruction::instruction(const instruction& instr)
	: type(instr.type),
	  generating_stmt(instr.generating_stmt)
{
	if(type == instruction_type::opcode)
		opcode = instr.opcode;
	else
		new(&v) valtype(instr.v);
}

instruction::~instruction() 
{
	if (type == instruction_type::data)
		v.~valtype();
}

instruction& instruction::operator=(const instruction& instr)
{
	type = instr.type;
	switch(type)
	{
	case instruction_type::opcode:
	case instruction_type::data:
	break;
	}
	return *this;
}

inline bool instruction::operator==(enum opcodetype opcode) const
{
	return type == instruction::instruction_type::opcode && opcode == opcode;
}

instruction_pipeline::instruction_pipeline(stmt_table& _stmts, uint32_t& _options)
	: stmts(_stmts),
	  options(_options)
{
}

void instruction_pipeline::declare_stmt(const stmt& stmt) {	stmts.push_back(stmt); }

instruction_pipeline& instruction_pipeline::operator<<(enum opcodetype opcode)
{
	uint32_t options = Hello_compiler_options::OPTIMISER_ON;  // temp
	if((options & Hello_compiler_options::OPTIMISER_ON) == Hello_compiler_options::OPTIMISER_ON)
	{
		switch(opcode)
		{
		case OP_TOALTSTACK:
			if(back() == OP_FROMALTSTACK)
				pop_back();
			break;
		case OP_FROMALTSTACK:
			if(back() == OP_TOALTSTACK)
				pop_back();
			break;
		}
	}
	instruction instr(opcode);
	instr.generating_stmt = stmts.size()-1;
	push_back(instr);
	return *this;
}

instruction_pipeline& instruction_pipeline::operator<<(const valtype& v)
{
	instruction instr(v);
	instr.generating_stmt = stmts.size()-1;
	push_back(instr);
	return *this;
}

instruction_pipeline& instruction_pipeline::operator<<(const CScriptNum& v)
{
	instruction instr(v.getvch());
	instr.generating_stmt = stmts.size()-1;
	push_back(instr);
	return *this;
}

instruction_pipeline& instruction_pipeline::operator<<(int n)
{
	if(n == 0)
		push_back(OP_0);
	else
		push_back(CScriptNum(n).getvch());
	back().generating_stmt = stmts.size()-1;
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
//
//  symbol_table
//
///////////////////////////////////////////////////////////////////////////////

const string& symbol_table::at(size_t idx) const 
{
	assert(map::size() == variable_names.size());
	return variable_names.at(idx);
}

size_t symbol_table::insert(const string& variable_name)
{
	auto p = map::insert(pair(variable_name, map::size()));
	assert(p.second);
	variable_names.push_back(variable_name);
	assert(map::size() == variable_names.size());
	return variable_names.size() - 1;
}

void symbol_table::relocate_variable_to_stack(class instruction_pipeline& pipe, const string& variable_name)
{
}

void symbol_table::relocate_variable_to_altstack(class instruction_pipeline& pipe)
{
}

void symbol_table::declare_new_variable(class instruction_pipeline& pipe, const string& variable_name)
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  Hello_compiler
//
///////////////////////////////////////////////////////////////////////////////

Hello_compiler::Hello_compiler()
{
}

pair<bool, string> Hello_compiler::execute(string script_txt)
{
	ifstream in_stream;
	ofstream out_stream;
	auto result = compile(in_stream, out_stream);
	if(!result.first)
		return result;
	CScript script;
	for(auto instruction : pipe)
	{
		switch(instruction.type)
		{
		case instruction::instruction_type::opcode:
			script << instruction.opcode;
			break;
		case instruction::instruction_type::data:
			script << instruction.v;
			break;
		default:
			assert(false);
		}
	}
	reset();
	{
		const BaseSignatureChecker checker;
		ScriptError serror;
		EvalScript(stack, alt_stack, vfExec, script, flags, checker, &serror);
		if(serror != SCRIPT_ERR_OK)
			return pair(false, "Script evaluation failed.");
	}
	return pair(false, "Not implemented");
}

pair<bool, string> Hello_compiler::go()
{
	return pair(false, "Not implemented");
}

pair<bool, string> Hello_compiler::step_over()
{
	return pair(false, "Not implemented");
}

pair<bool, string> Hello_compiler::step_into()
{
	return pair(false, "Not implemented");
}

void Hello_compiler::stop()
{
}

bool Hello_compiler::set_options(uint32_t _options)
{
	options = _options;
	return true; // TODO: test options are supported.
}

pair<size_t, bool> Hello_compiler::index_of(const string& variable_name)
{
	auto p = symbol_table.find(variable_name);
	if(p == symbol_table.end())
	{
		return pair(symbol_table.insert(variable_name), false);
	}
	return pair(p->second, true);
}

void Hello_compiler::assign_value_to_variable(const string& variable_name, int i)
{
	pipe << i;	
	assign_from_top_of_stack(variable_name);
}

void Hello_compiler::assign_value_to_variable(const string& variable_name, const valtype& v)
{
	pipe << v;
	assign_from_top_of_stack(variable_name);
}

// Pre-condition: alt-stack contains all variable values + value on top of stack.
// Post-condition: alt-stack contains all variables + variable-name has new value
void Hello_compiler::assign_from_top_of_stack(const string& variable_name)
{
	auto [idx, found] = index_of(variable_name);   // updates the symbol table.
	size_t table_size = symbol_table.size();
	if(found)
	{
		// variable already exist in symbol table.
		for(auto k=idx; k<table_size; k++)
		{
			pipe << OP_FROMALTSTACK;
			symbol_table.relocate_variable_to_stack(pipe, variable_name);
		}
		pipe << OP_DROP;
		if(table_size-1 - idx > 0)
			pipe << CScriptNum(table_size-1 - idx) << OP_ROLL;
		for(auto k=idx; k<table_size; k++)
		{
			pipe << OP_TOALTSTACK;
			symbol_table.relocate_variable_to_altstack(pipe);
		}
	}
	else
	{
		pipe << OP_TOALTSTACK;
		symbol_table.declare_new_variable(pipe, variable_name);
	}
}

// Pre-condition: alt-stack contains all variable values.
// Post-condition: alt-stack contains all variables + copy of variable-name on top of stack.
void Hello_compiler::copy_to_top_of_stack(const string& variable_name)
{
	auto [idx, found] = index_of(variable_name);
	if(!found)
	{
		stringstream ss; ss << "Uninitialised variable: '" << variable_name << "'";
		throw runtime_error(ss.str());
	}
	size_t sym_table_size = symbol_table.size();
	for(auto k=idx; k<sym_table_size; k++)
	{
		pipe << OP_FROMALTSTACK;
	}
	pipe << OP_DUP << OP_TOALTSTACK;
	for(auto k=idx+1; k<sym_table_size; k++)
	{
		pipe << OP_SWAP << OP_TOALTSTACK;
	}
}

bool Hello_compiler::fill_pipeline(AST_node_ptr ast, istream& f)
{
	return ast->generate(*this, f);
}

valtype Hello_compiler::get_extern_value(const string& name)
{
	char* val = std::getenv(name.c_str());
	if(val == nullptr)
		get_default_value();
	char* end_ptr = val; while(*end_ptr++);
	return valtype(val, end_ptr);
}

valtype Hello_compiler::get_default_value() { return valtype(); }  // = 0.

pair<bool, string> Hello_compiler::compile(istream& f_in, ostream& f_out)
{
	stringstream in_stream;
	in_stream << f_in.rdbuf();   // not sure this works with cin. TODO: Check

	//
	// Pass 1 - Parse, build AST, fill pipeline.
	//
	reset();
	auto result = compile_internal(in_stream, f_out);
	if(!result.first)
		return result;

	//
	// Pass 2 - Write out annotated script.
	//
	if((options & OUTPUT_ANNOTATED_SCRIPT) == OUTPUT_ANNOTATED_SCRIPT)
	{
		in_stream.clear();
		in_stream.seekg(0, istream::beg);
		write_annotated_script(in_stream, f_out);
	}

	return pair(true, "");
}

pair<bool, string> Hello_compiler::compile_internal(istream& f, ostream& f_out)
{
	Hello_parser parser(f);
	bool ok = true;
	stringstream err_msg;
	parser.ws();
	while(!parser.eof() && ok)
	{
		ok = false;
		try
		{
			auto ast = parser.eat_statement();
			ok = fill_pipeline(ast, f);
			if(!ok)
				break;
		}
		catch(parse_error& err)
		{
			err_msg << "Parsing error. " << err.what() << " at line: " << get<1>(err.pos)+1 << "\n";
		}
		catch(runtime_error& e)
		{
			err_msg << "Compile error. " << e.what() << "\n";
		}
	}
	return pair(ok, err_msg.str());
}

void Hello_compiler::write_annotated_script(istream& f, ostream& f_out)
{
	using namespace chrono;

	time_t t = system_clock::to_time_t(system_clock::now());
	f_out << "### Autogenerated Hello script. Created @ " << ctime(&t);

	set<size_t> processed_stmts;
	for(size_t k=0; k<pipe.size(); k++)
	{
		auto& instr = pipe[k];
		stmt& stmt = stmts[instr.generating_stmt];
		if(processed_stmts.find(instr.generating_stmt) == processed_stmts.end())  // only write out once.
		{
			write_annotation(f, f_out, stmt);
			processed_stmts.insert(instr.generating_stmt);
		}

		if(instr.type == instruction::instruction_type::opcode)
		{
			if(instr.opcode == OP_0)
				f_out << "L1 0x00\n";
			else
				f_out << GetOpName(instr.opcode) << "\n";
		}
		else
			f_out << "L" << instr.v.size() << " 0x" << HexStr(instr.v) << "\n";
	}
}

void Hello_compiler::write_annotation(istream& f, ostream& f_out, const stmt& stmt) const 
{
	streampos origpos = f.tellg();  // push pos.
	streampos pos = get<0>(stmt.start); 
	streampos last = get<0>(stmt.end); 
	assert(pos < last);
	f.seekg(pos);
	f_out << "\n### ";
	while(pos != last)
	{
		assert(!f.eof() && f.good());
		char ch = f.get();
		pos = f.tellg();		

		f_out << ch;
		if(ch == '\n')
			f_out << "### ";
	}
	f_out << "\n";
	f.seekg(origpos);  // pop pos.
}

