# Lox
_Language from Crafting Interpreters._  
__Copyright &copy; 2024 Chris Roberts__ (Krobbizoid).

# Contents
1. [About](#about)
2. [Extensions](#extensions)
   * [`__argc`](#__argc---int)
   * [`__argv`](#__argvindex-int---string--nil)
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
* [ ] Source merging script for bootstrapping preprocessor.
* [ ] Preprocessor for basic C-like modularity.
* [ ] Standard library of useful functions and collections.
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

# License
This implementation of Lox is released under the MIT License:  
https://krobbi.github.io/license/2024/mit.txt

See [LICENSE.txt](/LICENSE.txt) for a full copy of the license text.

_Crafting Interpreters is copyright of Bob Nystrom._
