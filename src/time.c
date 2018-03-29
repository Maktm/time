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

#ifdef _MSC_VER
// Disables "unsafe" MSVC warnings.
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

#include <windows.h>

#include "io_state.h"
#include "version.h"

IO_STATE io_state;

typedef struct
{
	int show_output;
} PROGRAM_OPTIONS;

typedef struct
{
	LARGE_INTEGER time_started;
	LARGE_INTEGER time_ended;
	double frequency;
	int running;
} STATS;

void usage()
{
	fprintf(stderr,
		"usage: time [-v | --version] [-h | --help] [-s | --show-output]\n"
		"\t    [<args>]\n");
	exit(EXIT_FAILURE);
}

void version()
{
	fprintf(stdout, "time version %d.%d.%d\n",
		TIME_VER_MAJOR,
		TIME_VER_MINOR,
		TIME_VER_PATCH);
	exit(EXIT_SUCCESS);
}

void execute_child_and_wait(wchar_t* runnable_command)
{
	BOOL success;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	si.cb = sizeof(si);

	success = CreateProcessW(
		NULL,
		runnable_command,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&si,
		&pi
	);

	WaitForSingleObject(pi.hProcess, INFINITE);

	if (!success) {
		fprintf(stderr, "Fatal error: failed to execute child process\n");
		exit(EXIT_FAILURE);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void create_runnable_command(int argc, wchar_t** argv, int start, wchar_t* runnable_cmd)
{
	int index;
	wchar_t preserved_argv[BUFSIZ];

	index = start;
	while (index < argc) {
		memset(preserved_argv, 0, BUFSIZ);
		wsprintf(preserved_argv, L"\"%ls\"", argv[index]);

		wcscat(runnable_cmd, preserved_argv);
		wcscat(runnable_cmd, L" ");

		index++;
	}
}

void start_stats_test(STATS* stats)
{
#define MICROSECOND_FREQUENCY 1000000.0

	LARGE_INTEGER freq;

	if (!QueryPerformanceFrequency(&freq))
	{
		// DANGER: We can't print an error from here because
		// we might have already disabled the output. Instead,
		// a quick exit like this will be easy to debug.
		assert(0);
	}

	stats->frequency = (double)(freq.QuadPart) / MICROSECOND_FREQUENCY;
	stats->running = 1;
	QueryPerformanceCounter(&stats->time_started);

#undef MICROSECOND_FREQUENCY
}

void end_stats_test(STATS* stats)
{
	if (stats->running) {
		QueryPerformanceCounter(&stats->time_ended);
		stats->running = 0;
	}
}

void print_stats(STATS* stats)
{
	double minute, second, millisecond, microsecond;

	minute = second = millisecond = microsecond = 0.0;

	LARGE_INTEGER* began = &stats->time_started;
	LARGE_INTEGER* ended = &stats->time_ended;
	microsecond =
		(double)(ended->QuadPart - began->QuadPart) / stats->frequency;

	millisecond = microsecond / 1000;
	second = millisecond / 1000;
	minute = second / 60;

	printf("%02ldm:%02lds:%02ldms\n",
		(long int)minute,
		(long int)second,
		(long int)millisecond);
}

int wmain(int argc, wchar_t** argv)
{
	int i;
	wchar_t runnable_cmd[BUFSIZ];
	PROGRAM_OPTIONS popts;
	STATS stats;

	if (argc < 2) {
		usage();
	}

	memset(&io_state, 0, sizeof(io_state));
	memset(&popts, 0, sizeof(popts));
	for (i = 1; i < argc && argv[i][0] == L'-'; i++) {
		if (!wcscmp(argv[i], L"--version") || !wcscmp(argv[i], L"-v"))
			version();

		if (!wcscmp(argv[i], L"--help") || !wcscmp(argv[i], L"-h"))
			usage();

		if (!wcscmp(argv[i], L"--show-output") || !wcscmp(argv[i], L"-s"))
			popts.show_output = 1;
	}

	if (i == argc)
		usage();

	if (!popts.show_output)
		disable_io(&io_state);

	memset(runnable_cmd, 0, BUFSIZ);
	create_runnable_command(argc, argv, i, runnable_cmd);

	memset(&stats, 0, sizeof(stats));
	start_stats_test(&stats);

	execute_child_and_wait(runnable_cmd);

	end_stats_test(&stats);

	if (!popts.show_output)
		enable_io(&io_state);

	print_stats(&stats);

	return 0;
}