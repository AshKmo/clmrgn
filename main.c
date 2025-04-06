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
	ELEMENT_SCOPE_COLLECTION,
	ELEMENT_FUNCTION,
	ELEMENT_CLOSURE,
} ElementType;

typedef struct Element {
	ElementType type;
	bool heaper_checked;
	bool gc_checked;
	void *value;
} Element;

Element* Element_new(ElementType type, void *value) {
	Element *e = malloc(sizeof(Element));
	e->type = type;
	e->heaper_checked = false;
	e->gc_checked = false;
	e->value = value;
	return e;
}

typedef struct Function {
	Element *expression;
	Element *variable;
} Function;

Function* Function_new(Element *expression, Element *variable) {
	Function *f = malloc(sizeof(Function));
	f->expression = expression;
	f->variable = variable;
	return f;
}

typedef struct Closure {
	Element *expression;
	Element *variable;
	Element *scopes;
} Closure;

Closure* Closure_new(Element *expression, Element *variable, Element *scopes) {
	Closure *c = malloc(sizeof(Closure));
	c->expression = expression;
	c->variable = variable;
	c->scopes = scopes;
	return c;
}

/*
 * TO ADD NEW OPERATIONS:
 * - add a new OperationType value
 * - set its precedence in OPERATION_PRECEDENCE
 * - add a case in Element_print
 * - add cases in tokenise:
 *   - any unhandled characters in the operator must be treated as operator characters
 *   - the operator must be identified after the token is complete
 * - add its behaviour in evaluate_expression
 */

// TODO: add more operators
typedef enum {
	OPERATION_APPLICATION,
	OPERATION_POW,
	OPERATION_MULTIPLICATION,
	OPERATION_DIVISION,
	OPERATION_REMAINDER,
	OPERATION_ADDITION,
	OPERATION_SUBTRACTION,
	OPERATION_SHIFT_LEFT,
	OPERATION_SHIFT_RIGHT,
	OPERATION_CONCATENATION,
	OPERATION_LT,
	OPERATION_GT,
	OPERATION_LTE,
	OPERATION_GTE,
	OPERATION_EQUALITY,
	OPERATION_INEQUALITY,
	OPERATION_AND,
	OPERATION_XOR,
	OPERATION_OR,
} OperationType;

const int OPERATION_PRECEDENCE[] = {0, 1, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 6, 7, 7, 8, 9, 10};

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

Element* Scope_get(Scope*, Element*);

bool Element_compare(Element *a, Element *b) {
	if (a == NULL || b == NULL) {
		return false;
	}

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
		case ELEMENT_SCOPE:
			{
				Scope *sa = a->value;
				Scope *sb = b->value;

				if (sa->length != sb->length) {
					return false;
				}

				for (size_t i = 0; i < sa->length; i++) {
					if (!Element_compare(Scope_get(sb, sa->maps[i].key), sa->maps[i].value)) {
						return false;
					}
				}

				return true;
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

// TODO: add missing operators and types
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
					case OPERATION_REMAINDER:
						fputs("%", stdout);
						break;
					case OPERATION_LT:
						fputs("<", stdout);
						break;
					case OPERATION_GT:
						fputs(">", stdout);
						break;
					case OPERATION_LTE:
						fputs("<=", stdout);
						break;
					case OPERATION_GTE:
						fputs(">=", stdout);
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
	}
}

void Element_nuke(Element *e) {
	switch (e->type) {
		case ELEMENT_VARIABLE:
		case ELEMENT_NUMBER:
		case ELEMENT_STRING:
		case ELEMENT_SCOPE:
		case ELEMENT_OPERATION:
		case ELEMENT_SCOPE_COLLECTION:
		case ELEMENT_FUNCTION:
		case ELEMENT_CLOSURE:
			free(e->value);
			break;
		case ELEMENT_SEQUENCE:
			{
				Stack *sequence = e->value;

				for (size_t y = 0; y < sequence->length; y++) {
					Stack *statement = sequence->content[y];

					free(statement);
				}

				free(sequence);
			};
			break;
	}

	free(e);
}

void bruh(char *e) {
	fputs("ERROR: ", stderr);
	fputs(e, stderr);
	fputc('\n', stderr);
	exit(1);
}

Element* make(ElementType type, void *value, Stack **heap) {
	Element *e = Element_new(type, value);
	*heap = Stack_push(*heap, e);
	return e;
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
		case OPERATION_REMAINDER:
			if (result->is_double) {
				bruh("cannot apply remainder operation to floating-point values");
			} else {
				result->value_long = na->value_long % nb->value_long;
			}
			break;
		case OPERATION_POW:
			result->is_double = true;
			double da = na->is_double ? na->value_double : na->value_long;
			double db = nb->is_double ? nb->value_double : nb->value_long;
			result->value_double = pow(da, db);
			break;
		case OPERATION_LT:
		case OPERATION_GT:
			if (ot == OPERATION_GT) {
				Number *temp = na;
				na = nb;
				nb = temp;
			}
			result->is_double = false;
			result->value_long =
				(na->is_double ? na->value_double : na->value_long) <
				(nb->is_double ? nb->value_double : nb->value_long);
			break;
		case OPERATION_LTE:
		case OPERATION_GTE:
			if (ot == OPERATION_GTE) {
				Number *temp = na;
				na = nb;
				nb = temp;
			}
			result->is_double = false;
			result->value_long =
				(na->is_double ? na->value_double : na->value_long) <=
				(nb->is_double ? nb->value_double : nb->value_long);
			break;

	}

	return result;
}

Element* operatify(Stack *expression, size_t start, size_t end, Stack **heap) {
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
		return make(ELEMENT_OPERATION, Operation_new(
				OPERATION_APPLICATION,
				operatify(expression, start, end - 1, heap),
				expression->content[end - 1]
				), heap);
	}

	final_operation->a = operatify(expression, start, op_location, heap);
	final_operation->b = operatify(expression, op_location + 1, end, heap);

	return final_element;
}

Element* elementify_sequence(Stack*, size_t*, Stack **heap);

Element* elementify_expression(Stack *tokens, size_t *i, Stack **heap) {
	Stack *expression = Stack_new();

	for (; *i < tokens->length; (*i)++) {
		Element *t = tokens->content[*i];

		if (t->type == ELEMENT_BRACKET && (uintptr_t)(t->value)) {
			break;
		}

		switch (t->type) {
			case ELEMENT_BRACE:
				(*i)++;
				expression = Stack_push(expression, elementify_sequence(tokens, i, heap));
				break;
			case ELEMENT_BRACKET:
				(*i)++;
				expression = Stack_push(expression, elementify_expression(tokens, i, heap));
				break;
			default:
				expression = Stack_push(expression, t);
				break;
		}
	}

	Element *result = operatify(expression, 0, expression->length, heap);

	free(expression);

	return result;
}

Element* elementify_sequence(Stack *tokens, size_t *i, Stack **heap) {
	Stack *sequence = Stack_new();

	Stack *statement = Stack_new();

	for (; *i < tokens->length; (*i)++) {
		Element *t = tokens->content[*i];

		if (t->type == ELEMENT_BRACE && (uintptr_t)(t->value)) {
			break;
		}

		switch (t->type) {
			case ELEMENT_BRACE:
				(*i)++;
				statement = Stack_push(statement, elementify_sequence(tokens, i, heap));
				break;
			case ELEMENT_BRACKET:
				(*i)++;
				statement = Stack_push(statement, elementify_expression(tokens, i, heap));
				break;
			case ELEMENT_TERMINATOR:
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

	return make(ELEMENT_SEQUENCE, sequence, heap);
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

Stack* tokenise(String *script, Stack **heap) {
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
				case '<':
				case '>':
				case '&':
				case '|':
				case '^':
				case '!':
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

			Element *new_token = make(current_type, NULL, heap);

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
						} else if (String_is(current_value, "-")) {
							o->type = OPERATION_SUBTRACTION;
						} else if (String_is(current_value, "*")) {
							o->type = OPERATION_MULTIPLICATION;
						} else if (String_is(current_value, "/")) {
							o->type = OPERATION_DIVISION;
						} else if (String_is(current_value, "%")) {
							o->type = OPERATION_REMAINDER;
						} else if (String_is(current_value, "==")) {
							o->type = OPERATION_EQUALITY;
						} else if (String_is(current_value, "..")) {
							o->type = OPERATION_CONCATENATION;
						} else if (String_is(current_value, "<")) {
							o->type = OPERATION_LT;
						} else if (String_is(current_value, ">")) {
							o->type = OPERATION_GT;
						} else if (String_is(current_value, "<=")) {
							o->type = OPERATION_LTE;
						} else if (String_is(current_value, ">=")) {
							o->type = OPERATION_GTE;
						} else if (String_is(current_value, "!=")) {
							o->type = OPERATION_INEQUALITY;
						} else if (String_is(current_value, "<<")) {
							o->type = OPERATION_SHIFT_LEFT;
						} else if (String_is(current_value, ">>")) {
							o->type = OPERATION_SHIFT_RIGHT;
						} else if (String_is(current_value, "&")) {
							o->type = OPERATION_AND;
						} else if (String_is(current_value, "|")) {
							o->type = OPERATION_OR;
						} else if (String_is(current_value, "^")) {
							o->type = OPERATION_XOR;
						} else if (String_is(current_value, "**")) {
							o->type = OPERATION_POW;
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

Element* heaper(Element *e, bool gc_checked, Stack **heap) {
	if (e == NULL) {
		return e;
	}

	if (e->heaper_checked) {
		return e;
	}

	e->heaper_checked = true;

	e->gc_checked = gc_checked;

	if (heap != NULL) {
		size_t i = 0;

		for (; i < (*heap)->length; i++) {
			if ((*heap)->content[i] == e) {
				break;
			}
		}

		if (i == (*heap)->length) {
			*heap = Stack_push(*heap, e);
		}
	}

	switch (e->type) {
		case ELEMENT_OPERATION:
			{
				Operation *o = e->value;
				heaper(o->a, gc_checked, heap);
				heaper(o->b, gc_checked, heap);
			};
			break;
		case ELEMENT_SEQUENCE:
			{
				Stack *sequence = e->value;

				for (size_t y = 0; y < sequence->length; y++) {
					Stack *statement = sequence->content[y];

					for (size_t x = 0; x < statement->length; x++) {
						heaper(statement->content[x], gc_checked, heap);
					}
				}
			};
			break;
		case ELEMENT_SCOPE:
			{
				Scope *s = e->value;

				for (size_t i = 0; i < s->length; i++) {
					heaper(s->maps[i].key, gc_checked, heap);
					heaper(s->maps[i].value, gc_checked, heap);
				}
			};
			break;
		case ELEMENT_SCOPE_COLLECTION:
			{
				Stack *scopes = e->value;

				for (size_t i = 0; i < scopes->length; i++) {
					heaper(scopes->content[i], gc_checked, heap);
				}
			};
			break;
		case ELEMENT_FUNCTION:
			{
				Function *f = e->value;
				heaper(f->expression, gc_checked, heap);
				heaper(f->variable, gc_checked, heap);
			};
			break;
		case ELEMENT_CLOSURE:
			{
				Closure *c = e->value;
				heaper(c->expression, gc_checked, heap);
				heaper(c->variable, gc_checked, heap);
				heaper(c->scopes, gc_checked, heap);
			};
			break;
	}

	e->heaper_checked = false;

	return e;
}

void garbage_collect(Element *result, Element *ast_root, Stack **scopes_stack, Stack **heap) {
	if (result != NULL) {
		heaper(result, true, NULL);
	}

	if (ast_root != NULL) {
		heaper(ast_root, true, NULL);
	}

	if (scopes_stack != NULL) {
		for (size_t i = 0; i < (*scopes_stack)->length; i++) {
			heaper((*scopes_stack)->content[i], true, NULL);
		}
	}

	for (size_t i = 0; i < (*heap)->length; i++) {
		Element *e = (*heap)->content[i];
		if (e->gc_checked) {
			e->gc_checked = false;
		} else {
			*heap = Stack_delete(*heap, i);
			Element_nuke(e);
			i--;
		}
	}
}

void Element_discard(Element *e) {
	Stack *heap = Stack_new();

	heaper(e, false, &heap);

	for (size_t i = 0; i < heap->length; i++) {
		Element_nuke(heap->content[i]);
	}

	free(heap);
}

void setvar(Element *key, Element *value, Element *scopes, bool local) {
	Stack *scope_collection = scopes->value;

	Element *scope;

	if (local) {
		scope = scope_collection->content[scope_collection->length - 1];
	} else {
		for (size_t i = 0; i < scope_collection->length; i++) {
			scope = scope_collection->content[i];

			if (i == scope_collection->length - 1 || Scope_has(scope->value, key)) {
				break;
			}
		}
	}

	scope->value = Scope_set(scope->value, key, value);
}

Element* evaluate_expression(Element *e, Element *ast_root, Stack **scopes_stack, Stack **heap) {
	Element *scopes = (*scopes_stack)->content[(*scopes_stack)->length - 1];

	switch (e->type) {
		case ELEMENT_SEQUENCE:
			{
				Element *scope = make(ELEMENT_SCOPE, Scope_new(), heap);

				scopes->value = Stack_push(scopes->value, scope);

				Stack *sequence = e->value;

				for (size_t i = 0; i < sequence->length; i++) {
					Stack *statement = sequence->content[i];

					Element *command = statement->content[0];

					if (command->type != ELEMENT_VARIABLE) {
						bruh("command name must be a keyword");
					}

					if (String_is(command->value, "do")) {
						for (size_t x = 1; x < statement->length; x++) {
							evaluate_expression(statement->content[x], ast_root, scopes_stack, heap);
						}
					}

					{
						bool writing = String_is(command->value, "write");
						bool printing = String_is(command->value, "print");
						bool showing = String_is(command->value, "show");
						bool displaying = String_is(command->value, "display");

						if (printing || writing || showing || displaying) {
							if (statement->length < 2) {
								if (writing) {
									bruh("'write' command accepts at least 1 argument");
								} else if (printing) {
									bruh("'print' command accepts at least 1 argument");
								} else if (showing) {
									bruh("'show' command accepts at least 1 argument");
								} else {
									bruh("'display' command accepts at least 1 argument");
								}
							}

							for (size_t x = 1; x < statement->length; x++) {
								Element *to_print = evaluate_expression(statement->content[x], ast_root, scopes_stack, heap);

								if ((writing || printing) && to_print->type == ELEMENT_STRING) {
									String_print(to_print->value);
								} else {
									Element_print(to_print);
								}

								if ((showing || displaying) && x < statement->length - 1) {
									putchar(' ');
								}
							}

							if (printing || displaying) {
								putchar('\n');
							}
						}
					};

					if (String_is(command->value, "return")) {
						if (statement->length != 2) {
							bruh("'return' command accepts exactly 1 argument");
						}

						Element *result = evaluate_expression(statement->content[1], ast_root, scopes_stack, heap);

						scopes->value = Stack_pop(scopes->value);

						return result;
					}

					if (String_is(command->value, "function")) {
						if (statement->length < 3) {
							bruh("'function' command acccepts at least 2 arguments");
						}

						Element *expression = statement->content[statement->length - 1];

						if (statement->length == 3) {
								expression = make(ELEMENT_FUNCTION, Function_new(expression, NULL), heap);
						} else {
							for (size_t i = statement->length - 2; i >= 2; i--) {
								expression = make(ELEMENT_FUNCTION, Function_new(expression, statement->content[i]), heap);
							}
						}

						Element *final_closure = evaluate_expression(expression, ast_root, scopes_stack, heap);

						setvar(statement->content[1], final_closure, scopes, true);
					}

					{
						bool letting = String_is(command->value, "let");
						bool setting = String_is(command->value, "set");

						if (letting || setting) {
							if (statement->length != 3) {
								if (letting) {
									bruh("'let' command accepts exactly 2 arguments");
								} else {
									bruh("'set' command accepts exactly 2 arguments");
								}
							}

							Element *key = statement->content[1];

							Element *value = evaluate_expression(statement->content[2], ast_root, scopes_stack, heap);

							setvar(key, value, scopes, letting);
						}
					};

					if (String_is(command->value, "mut")) {
						if (statement->length != 4) {
							bruh("'mut' command accepts exactly 3 arguments");
						}

						Element *subject = evaluate_expression(statement->content[1], ast_root, scopes_stack, heap);
						Element *key = evaluate_expression(statement->content[2], ast_root, scopes_stack, heap);
						Element *value = evaluate_expression(statement->content[3], ast_root, scopes_stack, heap);

						subject->value = Scope_set(subject->value, key, value);
					}

					if (String_is(command->value, "if")) {
						if (statement->length < 2) {
							bruh("'if' command accepts at least 1 argument");
						}

						for (size_t i = 1; i < statement->length; i += 2) {
							if (i == statement->length - 1) {
								evaluate_expression(statement->content[i], ast_root, scopes_stack, heap);
							} else {
								Element *condition = evaluate_expression(statement->content[i], ast_root, scopes_stack, heap);

								if (Element_is_truthy(condition)) {
									evaluate_expression(statement->content[i + 1], ast_root, scopes_stack, heap);
									break;
								}
							}
						}
					}

					if (String_is(command->value, "while")) {
						if (statement->length != 3) {
							bruh("'while' command accepts exactly 2 arguments");
						}

						while (Element_is_truthy(evaluate_expression(statement->content[1], ast_root, scopes_stack, heap))) {
							evaluate_expression(statement->content[2], ast_root, scopes_stack, heap);
						}
					}

					if (String_is(command->value, "bruh")) {
						if (statement->length != 2) {
							bruh("'bruh' command accepts exactly 1 argument");
						}

						Element *to_bruh = evaluate_expression(statement->content[1], ast_root, scopes_stack, heap);

						fputs("ERROR: ", stdout);
						String_print(to_bruh->value);
						putchar('\n');
						exit(1);
					}

					garbage_collect(NULL, ast_root, scopes_stack, heap);
				}

				scopes->value = Stack_pop(scopes->value);

				return scope;
			};
			break;
		case ELEMENT_OPERATION:
			{
				Operation *o = e->value;

				Element *a = evaluate_expression(o->a, ast_root, scopes_stack, heap);
				Element *b = evaluate_expression(o->b, ast_root, scopes_stack, heap);

				switch (o->type) {
					case OPERATION_APPLICATION:
						switch (a->type) {
							case ELEMENT_CLOSURE:
								{
									Closure *c = a->value;

									Stack *scopes = c->scopes->value;

									Element *new_scopes = make(ELEMENT_SCOPE_COLLECTION, Stack_new(), heap);

									for (size_t i = 0; i < scopes->length; i++) {
										new_scopes->value = Stack_push(new_scopes->value, scopes->content[i]);
									}

									if (c->variable != NULL) {
										Element *scope = make(ELEMENT_SCOPE, Scope_new(), heap);

										new_scopes->value = Stack_push(new_scopes->value, scope);

										scope->value = Scope_set(scope->value, c->variable, b);
									}

									*scopes_stack = Stack_push(*scopes_stack, new_scopes);

									Element *result = evaluate_expression(c->expression, ast_root, scopes_stack, heap);

									*scopes_stack = Stack_pop(*scopes_stack);

									return result;
								};
							case ELEMENT_NUMBER:
								return make(ELEMENT_NUMBER, Number_operate(OPERATION_MULTIPLICATION, a->value, b->value), heap);
							case ELEMENT_SCOPE:
								{
									Element *result = Scope_get(a->value, b);
									if (result == NULL) {
										bruh("no such key in scope");
									}
									return Scope_get(a->value, b);
								};
							default:
								bruh("application cannot be applied to this type");
						}
					case OPERATION_EQUALITY:
					case OPERATION_INEQUALITY:
						{
							Number *n = Number_new();
							n->value_long = (Element_compare(a, b) != (o->type == OPERATION_INEQUALITY)) ? 1 : 0;
							return make(ELEMENT_NUMBER, n, heap);
						};
					case OPERATION_SHIFT_LEFT:
					case OPERATION_SHIFT_RIGHT:
					case OPERATION_AND:
					case OPERATION_OR:
					case OPERATION_XOR:
						{
							if (a->type != ELEMENT_NUMBER || b->type != ELEMENT_NUMBER) {
								bruh("only integers may be shifted, ANDed, ORed or XORed");
							}
							Number *na = a->value;
							Number *nb = b->value;
							if (na->is_double || nb->is_double) {
								bruh("only integers may be shifted, ANDed, ORed or XORed");
							}
							Number *n = Number_new();
							switch (o->type) {
								case OPERATION_SHIFT_LEFT:
									n->value_long = na->value_long << nb->value_long;
									break;
								case OPERATION_SHIFT_RIGHT:
									n->value_long = na->value_long >> nb->value_long;
									break;
								case OPERATION_AND:
									n->value_long = na->value_long & nb->value_long;
									break;
								case OPERATION_OR:
									n->value_long = na->value_long | nb->value_long;
									break;
								case OPERATION_XOR:
									n->value_long = na->value_long ^ nb->value_long;
									break;
							}
							return make(ELEMENT_NUMBER, n, heap);
						};
					case OPERATION_LT:
					case OPERATION_GT:
					case OPERATION_LTE:
					case OPERATION_GTE:
					case OPERATION_ADDITION:
					case OPERATION_SUBTRACTION:
					case OPERATION_MULTIPLICATION:
					case OPERATION_DIVISION:
					case OPERATION_REMAINDER:
					case OPERATION_POW:
						if (a->type != ELEMENT_NUMBER || b->type != ELEMENT_NUMBER) {
							bruh("numeric operation applied to non-numeric value");
						}
						return make(ELEMENT_NUMBER, Number_operate(o->type, a->value, b->value), heap);
					case OPERATION_CONCATENATION:
						{
							if (a->type != ELEMENT_STRING || b->type != ELEMENT_STRING) {
								bruh("string concatenation applied to non-string value");
							}

							String *sa = a->value;
							String *sb = b->value;

							String *result = String_new(sa->length + sb->length);

							for (size_t i = 0; i < result->length; i++) {
								if (i < sa->length) {
									result->content[i] = sa->content[i];
								} else {
									result->content[i] = sb->content[i - sa->length];
								}
							}

							return make(ELEMENT_STRING, result, heap);
						};
				}
			};
			break;
		case ELEMENT_VARIABLE:
			{
				Stack *scope_list = scopes->value;
				for (size_t i = 0; i < scope_list->length; i++) {
					Element *scope_element = scope_list->content[i];
					Element *result = Scope_get(scope_element->value, e);
					if (result != NULL) {
						return result;
					}
				}

				putchar('\n');
				String_print(e->value);
				bruh("variable not found");
			};
		case ELEMENT_FUNCTION:
			{
				Function *f = e->value;
				return make(ELEMENT_CLOSURE, Closure_new(f->expression, f->variable, scopes), heap);
			};
		default:
			return e;
	}
}

Element* eval(String *script, Element *scopes) {
	Stack *heap = Stack_new();

	Stack *tokens = tokenise(script, &heap);

	/*
	putchar('\n');
	for (size_t i = 0; i < tokens->length; i++) {
		Element_print(tokens->content[i]);
	}
	putchar('\n');
	putchar('\n');
	*/

	Element *ast_root;
	{
		size_t i = 0;
		ast_root = elementify_sequence(tokens, &i, &heap);
	};

	free(tokens);

	/*
	Element_print(ast_root);
	putchar('\n');
	putchar('\n');
	*/

	Element *result;
	{
		Stack *scopes_stack = Stack_new();

		if (scopes == NULL) {
			scopes_stack = Stack_push(scopes_stack, make(ELEMENT_SCOPE_COLLECTION, Stack_new(), &heap));
		} else {
			scopes_stack = Stack_push(scopes_stack, scopes);
		}

		result = evaluate_expression(ast_root, ast_root, &scopes_stack, &heap);

		free(scopes_stack);
	};

	garbage_collect(result, NULL, NULL, &heap);

	free(heap);

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

	Element_discard(eval(script, NULL));

	free(script);

	return 0;
}
