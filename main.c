#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <ctype.h>

typedef struct Stack {
	size_t length;
	void *content[];
} Stack;

Stack* Stack_new() {
	Stack *s = malloc(sizeof(Stack));
	s->length = 0;
	return s;
}

Stack* Stack_push(Stack *s, void *e) {
	s = realloc(s, sizeof(Stack) + (s->length + 1) * sizeof(void*));
	s->content[s->length] = e;
	s->length++;
	return s;
}

Stack* Stack_pop(Stack *s) {
	s->length--;
	s = realloc(s, sizeof(Stack) + s->length * sizeof(void*));
	return s;
}

Stack* Stack_delete(Stack *s, size_t i) {
	i++;
	for (; i < s->length; i++) {
		s->content[i - 1] = s->content[i];
	}

	s->length--;
	s = realloc(s, sizeof(Stack) + s->length * sizeof(void*));
	return s;
}

typedef struct String {
	size_t length;
	char content[];
} String;

String* String_new(size_t length) {
	String *s = malloc(sizeof(String) + length * sizeof(char));
	s->length = length;
	return s;
}

String* String_append_char(String *s, char c) {
	s = realloc(s, sizeof(String) + (s->length + 1) * sizeof(char));
	s->content[s->length] = c;
	s->length++;
	return s;
}

void String_print(String *s) {
	for (size_t i = 0; i < s->length; i++) {
		putchar(s->content[i]);
	}
}

bool String_has_char(String *s, char c) {
	for (size_t i = 0; i < s->length; i++) {
		if (s->content[i] == c) {
			return true;
		}
	}

	return false;
}

bool String_is(String *s, char *c) {
	size_t i = 0;
	for (; i < s->length; i++) {
		if (c[i] == '\0' || c[i] != s->content[i]) {
			return false;
		}
	}

	if (c[i] != '\0') {
		return false;
	}

	return true;
}

typedef struct Number {
	bool is_double;
	union {
		long value_long;
		double value_double;
	};
} Number;

Number* Number_new() {
	Number *n = malloc(sizeof(Number));
	n->is_double = false;
	n->value_long = 0;
	return n;
}

// TODO: add functions
typedef enum {
	ELEMENT_NOTHING,
	ELEMENT_NULL,
	ELEMENT_VARIABLE,
	ELEMENT_STRING,
	ELEMENT_NUMBER,
	ELEMENT_BRACKET,
	ELEMENT_TERMINATOR,
	ELEMENT_BRACE,

	ELEMENT_OPERATION,
	ELEMENT_SEQUENCE,

	ELEMENT_SCOPE,
	ELEMENT_FUNCTION,
} ElementType;

typedef struct Element {
	ElementType type;
	bool gc_checked;
	bool heaper_checked;
	void *value;
} Element;

Element* Element_new(ElementType type, void *value) {
	Element *e = malloc(sizeof(Element));
	e->type = type;
	e->gc_checked = false;
	e->heaper_checked = false;
	e->value = value;
	return e;
}

/*
 * TO ADD NEW OPERATIONS:
 * - add a new OperationType value
 * - set its precedence in OPERATION_PRECEDENCE
 * - add a case in Element_print
 * - add cases in tokenise:
 *   - any unhandled characters in the operator must be treated as operator characters
 *   - the operator must be identified after the token is complete
 * - add its behaviour in Element_evaluate
 */

// TODO: add more operators
typedef enum {
	OPERATION_APPLICATION,
	OPERATION_MULTIPLICATION,
	OPERATION_DIVISION,
	OPERATION_ADDITION,
	OPERATION_SUBTRACTION,
	OPERATION_CONCATENATION,
	OPERATION_EQUALITY,
} OperationType;

const int OPERATION_PRECEDENCE[] = {0, 1, 1, 2, 2, 3, 4};

typedef struct Operation {
	OperationType type;
	Element *a;
	Element *b;
} Operation;

Operation* Operation_new(OperationType type, Element *a, Element *b) {
	Operation *o = malloc(sizeof(Operation));
	o->type = type;
	o->a = a;
	o->b = b;
	return o;
}

typedef struct Map {
	Element *key;
	Element *value;
} Map;

typedef struct Scope {
	size_t length;
	Map maps[];
} Scope;

Scope* Scope_new() {
	Scope *s = malloc(sizeof(Scope));
	s->length = 0;
	return s;
}

typedef struct Function {
	Stack *internal_scopes;
	Element *variable;
	Element *expression;
} Function;

Function* Function_new(Stack *internal_scopes, Element *variable, Element *expression) {
	Function *f = malloc(sizeof(Function));
	f->internal_scopes = internal_scopes;
	f->variable = variable;
	f->expression = expression;
	return f;
}

bool Element_compare(Element *a, Element *b) {
	if (a->type != b->type) {
		return false;
	}

	switch (a->type) {
		case ELEMENT_NULL:
			return true;
		case ELEMENT_VARIABLE:
		case ELEMENT_STRING:
			{
				String *sa = a->value;
				String *sb = b->value;

				if (sa->length != sb->length) {
					return false;
				}

				for (size_t i = 0; i < sa->length; i++) {
					if (sa->content[i] != sb->content[i]) {
						return false;
					}
				}

				return true;
			};
		case ELEMENT_NUMBER:
			{
				Number *na = a->value;
				Number *nb = b->value;

				if (na->is_double) {
					if (nb->is_double) {
						return na->value_double == nb->value_double;
					} else {
						return na->value_double == nb->value_long;
					}
				} else {
					if (nb->is_double) {
						return na->value_long == nb->value_double;
					} else {
						return na->value_long == nb->value_long;
					}
				}
			};
	}

	return a == b;
}

bool Element_is_truthy(Element *e) {
	if (e->type == ELEMENT_NULL) {
		return false;
	}

	if (e->type == ELEMENT_NUMBER) {
		Number *n = e->value;

		if (n->is_double) {
			return n->value_double != 0 && n->value_double != NAN;
		} else {
			return n->value_long != 0;
		}
	}
	
	return true;
}

Scope* Scope_set(Scope *s, Element *key, Element *value) {
	for (size_t i = 0; i < s->length; i++) {
		if (Element_compare(s->maps[i].key, key)) {
			s->maps[i].value = value;
			return s;
		}
	}

	s = realloc(s, sizeof(Scope) + (s->length + 1) * sizeof(Map));
	s->maps[s->length].key = key;
	s->maps[s->length].value = value;
	s->length++;

	return s;
}

Element* Scope_get(Scope *s, Element *key) {
	for (size_t i = 0; i < s->length; i++) {
		if (Element_compare(s->maps[i].key, key)) {
			return s->maps[i].value;
		}
	}

	return NULL;
}

bool Scope_has(Scope *s, Element *key) {
	for (size_t i = 0; i < s->length; i++) {
		if (Element_compare(s->maps[i].key, key)) {
			return true;
		}
	}

	return false;
}

Scope* Scope_delete(Scope *s, Element *key) {
	bool shift_back = false;

	for (size_t i = 0; i < s->length; i++) {
		if (shift_back) {
			s->maps[i - 1].key = s->maps[i].value;
			s->maps[i - 1].value = s->maps[i].value;
			continue;
		}

		if (Element_compare(s->maps[i].key, key)) {
			shift_back = true;
		}
	}

	s->length--;
	s = realloc(s, sizeof(Scope) + s->length * sizeof(Map));

	return s;
}

void Element_print(Element *e) {
	switch (e->type) {
		case ELEMENT_NUMBER:
			{
				Number *n = e->value;
				if (n->is_double) {
					printf("%.*g", DBL_DECIMAL_DIG, n->value_double);
				} else {
					printf("%d", n->value_long);
				}
				break;
			};
		case ELEMENT_OPERATION:
			{
				Operation *o = e->value;
				if (o->a != NULL) {
					putchar('(');
					Element_print(o->a);
					putchar(' ');
				}
				switch (o->type) {
					case OPERATION_APPLICATION:
						break;
					case OPERATION_MULTIPLICATION:
						putchar('*');
						break;
					case OPERATION_DIVISION:
						putchar('/');
						break;
					case OPERATION_ADDITION:
						putchar('+');
						break;
					case OPERATION_SUBTRACTION:
						putchar('-');
						break;
					case OPERATION_CONCATENATION:
						fputs("..", stdout);
						break;
					case OPERATION_EQUALITY:
						fputs("==", stdout);
						break;
				}
				if (o->b != NULL) {
					putchar(' ');
					Element_print(o->b);
					putchar(')');
				}
			};
			break;
		case ELEMENT_SEQUENCE:
			{
				Stack *sequence = e->value;

				putchar('{');
				for (size_t y = 0; y < sequence->length; y++) {
					Stack *statement = sequence->content[y];

					for (size_t x = 0; x < statement->length; x++) {
						Element_print(statement->content[x]);
					}

					putchar(';');
				}
				putchar('}');

				break;
			};
		case ELEMENT_VARIABLE:
			String_print(e->value);
			putchar(' ');
			break;
		case ELEMENT_STRING:
			putchar('"');
			{
				String *s = e->value;
				for (size_t i = 0; i < s->length; i++) {
					if (s->content[i] == '"') {
						putchar('\\');
					}

					putchar(s->content[i]);
				}
			};
			putchar('"');
			break;
		case ELEMENT_NULL:
			putchar('?');
			break;
		case ELEMENT_BRACKET:
			putchar((uintptr_t)(e->value) ? ')' : '(');
			break;
		case ELEMENT_BRACE:
			putchar((uintptr_t)(e->value) ? '}' : '{');
			break;
		case ELEMENT_TERMINATOR:
			putchar(';');
			break;
		case ELEMENT_SCOPE:
			puts("{...}");
			break;
		case ELEMENT_FUNCTION:
			puts("[FUNCTION]");
			break;
	}
}

void Element_nuke(Element *e) {
	if (e == NULL || e->gc_checked) {
		return;
	}

	switch (e->type) {
		case ELEMENT_VARIABLE:
		case ELEMENT_NUMBER:
		case ELEMENT_STRING:
		case ELEMENT_SCOPE:
			free(e->value);
			break;
		case ELEMENT_FUNCTION:
			{
				Function *f = e->value;
				free(f->internal_scopes);
				free(f);
			};
			break;
		case ELEMENT_OPERATION:
			{
				Operation *o = e->value;
				Element_nuke(o->a);
				Element_nuke(o->b);
				free(o);
			};
			break;
		case ELEMENT_SEQUENCE:
			{
				Stack *sequence = e->value;

				for (size_t y = 0; y < sequence->length; y++) {
					Stack *statement = sequence->content[y];

					for (size_t x = 0; x < statement->length; x++) {
						Element_nuke(statement->content[x]);
					}

					free(statement);
				}

				free(sequence);
			};
			break;
	}

	free(e);
}

void garbage_check(Element *e, bool gc_check) {
	if (e == NULL) {
		return;
	}

	if (e->gc_checked && gc_check) {
		return;
	}

	e->gc_checked = gc_check;

	switch (e->type) {
		case ELEMENT_FUNCTION:
			{
				Function *f = e->value;

				for (size_t i = 0; i < f->internal_scopes->length; i++) {
					garbage_check(f->internal_scopes->content[i], gc_check);
				}

				garbage_check(f->variable, gc_check);
				garbage_check(f->expression, gc_check);
			};
			break;
		case ELEMENT_SCOPE:
			{
				Scope *s = e->value;
				for (size_t i = 0; i < s->length; i++) {
					garbage_check(s->maps[i].key, gc_check);
					garbage_check(s->maps[i].value, gc_check);
				}
			};
			break;
		case ELEMENT_OPERATION:
			{
				Operation *o = e->value;
				garbage_check(o->a, gc_check);
				garbage_check(o->b, gc_check);
			};
			break;
		case ELEMENT_SEQUENCE:
			{
				Stack *sequence = e->value;
				for (size_t y = 0; y < sequence->length; y++) {
					Stack *statement = sequence->content[y];

					for (size_t x = 0; x < statement->length; x++) {
						garbage_check(statement->content[x], gc_check);
					}
				}
			};
			break;
	}
}

void garbage_collect(Element *return_value, Stack **scopes, Stack **heap) {
	garbage_check(return_value, true);

	if (scopes != NULL) {
		for (size_t i = 0; i < (*scopes)->length; i++) {
			garbage_check((*scopes)->content[i], true);
		}
	}

	for (size_t i = 0; i < (*heap)->length; i++) {
		Element *e = (*heap)->content[i];

		if (e->gc_checked) {
			e->gc_checked = false;
		} else {
			Element_nuke(e);
			*heap = Stack_delete(*heap, i);
		}
	}
}

Element* heap_it(Element *e, Stack **heap) {
	*heap = Stack_push(*heap, e);
	return e;
}

Element* heaper(Element *e, Stack **heap) {
	if (e == NULL || e->heaper_checked) {
		return NULL;
	}

	if (heap != NULL) {
		for (size_t i = 0; i < (*heap)->length; i++) {
			if ((*heap)->content[i] == e) {
				return NULL;
			}
		}

		heap_it(e, heap);
	}

	e->heaper_checked = true;
	e->gc_checked = false;

	switch (e->type) {
		case ELEMENT_SCOPE:
			{
				Scope *s = e->value;

				for (size_t i = 0; i < s->length; i++) {
					heaper(s->maps[i].key, heap);
					heaper(s->maps[i].value, heap);
				}
			};
			break;
		case ELEMENT_FUNCTION:
			{
				Function *f = e->value;

				for (size_t i = 0; i < f->internal_scopes->length; i++) {
					heaper(f->internal_scopes->content[i], heap);
				}

				heaper(f->variable, heap);
				heaper(f->expression, heap);
			};
			break;
	}

	e->heaper_checked = false;

	return e;
}

void Element_discard(Element *e) {
	Stack *to_be_discarded = Stack_new();

	heaper(e, &to_be_discarded);

	for (size_t i = 0; i < to_be_discarded->length; i++) {
		Element *e = to_be_discarded->content[i];
		Element_nuke(e);
	}

	free(to_be_discarded);
}

Element* make(Stack **heap, ElementType type, void *value) {
	return heap_it(Element_new(type, value), heap);
}

void bruh(char *e) {
	fputs("ERROR: ", stderr);
	fputs(e, stderr);
	fputc('\n', stderr);
	exit(1);
}

Element* fetch(Stack **scopes, Element *key) {
	for (size_t i = 0; i < (*scopes)->length; i++) {
		Element *scope_element = (*scopes)->content[(*scopes)->length - 1 - i];

		Element *result = Scope_get(scope_element->value, key);

		if (result != NULL) {
			return result;
		}
	}

	return NULL;
}

void mutate(Stack **scopes, Element *key, Element *value, bool local_only) {
	Element *scope_element;

	size_t i = 0;
	if (!local_only) {
		for (; i < (*scopes)->length; i++) {
			scope_element = (*scopes)->content[(*scopes)->length - 1 - i];

			if (Scope_has(scope_element->value, key)) {
				break;
			}
		}
	}

	if (local_only || i == (*scopes)->length) {
		scope_element = (*scopes)->content[(*scopes)->length - 1];
	}

	scope_element->value = Scope_set(scope_element->value, key, value);
}

Number* Number_operate(OperationType ot, Number *na, Number *nb) {
	Number *result = Number_new();
	result->is_double = na->is_double || nb->is_double;

	switch (ot) {
		case OPERATION_ADDITION:
			if (result->is_double) {
				result->value_double =
					(na->is_double ? na->value_double : na->value_long) +
					(nb->is_double ? nb->value_double : nb->value_long);
			} else {
				result->value_long = na->value_long + nb->value_long;
			}
			break;
		case OPERATION_SUBTRACTION:
			if (result->is_double) {
				result->value_double =
					(na->is_double ? na->value_double : na->value_long) -
					(nb->is_double ? nb->value_double : nb->value_long);
			} else {
				result->value_long = na->value_long - nb->value_long;
			}
			break;
		case OPERATION_MULTIPLICATION:
			if (result->is_double) {
				result->value_double =
					(na->is_double ? na->value_double : na->value_long) *
					(nb->is_double ? nb->value_double : nb->value_long);
			} else {
				result->value_long = na->value_long * nb->value_long;
			}
			break;
		case OPERATION_DIVISION:
			if (!result->is_double && (
						nb->value_long == 0 ||
						na->value_long % nb->value_long != 0
						)) {
				result->is_double = true;
			}

			if (result->is_double) {
				double da = na->is_double ? na->value_double : na->value_long;
				double db = nb->is_double ? nb->value_double : nb->value_long;

				result->value_double = db == 0 ?
					(da == 0 ? NAN : da > 0 ? INFINITY : -INFINITY) :
					da / db;
			} else {
				result->value_long = na->value_long / nb->value_long;
			}
			break;
	}

	return result;
}

Element* eval(String *script);

Element* Element_evaluate(Element *e, Stack **scopes, Stack **heap) {
	switch (e->type) {
		case ELEMENT_SEQUENCE:
			{
				*scopes = Stack_push(*scopes, make(heap, ELEMENT_SCOPE, Scope_new()));

				Stack *sequence = e->value;

				for (size_t i = 0; i < sequence->length; i++) {
					Stack *statement = sequence->content[i];

					String *command;
					{
						Element *first = statement->content[0];
						if (first->type != ELEMENT_VARIABLE) {
							bruh("command names are not dynamic");
						}
						command = first->value;
					};

					if (String_is(command, "return")) {
						if (statement->length != 2) {
							bruh("'return' command accepts 1 argument");
						}

						Element *result = Element_evaluate(statement->content[1], scopes, heap);

						*scopes = Stack_pop(*scopes);

						garbage_collect(result, scopes, heap);

						return result;
					}

					if (String_is(command, "read")) {
						if (statement->length != 1) {
							bruh("'read' command does not accept arguments");
						}

						Element *result;
						{
							char *input = NULL;
							size_t allocated_length = 0;

							size_t length = getline(&input, &allocated_length, stdin) - 1;

							String *s = String_new(length);
							memcpy(&(s->content), input, length);

							free(input);

							result = make(heap, ELEMENT_STRING, s);
						}

						*scopes = Stack_pop(*scopes);

						garbage_collect(result, scopes, heap);

						return result;
					}

					if (String_is(command, "eval")) {
						if (statement->length != 2) {
							bruh("'eval' command accepts 1 argument");
						}

						Element *expression = Element_evaluate(statement->content[1], scopes, heap);

						Element *result = heaper(eval(expression->value), heap);

						*scopes = Stack_pop(*scopes);

						garbage_collect(result, scopes, heap);

						return result;
					}

					if (String_is(command, "do")) {
						for (size_t i = 1; i < statement->length; i++) {
							Element_evaluate(statement->content[i], scopes, heap);
						}
					}

					if (String_is(command, "show")) {
						for (size_t i = 1; i < statement->length; i++) {
							Element_print(Element_evaluate(statement->content[i], scopes, heap));

							if (i < statement->length - 1) {
								putchar(' ');
							}
						}
						putchar('\n');
					}

					if (String_is(command, "print") || String_is(command, "write")) {
						for (size_t i = 1; i < statement->length; i++) {
							Element *to_print = Element_evaluate(statement->content[i], scopes, heap);
							
							if (to_print->type == ELEMENT_STRING) {
								String_print(to_print->value);
							} else {
								Element_print(to_print);
							}

							if (i < statement->length - 1) {
								putchar(' ');
							}
						}

						if (String_is(command, "print")) {
							putchar('\n');
						}
					}

					if (String_is(command, "let")) {
						if (statement->length != 3) {
							bruh("'let' command accepts 2 arguments");
						}

						Element *variable = statement->content[1];

						if (variable->type != ELEMENT_VARIABLE) {
							variable = Element_evaluate(variable, scopes, heap);

							if (variable->type != ELEMENT_STRING && variable->type != ELEMENT_NUMBER) {
								bruh("argument 1 of 'let' command must be a variable name, a string or a number");
							}
						}

						Element *value = Element_evaluate(statement->content[2], scopes, heap);

						mutate(scopes, variable, value, true);
					}

					if (String_is(command, "set")) {
						if (statement->length != 3) {
							bruh("'set' command accepts 2 arguments");
						}

						Element *variable = statement->content[1];

						if (variable->type != ELEMENT_VARIABLE) {
							variable = Element_evaluate(variable, scopes, heap);

							if (variable->type != ELEMENT_STRING && variable->type != ELEMENT_NUMBER) {
								bruh("argument 1 of 'set' command must be a variable name, a string or a number");
							}
						}

						Element *value = Element_evaluate(statement->content[2], scopes, heap);

						mutate(scopes, variable, value, false);
					}

					if (String_is(command, "mut")) {
						if (statement->length != 4) {
							bruh("'mut' command accepts 3 arguments");
						}

						Element *subject = Element_evaluate(statement->content[1], scopes, heap);

						Element *key = Element_evaluate(statement->content[2], scopes, heap);

						Element *value = Element_evaluate(statement->content[3], scopes, heap);

						subject->value = Scope_set(subject->value, key, value);
					}

					if (String_is(command, "function")) {
						if (statement->length < 4) {
							bruh("'function' command accepts at least 3 arguments");
						}

						Element *f = statement->content[statement->length - 1];

						for (size_t i = 2; i < statement->length - 1; i++) {
							Stack *internal_scopes = Stack_new();

							for (size_t i = 0; i < (*scopes)->length; i++) {
								internal_scopes = Stack_push(internal_scopes, (*scopes)->content[i]);
							}

							Element *variable = statement->content[i];

							if (variable->type != ELEMENT_VARIABLE) {
								bruh("function variables must be specified using variable names");
							}

							f = make(heap, ELEMENT_FUNCTION, Function_new(
										internal_scopes,
										variable,
										f
										));
						}

						mutate(scopes, statement->content[1], f, true);
					}

					if (String_is(command, "while")) {
						if (statement->length != 3) {
							bruh("'while' command accepts 2 arguments");
						}

						while (Element_is_truthy(Element_evaluate(statement->content[1], scopes, heap))) {
							Element_evaluate(statement->content[2], scopes, heap);
						}
					}

					if (String_is(command, "if")) {
						if (statement->length != 3 && statement->length != 4) {
							bruh("'if' command accepts either 2 or 3 arguments");
						}

						if (Element_is_truthy(Element_evaluate(statement->content[1], scopes, heap))) {
							Element_evaluate(statement->content[2], scopes, heap);
						} else if (statement->length == 4) {
							Element_evaluate(statement->content[3], scopes, heap);
						}
					}

					garbage_collect(NULL, scopes, heap);
				}

				Element *result = (*scopes)->content[(*scopes)->length - 1];

				*scopes = Stack_pop(*scopes);

				garbage_collect(result, scopes, heap);

				return result;
			};
			break;
		case ELEMENT_OPERATION:
			{
				Operation *o = e->value;

				Element *a = Element_evaluate(o->a, scopes, heap);
				Element *b = Element_evaluate(o->b, scopes, heap);

				switch (o->type) {
					case OPERATION_APPLICATION:
						switch (a->type) {
							case ELEMENT_NUMBER:
								{
									if (b->type != ELEMENT_NUMBER) {
										bruh("cannot implicitly multiply this type");
									}
									return make(heap, ELEMENT_NUMBER,
											Number_operate(OPERATION_MULTIPLICATION, a->value, b->value)
											);
								};
							case ELEMENT_SCOPE:
								{
									Element *result = Scope_get(a->value, b);

									if (result == NULL) {
										bruh("key does not exist in this Scope object");
									}

									return result;
								};
							case ELEMENT_FUNCTION:
								{
									Function *f = e->value;

									*scopes = Stack_push(*scopes, make(heap, ELEMENT_SCOPE, Scope_new()));

									mutate(scopes, f->variable, b, true);

									Element *result = Element_evaluate(f->expression, scopes, heap);

									*scopes = Stack_pop(*scopes);

									garbage_collect(result, scopes, heap);

									return result;
								};
							default:
								bruh("application cannot be applied to this type");
						}
						break;
					case OPERATION_CONCATENATION:
						{
							if (a->type != ELEMENT_STRING || b->type != ELEMENT_STRING) {
								bruh("string concatenation can only be applied to strings");
							}

							String *sa = a->value;
							String *sb = b->value;

							String *result = String_new(sa->length + sb->length);

							for (size_t i = 0; i < sa->length + sb->length; i++) {
								result->content[i] = i >= sa->length ?
									sb->content[i - sa->length] :
									sa->content[i];
							}

							return make(heap, ELEMENT_STRING, result);
						};
						break;
					case OPERATION_ADDITION:
					case OPERATION_SUBTRACTION:
					case OPERATION_MULTIPLICATION:
					case OPERATION_DIVISION:
						if (a->type != ELEMENT_NUMBER || b->type != ELEMENT_NUMBER) {
							bruh("numeric operations cannot be applied to this type");
						}
						return make(heap, ELEMENT_NUMBER, Number_operate(o->type, a->value, b->value));
					case OPERATION_EQUALITY:
						{
							Number *n = Number_new();
							n->is_double = false;
							n->value_long = Element_compare(a, b) ? 1 : 0;
							return make(heap, ELEMENT_NUMBER, n);
						};
						break;
				}
			};
			break;
		case ELEMENT_VARIABLE:
			{
				Element *value = fetch(scopes, e);

				if (value != NULL) {
					return value;
				}

				bruh("no such variable");
			};
			break;
	}

	return e;
}

Element* operatify(Stack *expression, size_t start, size_t end) {
	if (end - start == 1) {
		return expression->content[start];
	}

	Element *final_element = NULL;

	Operation *final_operation = NULL;

	int op_precedence = 0;
	size_t op_location = 0;
	for (size_t p = start; p < end; p++) {
		Element *e = expression->content[p];

		if (e->type != ELEMENT_OPERATION) {
			continue;
		}

		Operation *o = e->value;

		if (o->a != NULL) {
			continue;
		}

		if (OPERATION_PRECEDENCE[o->type] >= op_precedence) {
			final_element = e;
			final_operation = o;
			op_precedence = OPERATION_PRECEDENCE[o->type];
			op_location = p;
		}
	}

	if (final_operation == NULL) {
		return Element_new(ELEMENT_OPERATION, Operation_new(
				OPERATION_APPLICATION,
				operatify(expression, start, end - 1),
				expression->content[end - 1]
				));
	}

	final_operation->a = operatify(expression, start, op_location);
	final_operation->b = operatify(expression, op_location + 1, end);

	return final_element;
}

Element* elementify_sequence(Stack*, size_t*);

Element* elementify_expression(Stack *tokens, size_t *i) {
	Stack *expression = Stack_new();

	for (; *i < tokens->length; (*i)++) {
		Element *t = tokens->content[*i];

		if (t->type == ELEMENT_BRACKET && (uintptr_t)(t->value)) {
			free(t);
			break;
		}

		switch (t->type) {
			case ELEMENT_BRACE:
				free(t);
				(*i)++;
				expression = Stack_push(expression, elementify_sequence(tokens, i));
				break;
			case ELEMENT_BRACKET:
				free(t);
				(*i)++;
				expression = Stack_push(expression, elementify_expression(tokens, i));
				break;
			default:
				expression = Stack_push(expression, t);
				break;
		}
	}

	Element *result = operatify(expression, 0, expression->length);

	free(expression);

	return result;
}

Element* elementify_sequence(Stack *tokens, size_t *i) {
	Stack *sequence = Stack_new();

	Stack *statement = Stack_new();

	for (; *i < tokens->length; (*i)++) {
		Element *t = tokens->content[*i];

		if (t->type == ELEMENT_BRACE && (uintptr_t)(t->value)) {
			free(t);
			break;
		}

		switch (t->type) {
			case ELEMENT_BRACE:
				free(t);
				(*i)++;
				statement = Stack_push(statement, elementify_sequence(tokens, i));
				break;
			case ELEMENT_BRACKET:
				free(t);
				(*i)++;
				statement = Stack_push(statement, elementify_expression(tokens, i));
				break;
			case ELEMENT_TERMINATOR:
				free(t);

				if (statement->length < 0) {
					break;
				}

				sequence = Stack_push(sequence, statement);

				statement = Stack_new();
				break;
			default:
				statement = Stack_push(statement, tokens->content[*i]);
				break;
		}
	}

	free(statement);

	return Element_new(ELEMENT_SEQUENCE, sequence);
}

char hex_char(char c) {
	if (c >= 97) {
		return c - 87;
	}

	if (c >= 65) {
		return c - 55;
	}

	return c - 48;
}

Stack* tokenise(String *script) {
	Stack *tokens = Stack_new();

	ElementType current_type;
	String *current_value = String_new(0);

	bool escaped = false;
	bool comment = false;
	bool in_string = false;

	for (size_t i = 0; i <= script->length; i++) {
		ElementType new_type;

		char c;

		if (i == script->length) {
			c = '\n';
		} else {
			c = script->content[i];
		}

		if (c == '\\' && !escaped) {
			escaped = true;
			continue;
		}

		if (c == '#' && !escaped) {
			comment = !comment;
			continue;
		}

		if (comment) {
			continue;
		}

		if (escaped || in_string) {
			new_type = ELEMENT_VARIABLE;

			if (in_string) {
				new_type = ELEMENT_STRING;

				if (escaped) {
					switch (c) {
						case 'n':
							c = '\n';
							break;
						case 'r':
							c = '\r';
							break;
						case 't':
							c = '\t';
							break;
						case 'x':
							c = 16 * hex_char(script->content[i + 1]) + hex_char(script->content[i + 2]);
							i += 2;
							break;
					}
				} else if (c == '"') {
					new_type = ELEMENT_NOTHING;
				}
			}
		} else {
			switch (c) {
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					new_type = ELEMENT_NOTHING;
					break;
				case ';':
					new_type = ELEMENT_TERMINATOR;
					break;
				case '(':
				case ')':
					new_type = ELEMENT_BRACKET;
					break;
				case '{':
				case '}':
					new_type = ELEMENT_BRACE;
					break;
				case '"':
					new_type = ELEMENT_STRING;
					break;
				case '+':
				case '*':
				case '/':
				case '%':
				case '=':
					new_type = ELEMENT_OPERATION;
					break;
				case '.':
					if (current_type != ELEMENT_NUMBER) {
						new_type = ELEMENT_OPERATION;
					}
					break;
				case '-':
					if (i < script->length - 1 && isdigit(script->content[i + 1])) {
						new_type = ELEMENT_NUMBER;
					} else {
						new_type = ELEMENT_OPERATION;
					}
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (current_type != ELEMENT_VARIABLE) {
						new_type = ELEMENT_NUMBER;
					}
					break;
				case '?':
					new_type = ELEMENT_NULL;
					break;
				default:
					new_type = ELEMENT_VARIABLE;
					break;
			}
		}

		if ((current_value->length > 0 || in_string) && (new_type != current_type ||
					current_type == ELEMENT_BRACKET ||
					current_type == ELEMENT_BRACE ||
					current_type == ELEMENT_TERMINATOR
					)) {

			Element *new_token = Element_new(current_type, NULL);

			switch (current_type) {
				case ELEMENT_NULL:
				case ELEMENT_TERMINATOR:
					free(current_value);
					break;
				case ELEMENT_VARIABLE:
				case ELEMENT_STRING:
					new_token->value = current_value;
					break;
				case ELEMENT_BRACKET:
				case ELEMENT_BRACE:
					{
						char bc = current_value->content[0];

						new_token->value = (void*)(uintptr_t)(bc == '}' || bc == ')');
						
						free(current_value);
					};
					break;
				case ELEMENT_OPERATION:
					{
						Operation *o = Operation_new(OPERATION_APPLICATION, NULL, NULL);

						if (String_is(current_value, "+")) {
							o->type = OPERATION_ADDITION;
						}
						if (String_is(current_value, "-")) {
							o->type = OPERATION_SUBTRACTION;
						}
						if (String_is(current_value, "*")) {
							o->type = OPERATION_MULTIPLICATION;
						}
						if (String_is(current_value, "/")) {
							o->type = OPERATION_DIVISION;
						}
						if (String_is(current_value, "==")) {
							o->type = OPERATION_EQUALITY;
						}
						if (String_is(current_value, "..")) {
							o->type = OPERATION_CONCATENATION;
						}

						new_token->value = o;

						free(current_value);
					};
					break;
				case ELEMENT_NUMBER:
					{
						Number *n = Number_new();

						char number_string[current_value->length + 1];
						memcpy(number_string, current_value->content, current_value->length);
						number_string[current_value->length] = '\0';

						if (String_has_char(current_value, '.')) {
							n->is_double = true;
							n->value_double = atof(number_string);
						} else {
							n->value_long = atoi(number_string);
						}

						new_token->value = n;

						free(current_value);
					};
					break;
			}
			tokens = Stack_push(tokens, new_token);

			current_type = ELEMENT_NOTHING;
			current_value = String_new(0);
		}

		current_type = new_type;

		if (current_type != ELEMENT_NOTHING && (in_string || current_type != ELEMENT_STRING)) {
			current_value = String_append_char(current_value, c);
		}

		in_string = current_type == ELEMENT_STRING;

		escaped = false;
	}

	free(current_value);

	return tokens;
}

Element* eval(String *script) {
	Stack *tokens = tokenise(script);

	/*
	putchar('\n');
	for (size_t i = 0; i < tokens->length; i++) {
		Element_print(tokens->content[i]);
	}
	putchar('\n');
	putchar('\n');
	*/

	Element *root_element;
	{
		size_t i = 0;
		root_element = elementify_sequence(tokens, &i);
	};

	free(tokens);

	/*
	Element_print(root_element);
	putchar('\n');
	putchar('\n');
	*/

	Element *result;
	{
		Stack *scopes = Stack_new();
		Stack *heap = Stack_new();

		result = Element_evaluate(root_element, &scopes, &heap);

		free(scopes);

		garbage_check(root_element, false);
		garbage_check(result, true);
		Element_nuke(root_element);
		garbage_collect(result, NULL, &heap);

		heaper(result, NULL);

		free(heap);
	};

	return result;
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		puts("Need one file path argument!");
		return 1;
	}

	FILE *fptr = fopen(argv[1], "rb");

	fseek(fptr, 0, SEEK_END);
	long size = ftell(fptr);
	fseek(fptr, 0, SEEK_SET);

	String *script = String_new(size);

	for (long i = 0; i < size; i++) {
		script->content[i] = fgetc(fptr);
	}

	fclose(fptr);

	Element_discard(eval(script));

	free(script);

	return 0;
}
