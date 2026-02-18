# Lox
_Language from Crafting Interpreters._  
__Copyright &copy; 2024 Chris Roberts__ (Krobbizoid).

# Contents
1. [About](#about)
2. [Extensions](#extensions)
   * [`__argc`](#__argc---int)
   * [`__argv`](#__argvindex-int---string--nil)
   * [`__chrat`](#__chrattext-string-index-int---int--nil)
   * [`__exit`](#__exitstatus-int---)
   * [`__fclose`](#__fclosestream-int---bool)
   * [`__fgetc`](#__fgetcstream-int---int--nil)
   * [`__fopenr`](#__fopenrpath-string---int--nil)
   * [`__fopenw`](#__fopenwpath-string---int--nil)
   * [`__fputc`](#__fputcbyte-int-stream-int---int--nil)
   * [`__ftoa`](#__ftoanumber-float---string)
   * [`__stderr`](#__stderr---int)
   * [`__stdin`](#__stdin---int)
   * [`__stdout`](#__stdout---int)
   * [`__strlen`](#__strlentext-string---int)
   * [`__strof`](#__strofbyte-int---string--nil)
   * [`__trunc`](#__truncnumber-float---int)
3. [License](#license)

# About
This is an implementation of the Lox language from the book
[Crafting Interpreters](https://craftinginterpreters.com) by
[Bob Nystrom](https://github.com/munificent). The book covers implementing Lox,
a dynamically-typed, object-oriented language. Lox is first implemented with a
tree-walk interpreter written in Java (jlox), and then with a bytecode
interpreter written in C (clox).

I have followed this book before, so I started on the second half with the
bytecode interpreter. All of the original chapters have been completed and I am
adding some additional features:
* [x] More native functions to improve Lox's I/O capabilities.
* [x] Source merging script for bootstrapping a preprocessor.
* [x] Standard library of useful functions and collections.
* [x] Preprocessor for C-like modularity.
* [ ] Implementation of Lox, or another language in Lox.

# Extensions
This implementation of Lox defines several extension functions to make the
language more capable. Extension functions are prefixed with a double
underscore (`__`) to distinguish them from normal functions and reduce
namespace pollution.

Lox does not distinguish between `int` and `float` number types. If an `int`
parameter is documented, any number will be accepted, but the fractional part
of the number will be truncated. If an `int` return type is documented, the
returned value can be assumed to be an integer.

Calling an extension function is considered undefined behavior if:
* The incorrect number of arguments are passed.
* The incorrect argument types are passed.
* A `NaN` or `Infinity` number value is passed.

Extension functions may not behave as documented during undefined behavior.

## `__argc() -> int`
Return the number of command line arguments, starting at and including the
script path. Always returns `0` in REPL mode and at least `1` outside of REPL
mode.

## `__argv(index: int) -> string | nil`
Return the command line argument at index `index`, where index `0` is the
script path. Returns `nil` if `index` is out of bounds.

## `__chrat(text: string, index: int) -> int | nil`
Return the byte at index `index` of `text`, where index `0` is the first byte.
Returns `nil` if `index` is out of bounds.

## `__exit(status: int) -> !`
Exit with the status code `status`.

## `__fclose(stream: int) -> bool`
Close the file with the stream `stream` and return whether a file was
successfully closed. Files should be closed after opening.

## `__fgetc(stream: int) -> int | nil`
Read and return the next byte from the input stream `stream`. Returns `nil` if
an error occurred or if the end-of-file was reached.

## `__fopenr(path: string) -> int | nil`
Open the file at `path` for reading and return a stream. Returns `nil` if the
file could not be opened.

## `__fopenw(path: string) -> int | nil`
Open the file at `path` for writing and return a stream. Returns `nil` if the
file could not be opened.

## `__fputc(byte: int, stream: int) -> int | nil`
Write the byte `byte` to the output stream `stream` and return the written
byte. Returns `nil` if an error occurred.

## `__ftoa(number: float) -> string`
Return a string representing the number `number`.

## `__stderr() -> int`
Return a constant representing the standard error stream. Returns a value
unique from the other standard streams and any possible file stream.

## `__stdin() -> int`
Return a constant representing the standard input stream. Returns a value
unique from the other standard streams and any possible file stream.

## `__stdout() -> int`
Return a constant representing the standard output stream. Returns a value
unique from the other standard streams and any possible file stream.

## `__strlen(text: string) -> int`
Return the length of `text` in bytes.

## `__strof(byte: int) -> string | nil`
Return a single-byte string containing the byte `byte`. Returns `nil` if `byte`
is less than `1` or greater than `255`.

## `__trunc(number: float) -> int`
Return the number `number` with the fractional part truncated.

# License
This implementation of Lox is released under the MIT license. See
[LICENSE.txt](LICENSE.txt) for a full copy of the license text.

_Crafting Interpreters is copyright of Bob Nystrom._
