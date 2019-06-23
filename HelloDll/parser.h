#pragma once
// Copyright (c) Shaun O'Kane, 2010, 2018

#include "AST.h"
#include <unordered_map>

struct function_info
{
	int no_of_args; opcodetype opcode;
};
typedef unordered_map<string, function_info> function_info_table_type;

extern const function_info_table_type special_functions;
extern const function_info_table_type native_functions;

class Hello_parser : public tokeniser
{
	vector<streampoint> streampoints;
	streampoint declare_streampoint() 
	{ 
		auto cp = tellg(); 
		streampoints.push_back(cp); 
		return cp; 
	}
public:
	AST_node_ptr eat_value()
	{
		const token t = peek();
		if (t.type == token::_integer || t.type == token::_hex)
			eat(t);
		return make_shared<const_node>(t.value, value_type((int)t.type));
	}

	//////////////////////// Native Functions ///////////////////////
	
	bool is_special_function(const token& t) { return special_functions.find(t.value) != special_functions.end(); }
	bool is_native_function(const token& t) { return native_functions.find(t.value) != native_functions.end(); }

	AST_node_ptr eat_native_function(function_info_table_type function_info_table)
	{
		const token t = eat_name();
		auto p = function_info_table.find(t.value);
		assert(p != function_info_table.end());
		size_t no_of_args = (size_t)p->second.no_of_args;
		opcodetype opcode = p->second.opcode;

		auto func = make_shared<native_function>(t.value, opcode);
		auto args = make_shared<sequence>();
		eat("(");
		for(size_t i=0; i<no_of_args; i++)
		{
			if(peek() == ")" && no_of_args == ((size_t)-1))  // handle variable args case.
				break;
			// process arg.
			if(i != 0)
				eat(",");
			args->append(eat_expression());
		}
		eat(")");
		func->args = args;
		return func;
	}
	/////////////////// End of Native Functions /////////////////////

	////////////////////// Numeric Expressions //////////////////////

	AST_node_ptr eat_factor()
	{
		const token t = peek();
		if (t == "(")
		{
			eat("(");
			auto expr = eat_expression(); 
			eat(")");
			return expr;
		}
		else if (t == "-") 
		{
			eat("-"); 
			return make_shared<unary_op>("-", eat_factor());
		} 
		else if(is_native_function(t.value))
		{
			return eat_native_function(native_functions);
		}
		else if (t.type == token::_name || t.type == token::_extern)
		{
			return make_shared<rvalue_node>(eat_name().value, t.type == token::_extern);
		}
		else
			return eat_value();
	}

	AST_node_ptr eat_term()
	{
		auto a = eat_factor();
		AST_node_ptr p = a;
		for (string op = peek().value; op == "*" || op == "/" || op == "%" || op == "||"; op = peek().value)
		{
			eat(op);
			auto f = eat_factor();
			p = make_shared<binary_op>(op, p, f);
		}
		return p;
	}

	AST_node_ptr eat_expression()
	{
		auto a = eat_term();		
		AST_node_ptr p = a;
		for (string op = peek().value; op == "+" || op == "-"; op = peek().value)
		{
			eat(op);
			auto b = eat_term();
			p = make_shared<binary_op>(op, p, b);
		}
		return p;
	}

	/////////////////// End of Numeric Expressions //////////////////

	/////////////////////// Bit Manipulations ///////////////////////

	// No shift operators yet.
	/*AST_node eat_bitwise_ops()
	{
		eat();
		string op = peek().value;
		if (op == "&" || op == "|" || op == "~")
		{
			eat(op);
			eat();
		}
		return v;
	}*/

	//////////////////// End of Bit Manipulations ///////////////////

	////////////////////// Logical Expressions //////////////////////

	AST_node_ptr eat_logical_expression()
	{
		auto a = eat_logical_value();
		AST_node_ptr p = a;
		for (string op = peek().value; op == "or" || op == "and"; op = peek().value)
		{
			eat(op);
			auto b =  eat_logical_value();
			return make_shared<binary_op>(op, p, b);
		}
		return p;
	}

	AST_node_ptr eat_logical_value()
	{
		const token t = peek();
		if (t == "(")	
		{
			eat("("); 
			auto v = eat_logical_expression(); 
			eat(")");
			return v;
		}
		else if (t == "!") 
		{
			eat("!"); 
			auto v = eat_logical_value();
			return make_shared<unary_op>("!", v);
		}
		else
			return eat_comparison();
	}

	AST_node_ptr eat_comparison()
	{
		auto v = eat_expression();
		string comps[]{ "<", "<=", "==", "!=", ">=", ">" };
		auto op = find(begin(comps), end(comps), peek().value);
		if(op == end(comps))
		{
			stringstream ss; ss << "Unexpected token: " << peek().value; 
			throw parse_error(ss.str(), tellg());
		}
		eat(*op);
		auto w = eat_expression();
		return make_shared<binary_op>(*op, v, w);
	}

	/////////////////// End of Logical Expressions //////////////////

	AST_node_ptr eat_block()
	{
		eat("{");
		auto stmts = make_shared<sequence>();
		while (peek() != "}")
		{
			stmts->append(eat_statement());
		}
		eat("}");
		if(!stmts->a)
			return AST_node_ptr();
		if(!stmts->b)
			return stmts->a;  // Only one statement in brackets
		else
			return stmts;
	}

	AST_node_ptr eat_conditional()
	{
		auto p1 = declare_streampoint();
		eat("if"); 
		eat("("); 
		AST_node_ptr cond = eat_logical_expression(); 
		eat(")");
		auto p2 = declare_streampoint();
		AST_node_ptr a = eat_block();
		AST_node_ptr b;
		ws();
		if (!eof() && peek().value == "else")
		{
			eat("else");
			b = eat_block();
		}
		return make_shared<if_then_else>(cond, a, b, p1, p2);
	}

	AST_node_ptr eat_loop()
	{
		auto p1 = declare_streampoint();
		eat("for"); auto loop_var_name = eat_name(); eat("in"); eat("["); auto& a = eat_integer(); eat(".."); auto& b = eat_integer(); eat("]");
		auto p2 = declare_streampoint();
		int an = atoi(a.value.c_str()); int bn = atoi(b.value.c_str());
		return make_shared<for_loop>(loop_var_name.value, an, bn, eat_block(), p1, p2);
	}

	AST_node_ptr eat_assignment()
	{
		auto streampos1 = declare_streampoint();
		token name = eat_name();
		eat("=");
		AST_node_ptr v = eat_expression();		
		eat(";"); 
		auto streampos2 = declare_streampoint();
		return make_shared<assign_op>(name.value, v, streampos1, streampos2);
	}

	AST_node_ptr eat_assert()
	{
		auto streampos1 = declare_streampoint();
		eat("Assert");
		eat("(");
		AST_node_ptr cond = eat_logical_expression(); 
		eat(")");
		eat(";"); 
		auto streampos2 = declare_streampoint();
		return make_shared<assertion>(cond, streampos1, streampos2);
	}

	AST_node_ptr eat_statement()
	{
		AST_node_ptr v; bool ok;
		if(peek() == "Assert")
			v = eat_assert();
		else if(is_native_function(peek()))
			eat_native_function(special_functions); 
		else if(peek() == "if")
			v = eat_conditional();
		else if (peek() == "for")
			v = eat_loop();
		else
			v = eat_assignment();
		ws();
		return v;
	}

	Hello_parser(istream& f)
		: tokeniser(f)
	{}

	map<string, int> symbol_table;
};


