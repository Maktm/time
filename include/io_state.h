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

#ifndef TIME_IO_STATE_H
#define TIME_IO_STATE_H

typedef struct
{
	int stdout_state;
	int stderr_state;
	int stored;
}IO_STATE;

void store_io_state(IO_STATE* state);
void restore_io_state(IO_STATE* state);
void disable_io(IO_STATE* state);
void enable_io(IO_STATE* state);

/**
 * The original I/O state of the process used as
 * a backup when disabling/enabling input/output.
 */
extern IO_STATE io_state;

#endif