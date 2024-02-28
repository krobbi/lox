# Lox
_Language from Crafting Interpreters._  
__Copyright &copy; 2024 Chris Roberts__ (Krobbizoid).

# Contents
1. [About](#about)
   * [Completed Chapters](#completed-chapters)
2. [License](#license)

# About
This is an implementation of the Lox language from the book
[Crafting Interpreters](https://craftinginterpreters.com) by
[Bob Nystrom](https://github.com/munificent). The book covers implementing Lox,
a dynamically-typed, object-oriented language. Lox is first implemented with a
tree-walk interpreter written in Java (jlox), and then with a bytecode
interpreter written in C (clox).

I have followed this book before, and am starting on the second half with the
bytecode interpreter. After this I plan on adding some additional features:
* [ ] More native functions to improve Lox's I/O capabilities.
* [ ] Source merging script for bootstrapping preprocessor.
* [ ] Preprocessor for basic C-like modularity.
* [ ] Standard library of useful functions and collections.
* [ ] Implementation of Lox, or another language in Lox.

## Completed Chapters
* [x] 14. Chunks of Bytecode
* [x] 15. A Virtual Machine
* [x] 16. Scanning on Demand
* [x] 17. Compiling Expressions
* [x] 18. Types of Values
* [x] 19. Strings
* [ ] 20. Hash Tables
* [ ] 21. Global Variables
* [ ] 22. Local Variables
* [ ] 23. Jumping Back and Forth
* [ ] 24. Calls and Functions
* [ ] 25. Closures
* [ ] 26. Garbage Collection
* [ ] 27. Classes and Instances
* [ ] 28. Methods and Initializers
* [ ] 29. Superclasses
* [ ] 30. Optimization

# License
This implementation of Lox is released under the MIT License:  
https://krobbi.github.io/license/2024/mit.txt

See [LICENSE.txt](/LICENSE.txt) for a full copy of the license text.

_Crafting Interpreters is copyright of Bob Nystrom._
