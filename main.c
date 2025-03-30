#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

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
	for (; i < s->length; i++) {
		s->content[i - 1] = s->content[i];
	}

	s->length--;
	s = realloc(s, sizeof(Stack) * s->length * sizeof(void*));
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
} ElementType;

typedef struct Element {
	ElementType type;
	bool gc_checked;
	void *value;
} Element;

Element* Element_new(ElementType type, void *value) {
	Element *e = malloc(sizeof(Element));
	e->type = type;
	e->gc_checked = false;
	e->value = value;
	return e;
}

/*
 * TO ADD NEW OPERATIONS:
 * - add a new OperationType value
 * - set its precedence in OPERATION_PRECEDENCE
 * - add a case in Element_print
 * - add a case in elementify_token
 * - add a case in tokenise
 */

typedef enum {
	OPERATION_APPLICATION,
	OPERATION_MULTIPLICATION,
	OPERATION_DIVISION,
	OPERATION_ADDITION,
	OPERATION_SUBTRACTION,
	OPERATION_EQUALITY,
} OperationType;

const int OPERATION_PRECEDENCE[] = {0, 1, 1, 2, 2, 3};

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

bool Element_compare(Element *a, Element *b) {
	if (a->type != b->type) {
		return false;
	}

	switch (a->type) {
		case ELEMENT_NULL:
			return true;
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
		case ELEMENT_SCOPE:
			{
				Scope *s = e->value;
				for (size_t i = 0; i < s->length; i++) {
					garbage_check(s->maps[i].key, gc_check);
					garbage_check(s->maps[i].value, gc_check);
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

Element* Element_evaluate(Element *e, Stack **scopes, Stack **heap) {
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
				expression->content[start],
				operatify(expression, start + 1, end)
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
				case '-':
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
					new_type = ELEMENT_NUMBER;
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

	putchar('\n');
	for (size_t i = 0; i < tokens->length; i++) {
		Element_print(tokens->content[i]);
	}
	putchar('\n');
	putchar('\n');

	Element *root_element;
	{
		size_t i = 0;
		root_element = elementify_sequence(tokens, &i);
	};

	free(tokens);

	Element_print(root_element);
	putchar('\n');
	putchar('\n');

	Element *result;
	{
		Stack *scopes = Stack_new();
		Stack *heap = Stack_new();

		result = Element_evaluate(root_element, &scopes, &heap);

		free(scopes);

		garbage_check(root_element, false);
		garbage_collect(result, NULL, &heap);

		free(heap);
	};

	Element_nuke(root_element);

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

	eval(script);

	free(script);

	return 0;
}
