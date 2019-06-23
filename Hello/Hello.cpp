#include <fstream>
#include <iostream>
#include "../Common/Internals.h"

using namespace std;

class args
{
	int argc; const char** argv;
	const char* options;
	const char switch_ch = '-'; 	// typically either '-' or '/'
	int current_opt_index = 1;
	int eof();
	int parse_error();
public:	
	args(int argc, const char** argv, const char* options);
	const char* arg;		// option
	const char* optarg;		// additional param
	int getopt();
};

args::args(int _argc, const char** _argv, const char* _options)
	: argc(_argc), argv(_argv), options(_options)
{
	assert(options != nullptr);
}

int args::eof() { current_opt_index = argc; return -1; }
int args::parse_error() { current_opt_index = argc; return ('?'); }

// Mutant version of getopt. Just couldn't leave the C version alone. Ugh. And boost seemed like overkill...
int args::getopt()
{
	optarg = nullptr;
	if(current_opt_index < argc)
	{
		arg = argv[current_opt_index++];
		if(*arg != switch_ch)
			return '?';
		int ch = *(++arg);
		if(ch == switch_ch || ch == '\0')
		{
			// "--" and "-" signifies end of options.
			return eof();
		}
		const char* p;	// points to the option char in options
		if((p = strchr(options, ch)) == NULL)
			return parse_error(); // not an option
		if(':' == *(++p))
		{
			// option needs a parameter.
			if(current_opt_index < argc)
				optarg = argv[current_opt_index++];
			else
				return parse_error();
		}
		return ch;
	}
	else
		return eof();
 }

void usage()
{
	cout << "Hello -t | [-o] -f <in-filename> [-o <out-filename>]\n"
		    "\n"
			"\t-f <in-filename>  \tOptional. Compile <in-filename>.\n"
			"\t                  \tDefault is std input if in-filename does not exists.\n";
			"\t-o <out-filename> \tOptional. Compile output to <out-filename>. \n"
			"\t                  \tDefault is <in-filename>.script if out-filename does not exists.\n"
			"\t                  \tIf both in-filename and out-filename do not exist, defaults to std output.\n"
			"\t-O                \tOptional. Optimiser on. Default is optimiser off.\n";
}

int main(int argc, char** argv)
{
	args cmdline(argc, (const char**)argv, "f:o:O");
	
	// Command line options.
	string in_filename, out_filename;
	bool optimiser_on = false;
	bool execute = false;

	// Update options from Command line.
	int ch;
	while((ch = cmdline.getopt()) != -1)
	{
		switch(cmdline.arg[0])
		{
		case 'f':
			in_filename = cmdline.optarg;
			break;
		case 'o':
			out_filename = cmdline.optarg;
			break;
		case 'O':
			optimiser_on = true;
			break;
		case 'x':
			execute = true;
			break;
		}
	}
	if(ch == '?')
	{
		usage();
		return -1;
	}

	// Open the input stream
	istream* in_stream = nullptr;
	ifstream f_in;
	if(!in_filename.empty())
	{
		f_in.open(in_filename);
		if(!f_in.is_open())
		{
			cerr << "Unable to read input stream.\n";
			return -1;
		}
		in_stream = &f_in;
	}
	else
		in_stream = &cin;

	// Open the output stream
	ostream* out_stream = nullptr;
	ofstream f_out;
	if(!out_filename.empty() || !in_filename.empty())
	{
		if(out_filename.empty())
			out_filename = in_filename + ".script";
		f_out.open(out_filename);
		if(!f_out.is_open())
		{
			cerr << "Unable to open output stream.\n";
			return -1;
		}
		out_stream = &f_out;
	}
	else
		out_stream = &cout;

	// Create the compiler.
	shared_ptr<executable> compiler(create_Hello_compiler());
	if(optimiser_on)
		compiler->set_options(Hello_compiler_options::OPTIMISER_ON);
	compiler->set_options(OUTPUT_ANNOTATED_SCRIPT);

	// Compile the code.
	auto [ok, err_str] = compiler->compile(*in_stream, *out_stream);
	if(!ok)
	{
		cerr << "'" << in_filename << "' compilation failed. " << err_str << flush;
		return -1;
	}
	return 0;
}



