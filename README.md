UPDATES WILL BE DONE REGULARLY EVERY DAY// EXPECT SOME "HOLE" DAYS WITH NO UPDATES

UPDATES WILL ALWAYS BE AT NIGHT CENTRAL EUROPE TIME

Flux 0.1.0 "Foundation"

Features Included

    Core I/O: write for output, wait for user for blocking input

    Conditionals: if user said "..." then reply "..." with exact match

    Variables: set name to "value", set name to other, set name to 42

    User-defined blocks: define name as ... end and call by name

    Foreign Function Interface: load "lib.so" and call for sqrt, puts, sin, cos

    Comments: # to end of line

    Command-line help: --help flag with usage reference

    Memory management: AST cleanup after execution

THE SYNTAXES SUMMARY:
write "Hello"
write variable
wait for user
if user said "hi" then reply "hey"
set name to "Flux"
define greet as
    write "Hello from block!"
end
greet
load "libm.so.6"
call sqrt 16.8
# This is a comment

CONS:

Limitations (by design for v0.1.0)

    if body supports only one statement

    FFI limited to hardcoded non-variadic functions

    No loops or else clause

    No line numbers in error messages

