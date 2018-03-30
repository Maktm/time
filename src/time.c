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
	double kernel_time;
	double user_time;
	int running;
} STATS;

typedef struct
{
	int minute;
	int second;
	int millisecond;
} DURATION;

void millisecond_to_duration(double ms, DURATION* duration)
{
	duration->minute = (int)ms / (1000 * 60);
	duration->second = ((int)ms % (1000 * 60)) / 1000;
	duration->millisecond = ((int)ms % (1000 * 60)) % 1000;
}

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

void start_stats_test(STATS* stats)
{
#define MILLISECOND_FREQUENCY 1000.0

	LARGE_INTEGER freq;

	if (!QueryPerformanceFrequency(&freq))
	{
		// DANGER: We can't print an error from here because
		// we might have already disabled the output. Instead,
		// a quick exit like this will be easy to debug.
		assert(0);
	}

	stats->frequency = (double)(freq.QuadPart) / MILLISECOND_FREQUENCY;
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

void execute_child_and_wait(wchar_t* runnable_command, STATS* stats)
{
	BOOL success;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	SYSTEMTIME system_time;
	FILETIME creation, exited, kernel, user;

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	si.cb = sizeof(si);

	memset(stats, 0, sizeof(*stats));
	start_stats_test(stats);

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

	end_stats_test(stats);

	if (!GetProcessTimes(pi.hProcess, &creation, &exited, &kernel, &user)) {
		// DANGER: We can't print an error from here because
		// we might have already disabled the output. Instead,
		// a quick exit like this will be easy to debug.
		assert(0);
	}

	FileTimeToSystemTime(&kernel, &system_time);
	stats->kernel_time = system_time.wMilliseconds;

	FileTimeToSystemTime(&user, &system_time);
	stats->user_time = system_time.wMilliseconds;

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

void print_formatted_time(char duration_type[], DURATION* duration)
{
	printf("%s:\t", duration_type);

	if (duration->minute > 0)
		printf("%02ldm:", duration->minute);

	if (duration->second > 0)
		printf("%02lds:", duration->second);

	printf("%02ldms\n", duration->millisecond);
}

void print_stats(STATS* stats)
{
	DURATION wall_duration, user_duration, kernel_duration;
	double wall_ms, user_ms, kernel_ms;
	char output_format[BUFSIZ];

	LARGE_INTEGER* began = &stats->time_started;
	LARGE_INTEGER* ended = &stats->time_ended;
	wall_ms =
		(double)(ended->QuadPart - began->QuadPart) / stats->frequency;

	millisecond_to_duration(wall_ms, &wall_duration);
	millisecond_to_duration(stats->user_time, &user_duration);
	millisecond_to_duration(stats->kernel_time, &kernel_duration);

	print_formatted_time("wall", &wall_duration);
	print_formatted_time("user", &user_duration);
	print_formatted_time("kernel", &kernel_duration);
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

	execute_child_and_wait(runnable_cmd, &stats);

	if (!popts.show_output)
		enable_io(&io_state);

	print_stats(&stats);

	return 0;
}