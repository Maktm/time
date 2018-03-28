/**
*  The MIT License
*
* Copyright (c) 2018 Michael Kiros
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <process.h>
#include <windows.h>
#include <assert.h>

#define EXECV_VER_MAJOR 0
#define EXECV_VER_MINOR 1

#define DUP_STDOUT 1
#define DUP_STDERR 2

typedef struct
{
	LARGE_INTEGER start;
	LARGE_INTEGER finish;
	double frequency;
	int finished;
} PERFORMANCE;

/**
* Used for enabling/disabling I/O to the console
* depending on the flags used.
*/
int stdout_duplicate;
int stderr_duplicate;

void save_io_state()
{
	stdout_duplicate = _dup(DUP_STDOUT);
	stderr_duplicate = _dup(DUP_STDERR);
}

void enable_io()
{
	_dup2(stdout_duplicate, DUP_STDOUT);
	_dup2(stderr_duplicate, DUP_STDERR);
}

void disable_io()
{
	freopen("nul", "w", stdout);
	freopen("nul", "w", stderr);
}

void usage()
{
	fprintf(stderr, "usage: time [-options] <command>\n");
	exit(EXIT_FAILURE);
}

void version()
{
	fprintf(stdout, "time %d.%d\n",
		EXECV_VER_MAJOR, EXECV_VER_MINOR);
	exit(EXIT_FAILURE);
}

int locate_executev_path(char* time_path)
{
	/**
	* In the current state of this program, there's a requirement
	* that execv is placed in the same directory as time. If not,
	* then the application won't be able to run.
	*/
	int state;
	char* rewrite_position;
	char* nullify_position;
	char* execv_path;
	struct _stat sb;

	rewrite_position = strrchr(time_path, '\\');
	if (rewrite_position == NULL)
		rewrite_position = time_path;

	nullify_position = rewrite_position + strlen("\\execv.exe");
	strcpy(rewrite_position, "\\execv.exe");

	execv_path = time_path;
	state = 1;
	if (!_stat(execv_path, &sb)) {
		state = 0;
	}

	return state;
}

void restore_quotation_marks(char* argv, char* restored_argv)
{
	/**
	* Running from a console/terminal will remove the quotation
	* marks creating issues for commands such as:
	*			python -c "for i in range(0, 100): pass"
	*
	* so we're adding quotation marks to all the command-line
	* arguments provided.
	*/
	char restored_arg[BUFSIZ];
	memset(restored_arg, 0, BUFSIZ);
	sprintf(restored_arg, "\"%s\"", argv);

	memset(restored_argv, 0, strlen(restored_arg) + 1);
	memcpy(restored_argv, restored_arg, strlen(restored_arg));
}

void generate_command_args(
	int argc, int cmd_index, char** argv, char* execv_path, char* execv_args)
{
	int i;

	i = cmd_index;
	for (; i < argc; i++) {
		char restored_argv[BUFSIZ];
		memset(restored_argv, 0, BUFSIZ);
		restore_quotation_marks(argv[i], restored_argv);

		strcat(execv_args, restored_argv);
		strcat(execv_args, " ");
	}
}

void start_performance_measurement(PERFORMANCE* performance)
{
	if (!QueryPerformanceFrequency(&performance->start)) {
		// DANGER: We can't print an error from here because
		// we might have already disabled the output. Instead,
		// a quick exit like this will be easy to debug.
		assert(0);
	}

	performance->frequency = (double)(performance->start.QuadPart) / 1000.0;
	QueryPerformanceCounter(&performance->start);
}

void stop_performance_measurement(PERFORMANCE* performance)
{
	QueryPerformanceCounter(&performance->finish);
	performance->finished = 1;
}

double measure_difference(PERFORMANCE* performance)
{
	double elapsed;

	elapsed = 0.0;
	if (performance->finished) {
		elapsed = (double)
			(performance->finish.QuadPart -
				performance->start.QuadPart) / performance->frequency;
	}

	return elapsed;
}

int main(int argc, char** argv)
{
	int i, status;
	int command_index;
	int flag_output_enabled;
	PERFORMANCE performance;
	double elapsed;
	char execv_path[BUFSIZ], execv_args[BUFSIZ];
	PROCESS_INFORMATION pi;
	STARTUPINFOA si;
	BOOL success;
	DWORD exit_code;

	if (argc < 2)
		usage();

	save_io_state();

	/**
	* Enable the output if the correct flag for enabling
	* output during assessment is provided. Check other
	* flags beforehand (e.g. -v).
	*/
	flag_output_enabled = 0;
	command_index = 0;
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--version") || !strcmp(argv[i], "-v")) {
			version();
		}

		if (!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o")) {
			flag_output_enabled = 1;
		}

		/**
		* If the last argument is a non-option then we know
		* that a command was provided.
		*/
		if (!command_index && argv[i][0] != '-') {
			command_index = i;
			break;
		}
	}

	if (!command_index) {
		usage();
	}

	memset(execv_path, 0, BUFSIZ);
	memcpy(execv_path, argv[0], strlen(argv[0]));
	status = locate_executev_path(execv_path);

	if (status) {
		fprintf(stderr,
			"Fatal error: You must have execv in the same directory as time\n");
		exit(EXIT_FAILURE);
	}

	if (!flag_output_enabled)
		disable_io();

	memset(execv_args, 0, BUFSIZ);
	generate_command_args(argc, command_index, argv, execv_path, execv_args);

	start_performance_measurement(&performance);

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	success = CreateProcessA(
		NULL,
		execv_args,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&si,
		&pi);

	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &exit_code);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	stop_performance_measurement(&performance);
	elapsed = measure_difference(&performance);

	if (!flag_output_enabled)
		enable_io();

	if (exit_code != 0) {
		fprintf(stderr, "Failed to execute child process using execv\n");
		return 1;
	} else {
		fprintf(stderr, "Elapsed: %fms\n", elapsed);
	}

	return 0;
}