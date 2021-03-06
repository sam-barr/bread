# bread 🍞

`bread` is an expression based, object oriented, dynamically typed
scripting language with first class closures,
and a garbage-collected interpreter for that language (see GUIDE.md for more
information on using `bread`). The design goals for `bread` are as follows:

* Get a good grade on my final project in my programming languages class

## Building bread

`bread` has no dependencies outside of the C standard library, so simply run

```
make release
```

to create an optimized build. The generated binary will be ./release/bread.
To install `bread`, run

```
make install
```

By default `bread` will be installed in ~/.local/bin/bread. To run a `bread` file,
the command is

```
bread file.brd
```

To start a REPL, just run `bread` with no arguments. You can run
`bread --help` for more detailed usage information.

## Acknowledgements

[Crafting Interpreters](https://craftinginterpreters.com/) for reference/inspiration
in implementing various parts of the interpreter, in particular calling convention
for closures and the garbage collector.

The [Lua](https://www.lua.org/) programming language, for inspiration on syntax
and some of the bytecode operations for the virtual machine.
