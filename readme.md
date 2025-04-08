# The clmrgn programming language and interpreter
This is an interpreter for a new programming language called clmrgn written in C. Its main goal is to be a new programming language that has an interpreter written in C.

## How to use
Build the interpreter from source by running `make`.

You should now be able to run `./bin/clmrgn <script>.clmrgn` to evaluate a clmrgn script file.

To evaluate the pi calculation example script, run `./bin/clmrgn examples/pi.clmrgn`.

## Docco

### Data types
An element can be one of the following types:
- Null (`?`): a single value that is different from any other value of any other type
- String (`"Hello, world!"`): a string of characters
- Number (`-12.4`): a number stored as a long int or a double
- Scope (`{let "three" 3;}`): a list of key/value pairs
- Closure (`(x => x * 4)`): a function that contains the scopes in the code surrounding it

### Sequence commands
A clmrgn script starts as a Sequence, which contains a series of statements starting with a command:

```
let x "Hello, world!";
print x;
```

Each statement must start with a command name end with a semicolon. You may pass whitespace-separated arguments to a command, but keep in mind that any operations or other non-trivial expressions must be contained within parentheses to count as a single argument. By default, a Sequence, when evaluated, will return the Scope object that it generates to store its properties and variables in, although this can be overridden with certain commands.

A Sequence can also be used as part of an expression by enclosing it in braces.

The list of valid commands is as follows:

#### `do`
Accepts any number of expressions and evaluates all of them.

#### `write`
Accepts any number of expressions and writes the results to the console, separated by spaces. Anything of a String type has its contents printed instead.

#### `print`
Accepts any number of expressions and writes the results to the console, separated by spaces and followed by a newline. Anything of a String type has its contents printed instead.

#### `show`
Accepts any number of expressions and writes the results to the console, separated by spaces.

#### `display`
Accepts any number of expressions and writes the results to the console, separated by spaces and followed by a newline.

#### `return`
Exits the current Sequence and causes the Sequence's evaluation to return the specified value. Accepts only one expression.

#### `eval`
Accepts only one expression that evaluates to a String. The String will then be interpreted as a clmrgn script that is then evaluated. Afterward, the current Sequence is exited and the Sequence's evaluation will return the value returned by the evaluation of the new script.

#### `function`
Accepts at least three arguments:
- The name of a new closure to create
- The names of any arguments that are to be passed to the closure
- An expression to evaluate using arguments passed to the closure
This command then adds the function to the current scope under the specified name.

#### `let`
Accepts two arguments of any type. The current scope will then be modified such that accessing said scope with an element equal to the first argument will return the second argument.

#### `set`
Accepts two arguments of any type. The relevant scope will then be modified such that accessing said scope with an element equal to the first argument will return the second argument.

#### `selfmut`
Accepts two arguments of any type. The relevant scope will then be modified such that accessing said scope with an element equal to the evaluation of the first argument will return the evaluation of the second argument.

#### `mut`
Accepts three arguments:
- An expression evaluating to a Scope object to mutate
- An expression evaluating to an element of any type
- An expression evaluating to an element of any type

The specified Scope object will then be modified such that accessing said scope with an element equal to the evaluation of the first argument will return the evaluation of the second argument

#### `setprop`
Accepts three arguments:
- An expression evaluating to a Scope object to mutate
- An expression of any type
- An expression evaluating to an element of any type

The specified Scope object will then be modified such that accessing said scope with an element equal to the first argument will return the evaluation of the second argument

#### `del`
Accepts an argument of type Scope from which the key specified in the evaluation of the second argument will be removed.

#### `delprop`
Accepts an argument of type Scope from which the key specified in the second argument will be removed.

#### `if`
Accepts any number of arguments. Consider the first set of two arguments. If the first argument evaluates to a truthy element, the second argument will be evaluated. Otherwise, the next set of two arguments will be considered. If there is a remaining argument that is not in a pair, it will be evaluated if all pairs have their first argument evaluate to a falsy value.

#### `while`
Accepts two arguments. The first argument will be evaluated and, if it evaluates to a truthy value, the second argument will be evaluated. This will repeat until the first argument evaluates to a falsy value.

#### `each`
Accepts two arguments. The second argument will be applied to all non-property-like keys in the Scope to which the first argument should evaluate

#### `bruh`
Accepts one argument that evaluates to a String. The interpreter will print the contents of this string to the console and immediately exit with code 1, leaving the OS to deal with any rubbish it leaves in memory.

### Operators
All operators are infix except for application by juxtaposition, which, as the name suggests, is applied by juxtaposition.

#### Application by juxtaposition
Two values can be juxtaposed to apply the second to the first. The result depends on the type of the evaluation of the first argument:
- If it is a Closure, the Closure is evaluated with its variable set to the evaluation of the second argument
- If it is a Number, the Number is multiplied with the Number returned by the evaluation of the second argument
- If it is a Scope, the value associated with the key matching the evaluation of the second argument is returned

#### `$`
Does the same as application by juxtaposition but is an infix operator with a lower precedence.

#### `.`
Returns the value associated with the key matching the second argument in the Scope that the first argument evaluates to.

#### `**`
Raises the Number that the first argument evaluates to, to the power of the Number that the second argument evaluates to.

#### `*`, `/`, `%`, `+`, `-`, `<<`, `>>`
Pretty obvious, except that `-` must have two operands.

#### `</`
Returns the portion of the String that the first argument evaluates to, that contains the first (n) characters, where 'n' is the value of the integer Number returned by the evaluation of the second argument.

#### `>/`
Returns the portion of the String that the first argument evaluates to, that does not contain the first (n-1) characters, where 'n' is the value of the integer Number returned by the evaluation of the second argument.

#### `..`
Concatenates the evaluation of the two arguments, which should both be Strings.

#### `<`, `>`, `<=`, `>=`, `==`, `!=`, `&`, `|`, `^`
Pretty obvious, although keep in mind that `&`, `|` and `^` are bitwise operators.

#### `=>`
Returns a Closure that contains the expression specified in the second argument and that accepts an argument that can be accessed using the key specified in the first argument.
