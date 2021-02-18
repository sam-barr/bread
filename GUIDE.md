# A Guide to bread

A guide to the `bread` programming language
A guide through the syntax, features, and semantics of the `bread` programming
languages.

# Types

While dynamically typed, every value in `bread` has a type. The type of an expression
can be determined at runtime with the `@typeof` builtin function.

## number

All numbers in `bread` are floating point numbers. Internally the `long double`
C type is used for numbers, so integer arithmetic is still (reasonably) accurate.
Numerical literals are defined by whatever the `strtold` function in the C standard
library accepts, with the caveat that numbers cannot begin with a decimal point
(i.e `.5` is not a valid expression, but `0.5`, `5e-1` and `5E-1` are).

When coerced to a string, numbers become the string representation of the number
(using the `%Lg` formatter). All non-zero numbers are truthy.

## string

Strings in `bread` are immutable ASCII strings. Theoretically a string
can contain any UTF-8 sequence, but indexing operations will behave in undesirable
ways if this is the case. String literals are enclosed in double quotes, with
the standard escape characters for newlines, tabs, and double quotes.

When coerced to a number, `strtold` is used to parse the string as a number.
All non-empty strings are truthy.

## boolean

Boolean values are either `true` or `false`.

When coerced to a number, `true` becomes 1 and `false` becomes 0. When coerced
to a string, `true` and `false` become `"true"` and `"false"`, respectively.

## unit

A `unit` contains no information other than it's own existence.

When coerced to a number, a unit becomes 0.
When coerced to a string, a unit becomes the string `"unit"`.
Unit values are not truthy.

## builtin

`bread` provides several builtin functions for basic IO and other operations.
The identifiers of all builtin functions begin with the `@` symbol.
Bread provides the following builtin functions:

* `@write(args...)` prints the arguments to stdout.
* `@writeln(args...)` prints the arguments to stdout with an additional newline.
* `@readln()` takes no arguments and reads a line from stdin.
* `@length(arg)` reports the length of a string or list
* `@typeof(arg)` reports the type of its argument
* `@system(args...)` runs a shell command, and returns the exit code of the command
* `@push(list, arg)` pushes a value onto the end of a list
* `@insert(list, arg, idx)` inserts a value into a list at a given index

Builtins cannot be coerced into a number. When coerced into a string,
a builtin becomes the name of the builtin (including the "@"). All builtins
are truthy.

## list

A list is in fact an array of non-homogeneous elements. List literals
are comma separated expressions surrounded by a pair of square brackets.

Lists cannot be coerced into a number. When coerced into a string, a list
becomes a string representation of the list. All non-empty lists are truthy.

## closure

`bread` has first class functions in the form of closures which capture
their environment when initialized. Closures are immutable, and are initialized
with the function definition syntax discussed below. If `foo` is a closure,
then `foo` can be called with the syntax `foo(args...)`.

Closures cannot be coerced into a number. When coerced into a string, a closure
becomes the string `"closure"`. All closures are truthy.

## class

Classes are first class values in `bread`. The only class which `bread` initially
provides is the `@Object` class, and all other classes must be defined as a subclass
of `@Object`, using subclass definition syntax discussed below.

Classes cannot be coerced into a number. When coerced into a string, a class
becomes the string `"class"`. All classes are truthy.

## object

Objects are instances of a class and have fields and methods. All fields
of an object are mutable, while the methods are immutable (since classes are
immutable). if `Foo` is a class, then an object of the class `Foo` can
be instantiated with `Foo(args...)`, and these arguments are given to the constructor
of `Foo`.

Objects cannot be coerced into a number. When coerced into a string, an object
becomes the string `"object"`. All objects are truthy.

## method

Methods are closures which also carry a reference to some object. They behave
the same way as closures aside from their type.

Methods cannot be coerced into a number. When coerced into a string, a method
becomes the string `"method"`. All methods are truthy.

# Expressions

Here we discuss the syntax and semantics of some expressions not defined above.

## Arithmetic Operators

When using an arithmetic operator, all operands are coerced into numbers before
performing the arithmetic.
`bread` has the regular arithmetic operators `+, -, *, /, %` with the (hopefully)
expected semantics, associativity, and precedence. Additionally, `bread`
has the integer division operator `//` and the exponentiation operator `^`.
The syntax of `//` is the same as `/`. Exponentiation is right associative
and binds tighter than every other binary operator.

## Boolean Operators

The three boolean operations in `bread` are `and`, `or`, and `not`. If the first
operand of `and` is truthy, then the second operand is evaluated and returned.
Otherwise the first operand is returned. Is the first operand of `or` is truthy,
then the first operand is returned. Otherwise the second operand is evaluated
and returned. If the operand of `not` is truthy, `false` is returned, otherwise
`true` is returned.

## Comparison Operators

The comparison operators in `bread` are `<, <=, =, !=, >, >=`. 
Equality for strings, numbers, booleans, lists, and units works as expected
(there is no epsilon threshold or comparing numbers, so floating point errors
are possible here). Closures, objects, and classes are checked for equality
by reference. Two methods are equal if they contain a reference to the same
object and the same closure. As for checking values of different type for equality,
the following rule set is used:

1. If one of the operands is a number, then try and coerce the other operands to
a number. If it can, check for equality, If it cannot, return `false`.
2. If one of the operands is a string and the other is a boolean, string, or unit,
coerce it to a string and check for equality.
3. If neither of the above hold, return `false`.

As for comparison, for strings and numbers comparison works as expected.
For other types, comparison operators will return false. For operands of different
types, the same rule set as above is used.

## Concatenation Operator

The `..` operator is used to concatenate strings and lists. If both operands
are lists, the concatenation of the lists is returned (neither of the operands
are mutated). If the left operand is a list and the right operand is not a list,
then the result of appending the right onto the left is returned (again, neither
operand is mutated). Otherwise, both operands are coerced into strings and the
concatenation is returned. Concatenation is left associative, and every other
binary operator binds tighter than concatenation.