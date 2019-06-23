#pragma once
// Copyright (c) Shaun O'Kane, 2010, 2018

#include <map>
#include <tuple>
#include <istream>
#include <ostream>
#include <algorithm>
#include "script/script.h"
#include "script/standard.h"

#ifdef _WINDOWS
#define WINDOW_EXPORT __declspec(dllexport)
#else
#define WINDOW_EXPORT
#endif


typedef std::vector<uint8_t> valtype; 
typedef std::tuple<std::streampos, size_t, size_t> streampoint;
typedef struct { streampoint start, end; } stmt;  // stmts are specified by offset into source stream.
typedef std::vector<stmt> stmt_table;

// This needs a lot of work.
class WINDOW_EXPORT symbol_table : private std::map<std::string, size_t>  // variable name -> index in alt-stack
{
	std::vector<std::string> variable_names;  // index -> variable name
public:
	using std::map<std::string, size_t>::size;
	using std::map<std::string, size_t>::find;
	using std::map<std::string, size_t>::end;
	using std::map<std::string, size_t>::clear;
	const std::string& at(size_t) const;
	size_t insert(const std::string& variable_name);

	void relocate_variable_to_stack(class instruction_pipeline& pipe, const std::string& variable_name);
	void relocate_variable_to_altstack(class instruction_pipeline& pipe);
	void declare_new_variable(class instruction_pipeline& pipe, const std::string& variable_name);
};

struct WINDOW_EXPORT instruction
{
	instruction(opcodetype op);
	instruction(const valtype& v);
	instruction(const instruction& instr);
	enum class instruction_type : unsigned char { opcode, data } type;
	union  // TODO: replace with std::variant.
	{
		valtype v;
		enum opcodetype opcode;
	};
	size_t generating_stmt; // Need map instruction -> statment (index into stmt_table)
	~instruction();
	instruction& operator=(const instruction&);
	bool operator==(enum opcodetype opcode) const;
	// Needed for dllexport
	instruction() {}
};

class WINDOW_EXPORT instruction_pipeline : public std::vector<instruction>
{
	uint32_t& options;
	stmt_table& stmts;
public:
	instruction_pipeline(stmt_table& stmts, uint32_t& options);
	instruction_pipeline& operator<<(enum opcodetype opcode);
	instruction_pipeline& operator<<(const valtype& v);
	instruction_pipeline& operator<<(const CScriptNum& v);
	instruction_pipeline& operator<<(int n);
	void declare_stmt(const stmt&);
};

std::tuple<CScript, bool, std::string> try_parse_script(std::string text)  noexcept;

typedef std::vector<valtype> stack_type;

class WINDOW_EXPORT executable   // Base class for script engines/interpreters/compilers.
{
public:
	executable() : pipe(stmts, options) {}
	virtual ~executable() {}

	virtual std::pair<bool, std::string> step_over() = 0;
	virtual std::pair<bool, std::string> compile(std::istream& in_filename, std::ostream& out_filename) = 0;
	virtual std::pair<bool, std::string> execute(std::string script_txt) = 0;
	virtual std::pair<bool, std::string> go() = 0;
	virtual std::pair<bool, std::string> step_into() = 0;
	virtual void stop() = 0;
	virtual void reset()
	{
		symbol_table.clear(); stmts.clear(); pipe.clear(); stack.clear(); alt_stack.clear(); vfExec.clear();
	}
	virtual bool set_options(uint32_t options) = 0;

	// Compiler state
	bool debugger_active = true;

	// High-level language constructs
	uint32_t options;
	symbol_table symbol_table;
	stmt_table stmts; 
	instruction_pipeline pipe;

	// Low-level Bitcoin constructs 
	uint32_t flags = MANDATORY_SCRIPT_VERIFY_FLAGS | SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS;
	std::vector<valtype> stack;
	std::vector<valtype> alt_stack;
	std::vector<bool> vfExec;
};

WINDOW_EXPORT executable* create_Hello_compiler();
typedef enum
{
	ASSERTS_ON=0x01,
	OPTIMISER_ON=0x02,
	OUTPUT_ANNOTATED_SCRIPT=0x04
} Hello_compiler_options;

WINDOW_EXPORT bool EvalScript(std::vector<valtype>& stack,
	std::vector<valtype>& altstack,
	std::vector<bool>& vfExec,
	const CScript& script,
	uint32_t flags,
	const BaseSignatureChecker& checker,
	ScriptError* serror);


