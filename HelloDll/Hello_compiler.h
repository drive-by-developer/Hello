#pragma once
// Copyright (c) Shaun O'Kane, 2010, 2018

#include "tokeniser.h"
#include "parser.h"
#include <map>

using namespace std;

class Hello_compiler :  public executable
{
public:
	Hello_compiler();

	pair<bool, string> compile(istream& in_filename, ostream& out_filename) override;
	pair<bool, string> execute(std::string script_txt) override;
	pair<bool, string> go() override;
	pair<bool, string> step_over() override;
	pair<bool, string> step_into() override;
	void stop() override;
	bool set_options(uint32_t options) override;

	// These are used to get the value of $<variable-name> 
	virtual valtype get_extern_value(const string& name);
	virtual valtype get_default_value();

	pair<size_t, bool> index_of(const string& variable_name);
	void copy_to_top_of_stack(const string& variable_name);
	void assign_from_top_of_stack(const string& variable_name);
	void assign_value_to_variable(const string& variable_name, int i);
	void assign_value_to_variable(const string& variable_name, const valtype& value);

	pair<bool, string> compile_internal(std::istream& f, ostream& f_out);
	bool fill_pipeline(AST_node_ptr ast, istream& f);
	void write_annotated_script(istream& f, ostream& f_out);
	void write_annotation(istream& f, ostream& f_out, const stmt& stmt) const;
};