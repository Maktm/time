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

#include <io.h>
#include <stdio.h>

#include "io_state.h"

#define IO_STDOUT 1
#define IO_STDERR 2

void store_io_state(IO_STATE* state)
{
	state->stdout_state = _dup(IO_STDOUT);
	state->stderr_state = _dup(IO_STDERR);
	state->stored = 1;
}

void restore_io_state(IO_STATE* state)
{
	if (state->stored) {
		_dup2(state->stdout_state, IO_STDOUT);
		_dup2(state->stderr_state, IO_STDERR);
	}
}

void disable_io(IO_STATE* state)
{
	store_io_state(state);
	freopen("nul", "w", stdout);
	freopen("nul", "w", stderr);
}

void enable_io(IO_STATE* state)
{
	restore_io_state(state);
}