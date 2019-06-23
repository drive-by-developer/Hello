#pragma once
// Copyright (c) Shaun O'Kane, 2010, 2018

#include <string>
#include <functional>
#include <istream>
#include <sstream>
#include <cassert>
#include <cctype>
#include <tuple>
#include <memory>

using namespace std;  // naughty.

typedef enum { _undefined, _integer, _hex, _bool } value_type;

typedef tuple<streampos, size_t, size_t> streampoint;

struct token
{
	typedef enum 
	{
		_undefined = value_type::_undefined,
		_integer = value_type::_integer,
		_hex = value_type::_hex,
		_bool = value_type::_bool,  // not currently used.
		_std,
		_extern,
		_name
	} token_type;

	token() : type(token::_undefined) {}
	token(const string& val, token_type _type = _std) : value(val), type(_type) {}
	token(char ch) : value(1, ch), type(_std) {}

	bool operator==(const token& other) const
	{
		return (type == other.type && type != _undefined);
	}
	bool operator==(const string& t) const { return (type == _std && t == value); }
	bool operator!=(const string& t) const { return !operator==(t); }
	bool operator!=(const token& other) const { return !operator==(other); }
	void reset()
	{
		type = _undefined;
		value.clear();
	}
	bool is_valid() const { return (type != _undefined); }

	string value;
	token_type type;

	const string& type_name(token_type _type) const
	{
#define STR(x) #x
		static string token_type_names[] ={STR(_undefined), STR(_integer), STR(_hex), STR(_bool), STR(_std), STR(_name)};
		return token_type_names[_type];
	}
};

inline ostream& operator<<(ostream& str, const token& t)
{
	str << "type=" << t.type_name(t.type);
	if(t.type != _undefined)
		str << ", value='" << t.value << "'";
	return str;
}

struct AST_node
{ 
	virtual bool generate(class Hello_compiler& compiler, istream& f) = 0;
};
typedef shared_ptr<AST_node> AST_node_ptr;

class parse_error : public runtime_error  // localised error condition
{
public:
	parse_error(const string& what, const tuple<streampos, size_t, size_t>& _pos)
		: runtime_error(what),
	      pos(_pos)
	{}
	const tuple<streampos, size_t, size_t> pos;
};

//static const char* separators = " \t\n\r\v";

class tokeniser 
{
	char getch()
	{
		char ch = f.get();
		if (!f.good())
			throw parse_error("Unexpected EOF.", tellg());  // Should already know what is on the stream before calling getch()
		return ch;
	}

	char fpeek() const { return f.peek(); }

	static bool is_name_char(char ch) { return isalnum(ch) || (ch == '_'); }
	static bool is_digit(char ch) { return isdigit(ch) != 0; }
	static bool is_xdigit(char ch) { return isxdigit(ch) != 0; }

	const token f_eat_name() { return f_eat(is_name_char, token::_name); }
	const token f_eat_integer() { return f_eat(is_digit, token::_integer); }	
	const token f_eat_Hex()  // leading "0" should already be consumed.
	{
		char ch = getch();   // 'x'
		assert(ch == 'x');
		token t = f_eat(is_xdigit, token::_hex);
		if(t.value.length()%2 != 0)
			throw parse_error("Hex constants must be have an even number of digits.", tellg());
		return t;
	}

	// Expects at least 1 char to match.
	const token f_eat(function<bool(char)> accept, token::token_type _type)
	{
		stringstream ss;
		for (;;)
		{
			if(f.eof())
				break;
			char ch = f.get();
			if (accept(ch))
				ss << ch;
			else
			{
				f.putback(ch);
				break;
			}
		};
		assert(!ss.str().empty());
		return token(ss.str(), _type);
	}

	const token get_ftoken()
	{
		ws();
		char ch = getch();
		switch (ch)
		{
		case '=':
			if (fpeek() == '=') 
			{ 
				f.get(); 
				return token("=="); 
			}
			else 
				return token("=");
		case '<':
			if (fpeek() == '=')
			{
				f.get();
				return token("<=");
			}
			else
				return token("<");
		case '>':
			if (fpeek() == '=')
			{
				f.get();
				return token(">=");
			}
			else
				return token(">");
		case '|':
			if (fpeek() == '|')
			{
				f.get();
				return token("||");
			}
			else
				return token("|");
		case '&':
			if (fpeek() == '&')
			{
				f.get();
				return token("&&");
			}
			else
				return token("&");
		case '.':
			if (fpeek() == '.')
			{
				f.get();
				return token("..");
			}
			else
				throw parse_error("Unexpected character '.'", tellg());
		case '!':
			if (fpeek() == '=')
			{
				f.get();
				return token("!=");
			}
			else
				return token("!");
		case ',':
		case '~':
		case '+':
		case '-':
		case '%':
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case ':':
		case ';':
			return token(ch);		
		default:
			if (isdigit(ch))
			{
				if ((ch == '0') && (fpeek() == 'x'))
					return f_eat_Hex();
				f.putback(ch);
				return f_eat_integer();  // used for for loops
			}
			else if (isalpha(ch) || ch == '$')
			{
				if(ch != '$')
					f.putback(ch);
				token t = f_eat_name();
				if (   t.value == "if" || t.value == "else" 
					|| t.value == "for" || t.value == "in"
					|| t.value == "true" || t.value == "false"
					|| t.value == "and" || t.value == "or"
					|| t.value == "Assert") // keywords not names
					t.type = token::_std;
				if(ch == '$')
					t.type = token::_extern; 
				return t;
			}
			else
			{
				stringstream ss; ss << "Unexpected character '" << ch << "' encountered.";
				throw parse_error(ss.str(), tellg());
			}
		}
	}

	const token get_token()
	{
		if (stored_token.is_valid())
		{
			token t = stored_token;
			stored_token.reset();
			return t;
		}
		else
			return get_ftoken();
	}

public:
	tokeniser(istream& _f)
		: f(_f),
		lineno(0),
		pos_line_start(0)
	{
	}

	const token peek()
	{
		if(!stored_token.is_valid())
		{
			stored_token_fpos = tellg();
			stored_token = get_ftoken();
		}
		return stored_token;
	}

	void push_token(const token& t)
	{
		if (stored_token.is_valid())
			throw parse_error("INTERNAL ERROR: Only one token can be pushed back at a time.", tellg());
		stored_token = t;
	}

	const token eat_token_type(token::token_type type)
	{
		token t = get_token();
		if(t.type != type)
		{
			stringstream ss; ss << "Unexpected token: " << t; 
			throw parse_error(ss.str(), tellg());
		}
		return t;
	}

	const token eat_name() { return eat_token_type(token::_name); }
	const token eat_integer() { return eat_token_type(token::_integer); }

	void eat() { get_token(); } // gobble up next token.

	void eat(const token& expected)
	{
		const token t = get_token();
		if (t != expected)
		{
			stringstream ss;
			ss << "Expected: '" << expected.value << "', got '" << t.value << "'";  // TODO - bug if not std.
			throw parse_error(ss.str(), tellg());
		}
	}

	void eat(const string& expected) { eat(token(expected)); }

	void ws()	// Eats whitespace and comments
	{ 
		bool in_comment = false;
		for(;;)
		{
			char ch = fpeek(); 
			if(f.eof())
				break;
			if(ch == '\n')
			{
				pos_line_start = f.tellg();
				lineno++;
			}
			if(in_comment || isspace(ch) || ch == '#')
			{
				getch();
				if(ch == '\n')
					in_comment = false;
				else if(ch == '#')
					in_comment = true;
			}
			else
				break;
		}
	}

	bool eof() const 
	{
		if(stored_token.is_valid())
			return false;
		f.peek(); 
		return f.eof(); 
	}

	tuple<streampos, size_t, size_t> tellg() const
	{
		if(stored_token.is_valid())
			return stored_token_fpos;
		long long pos = f.tellg();
		if(pos < 0)	pos = 0;
		size_t offset = pos - pos_line_start;
		return tuple(pos, lineno, offset);
	}
private:
	istream& f;
	tuple<streampos, size_t, size_t> stored_token_fpos;
	streampos pos_line_start;
	size_t lineno;

	token stored_token;
};

