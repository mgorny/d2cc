// d2cc: gcc toolchain support
// (c) 2015 Michał Górny
// Distributed under the terms of the 2-clause BSD license

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <cerrno>
#include <cstring>
#include <iostream>
#include <iterator>

extern "C"
{
#	include <sys/types.h>
#	include <sys/wait.h>
#	include <unistd.h>
}

#include "libd2cc/unixclient.h"

static void mangle_argv_for_cpp(char* argv[])
{
	for (char** it = &argv[1]; *it; ++it)
	{
		char* argp = *it;

		if (argp[0] == '-' && argp[1] != '\0')
		{
			if (argp[1] == 'c' && argp[2] == '\0')
				argp[1] = 'E'; // change to -E for preprocessing
			// -o indicates output file
			else if (argp[1] == 'o')
			{
				// force stdout output
				if (argp[2] != '\0') // inline parameter
				{
					argp[2] = '-';
					argp[3] = '\0';
				}
				else // split parameter
				{
					argp = *++it;
					argp[0] = '-';
					argp[1] = '\0';
				}
			}
		}
	}
}

static pid_t run_preprocessor(char* argv[],
		const char* compiler_path,
		int* output_fd)
{
	int cpp_pipes[2];

	if (pipe(cpp_pipes) == -1)
	{
		std::cerr << "d2cc: Unable to create pipe for cpp: "
			<< strerror(errno) << std::endl;
		return false;
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		std::cerr << "d2cc: Unable to fork() for preprocessor run: "
			<< strerror(errno) << std::endl;
		return pid;
	}

	if (pid == 0) // subprocess
	{
		if (dup2(cpp_pipes[1], 1) == -1)
		{
			std::cerr << "d2cc: Unable to replace stdout with a pipe: "
				<< strerror(errno) << std::endl;
			exit(1);
		}
		close(cpp_pipes[0]);
		close(cpp_pipes[1]);

		// since we forked, we can mangle argv in-place
		mangle_argv_for_cpp(argv);

		if (execvp(compiler_path, argv))
		{
			std::cerr << "d2cc: Unable to spawn preprocessor " << compiler_path
				<< ": " << strerror(errno) << std::endl;
			exit(1);
		}
	}

	close(cpp_pipes[1]);
	*output_fd = cpp_pipes[0];

	return pid;
}

static void get_compiler_argv(
		const d2cc::protocol::ArgVMessage::argv_yield_func& yield,
		const char* const* orig_argv)
{
	for (const char* const* it = &orig_argv[1]; *it; ++it)
	{
		const char* argp = *it;

		// Does it look like an option?
		if (argp[0] == '-' && argp[1] != '\0')
		{
			// -o indicates output file
			if (argp[1] == 'o')
			{
				if (argp[2] != '\0') // inline parameter
					yield("-o" D2CC_ARGV_OUTPUT_TAG);
				else // split parameter
				{
					yield(argp);
					yield(D2CC_ARGV_OUTPUT_TAG);
					++it;
				}
				continue;
			}
			// skip preprecessor's make output magic (done locally)
			else if (argp[1] == 'M')
				continue;
		}
		else // input filename
		{
			yield(D2CC_ARGV_INPUT_TAG);
			continue;
		}

		yield(argp);
	}
}

static ssize_t read_cpp_pipe(int fd, uint8_t* buf, size_t buf_size)
{
	ssize_t ret = read(fd, buf, buf_size);
	if (ret == -1)
		std::cerr << "d2cc: Unable to read preprocessor pipe: "
			<< strerror(errno) << std::endl;
	return ret;
}

static bool run_remotely(char* argv[],
		const char* compiler_path,
		const char* in_filename,
		const char* out_filename)
{
	d2cc::UNIXClient cl;
	if (!cl.connect())
		return false;

	if (!cl.send_argv(std::bind(get_compiler_argv,
					std::placeholders::_1, argv)))
		return false;

	int output_fd;
	pid_t cpp_pid = run_preprocessor(argv, compiler_path, &output_fd);
	if (cpp_pid == -1)
		return false;

	if (!cl.send_data(std::bind(read_cpp_pipe, output_fd,
					std::placeholders::_1, std::placeholders::_2)))
		return false;

	int ret;
	if (waitpid(cpp_pid, &ret, 0) == -1)
	{
		std::cerr << "d2cc: Unable to reap preprocessor: "
			<< strerror(errno) << std::endl;
		return false;
	}


	if (WEXITSTATUS(ret) != 0)
	{
		std::cerr << "d2cc: The preprocessor returned non-zero exit status "
			<< WEXITSTATUS(ret) << std::endl;
		return false;
	}
	if (!cl.finish_request())
		return false;

	std::cerr << "d2cc: Remote protocol not implemented yet" << std::endl;

	return false;
}

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
	argv[0] = compiler_path;

	if (is_possible)
	{
		if (run_remotely(argv, compiler_path, in_filename, out_filename))
			return 0;
		std::cerr << "d2cc: Running locally instead ..." << std::endl;
	}

	// TODO: cleanse $PATH to avoid loop-spawning

	// spawn the build directly
	if (execvp(compiler_path, argv))
	{
		std::cerr << "d2cc: Unable to spawn compiler " << compiler_path
			<< ": " << strerror(errno) << std::endl;
		return 1;
	}

	return 0;
}
