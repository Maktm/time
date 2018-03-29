# time
`time` is a tool for measuring the execution speed of commands on Windows similar to
the UNIX `time` program. It currently supports printing the duration for the execution
of a program in a `minute:second:millisecond` format.

<p align="center">
  <img src="/assets/time_example.gif">
</p>

# Usage
The general syntax is:

    time [options] <args>
    
`<args>` can be anything that can be executed as a command from within the environment
you're running `time` from. Output from the `<args>` command is disabled by default but
you can enable it using the `[-s | --show-output]` flags.

# TODO
* Add more output formatting options
* Test cross-compiler compatibility (gcc, clang etc.)

# Author
[Michael Kiros](http://github.com/michaelkiros)

# Credits
A big thank you to the contributors of UNIX's time command but mainly the following people:

* David Keppel
* David MacKenzie
* Assaf Gordon
