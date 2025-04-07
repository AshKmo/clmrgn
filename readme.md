# The clmrgn programming language and interpreter
This is an interpreter for a new programming language called clmrgn written in C. Its main goal is to be a new programming language that has an interpreter written in C.

## How to use
Build the interpreter from source by running `make`.

You should now be able to run `./bin/clmrgn <script>.clmrgn` to evaluate a clmrgn script file.

To evaluate the pi calculation example script, run `./bin/clmrgn examples/pi.clmrgn`.

## Docco
### Sequence commands
A clmrgn script starts as a Sequence, which contains a series of statements starting with a command:

```
let x "Hello, world!";
print x;
```

Each statement must start with a command name end with a semicolon. The list of valid commands are as follows:

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
