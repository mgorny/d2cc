// d2cc: gcc toolchain support
// (c) 2015 Michał Górny
// Distributed under the terms of the 2-clause BSD license

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cerrno>
#include <cstring>
#include <iostream>

extern "C"
{
#	include "unistd.h"
};

int main(int argc, char* argv[])
{
	bool is_possible = true;
	bool had_c = false; // -c option
	const char* in_filename = nullptr;
	const char* out_filename = nullptr;

	// Parse gcc argv
	// TODO: improve it a lot
	for (char** it = &argv[1]; *it; ++it)
	{
		const char* argp = *it;

		// Does it look like an option?
		if (argp[0] == '-' && argp[1] != '\0')
		{
			// -c indicates we're compiling without linking
			if (argp[1] == 'c' && argp[2] == '\0')
				had_c = true;
			// -o indicates output file
			else if (argp[1] == 'o')
			{
				if (argp[2] != '\0') // inline parameter
					out_filename = &argp[2];
				else // split parameter
					out_filename = *++it;
			}
			// TODO: options that make distributed compiling impossible
			else if (argp[1] == 'M')
			{
				if (argp[2] == 'T' || argp[2] == 'F')
				{
					if (argp[3] == '\0')
						++it; // skip the argument
				}
			}
			// TODO: other options that take split arguments
		}
		else // input filename
		{
			if (argp[0] == '-')
			{
				// stdin? won't handle it right now
				// TODO: is it worth the effort to implement it?
				is_possible = false;
				break;
			}

			// find the last suffix for matching
			const char* suffix = strrchr(argp, '.');
			if (!suffix || !strcmp(suffix, ".o"))
			{
				is_possible = false;
				break;
			}

			// multiple input files?
			if (in_filename)
			{
				is_possible = false;
				break;
			}

			in_filename = argp;
		}
	}

	// Mangle compiler path
	char* compiler_path = argv[0];
	char* compiler_slash = strrchr(argv[0], '/');
	if (compiler_slash)
		compiler_path = &compiler_slash[1];

	if (is_possible)
	{
		// TODO: implement d2cc server comms
	}

	// TODO: cleanse $PATH to avoid loop-spawning

	// spawn the build directly
	argv[0] = compiler_path;
	if (execvp(compiler_path, argv))
	{
		std::cerr << "Unable to spawn compiler " << compiler_path
			<< ": " << strerror(errno) << std::endl;
		return 1;
	}

	return 0;
}
