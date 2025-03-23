#include <stdio.h>
#include <stdbool.h>
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

Stack* Stack_fpop(Stack *s) {
	free(s->content[s->length - 1]);
	return Stack_pop(s);
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
	ELEMENT_NULL,
	ELEMENT_VARIABLE,
	ELEMENT_NUMBER,
	ELEMENT_STRING,
	ELEMENT_OPERATION,
	ELEMENT_SEQUENCE,
} ElementType;

typedef struct Element {
	ElementType type;
	void *value;
} Element;

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

Element* Element_new(ElementType type, void *value) {
	Element *e = malloc(sizeof(Element));
	e->type = type;
	e->value = value;
	return e;
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
				putchar('(');
				Element_print(o->a);
				putchar(' ');
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
						putchar('=');
						break;
				}
				putchar(' ');
				Element_print(o->b);
				putchar(')');
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
	}
}

void Element_nuke(Element *e) {
	switch (e->type) {
		case ELEMENT_VARIABLE:
		case ELEMENT_NUMBER:
		case ELEMENT_STRING:
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

typedef enum {
	TOKEN_NOTHING,
	TOKEN_NULL,
	TOKEN_VARIABLE,
	TOKEN_OPERATOR,
	TOKEN_STRING,
	TOKEN_NUMBER,
	TOKEN_BRACKET,
	TOKEN_TERMINATOR,
	TOKEN_BRACE,
} TokenType;

typedef struct Token {
	TokenType type;
	String *value;
} Token;

Token* Token_new(TokenType type, String *value) {
	Token *t = malloc(sizeof(Token));
	t->type = type;
	t->value = value;
	return t;
}

void Token_nuke(Token *t) {
	free(t->value);
	free(t);
}

Element* elementify_token(Token *t) {
	switch (t->type) {
		case TOKEN_NULL:
			return Element_new(ELEMENT_NULL, NULL);
		case TOKEN_NUMBER:
			{
				Number *n = Number_new();

				char number_string[t->value->length + 1];
				memcpy(number_string, t->value->content, t->value->length);
				number_string[t->value->length] = '\0';

				if (String_has_char(t->value, '.')) {
					n->is_double = true;
					n->value_double = atof(number_string);
				} else {
					n->value_long = atoi(number_string);
				}

				return Element_new(ELEMENT_NUMBER, n);
			};
		case TOKEN_OPERATOR:
			{
				int value;
				switch (t->value->content[0]) {
					case '+':
						value = OPERATION_ADDITION;
						break;
					case '-':
						value = OPERATION_SUBTRACTION;
						break;
					case '*':
						value = OPERATION_MULTIPLICATION;
						break;
					case '/':
						value = OPERATION_DIVISION;
						break;
					case '=':
						value = OPERATION_EQUALITY;
						break;
				}
				return Element_new(ELEMENT_OPERATION, Operation_new(value, NULL, NULL));
			};
		case TOKEN_STRING:
			{
				String *s = t->value;
				String *new_s = String_new(s->length);
				memcpy(new_s->content, s->content, s->length);
				return Element_new(ELEMENT_STRING, new_s);
			};
		case TOKEN_VARIABLE:
			{
				String *s = t->value;
				String *new_s = String_new(s->length);
				memcpy(new_s->content, s->content, s->length);
				return Element_new(ELEMENT_VARIABLE, new_s);
			};
	}
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
		Token *t = tokens->content[*i];

		if (t->type == TOKEN_BRACKET && t->value->content[0] == ')') {
			break;
		}

		switch (t->type) {
			case TOKEN_BRACE:
				(*i)++;
				expression = Stack_push(expression, elementify_sequence(tokens, i));
				break;
			case TOKEN_BRACKET:
				(*i)++;
				expression = Stack_push(expression, elementify_expression(tokens, i));
				break;
			default:
				expression = Stack_push(expression, elementify_token(t));
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
		Token *t = tokens->content[*i];

		if (t->type == TOKEN_BRACE && t->value->content[0] == '}') {
			break;
		}

		switch (t->type) {
			case TOKEN_BRACE:
				(*i)++;
				statement = Stack_push(statement, elementify_sequence(tokens, i));
				break;
			case TOKEN_BRACKET:
				(*i)++;
				statement = Stack_push(statement, elementify_expression(tokens, i));
				break;
			case TOKEN_TERMINATOR:
				if (statement->length < 0) {
					break;
				}

				sequence = Stack_push(sequence, statement);

				statement = Stack_new();
				break;
			default:
				statement = Stack_push(statement, elementify_token(tokens->content[*i]));
				break;
		}
	}

	free(statement);

	return Element_new(ELEMENT_SEQUENCE, sequence);
}

void print_tokens(Stack *tokens) {
	char token_types[] = "Z?VOSNB;M";
	for (size_t i = 0; i < tokens->length; i++) {
		Token *token = tokens->content[i];
		printf("[%c ", token_types[token->type]);
		String_print(token->value);
		printf("] ");
	}
}

Stack* tokenise(String *script) {
	Stack *tokens = Stack_new();

	TokenType current_type;
	String *current_value = String_new(0);

	bool escaped = false;
	bool comment = false;
	bool in_string = false;

	for (size_t i = 0; i <= script->length; i++) {
		TokenType new_type;

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
			new_type = TOKEN_VARIABLE;

			if (in_string) {
				new_type = TOKEN_STRING;

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
					new_type = TOKEN_NOTHING;
				}
			}
		} else {
			switch (c) {
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					new_type = TOKEN_NOTHING;
					break;
				case ';':
					new_type = TOKEN_TERMINATOR;
					break;
				case '(':
				case ')':
					new_type = TOKEN_BRACKET;
					break;
				case '{':
				case '}':
					new_type = TOKEN_BRACE;
					break;
				case '"':
					new_type = TOKEN_STRING;
					break;
				case '+':
				case '-':
				case '*':
				case '/':
				case '%':
				case '=':
					new_type = TOKEN_OPERATOR;
					break;
				case '.':
					if (current_type != TOKEN_NUMBER) {
						new_type = TOKEN_OPERATOR;
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
					new_type = TOKEN_NUMBER;
					break;
				case '?':
					new_type = TOKEN_NULL;
					break;
				default:
					new_type = TOKEN_VARIABLE;
					break;
			}
		}

		if ((current_value->length > 0 || in_string) && (new_type != current_type ||
					current_type == TOKEN_BRACKET ||
					current_type == TOKEN_BRACE ||
					current_type == TOKEN_TERMINATOR
					)) {

			Token *new_token = Token_new(current_type, current_value);
			tokens = Stack_push(tokens, new_token);
			current_type = TOKEN_NOTHING;
			current_value = String_new(0);
		}

		current_type = new_type;

		if (current_type != TOKEN_NOTHING && (in_string || current_type != TOKEN_STRING)) {
			current_value = String_append_char(current_value, c);
		}

		in_string = current_type == TOKEN_STRING;

		escaped = false;
	}

	free(current_value);

	return tokens;
}

void eval(String *script) {
	Stack *tokens = tokenise(script);

	putchar('\n');
	print_tokens(tokens);
	putchar('\n');
	putchar('\n');

	Element *root_element;
	{
		size_t i = 0;
		root_element = elementify_sequence(tokens, &i);
	};

	for (size_t i = 0; i < tokens->length; i++) {
		Token_nuke(tokens->content[i]);
	}
	free(tokens);

	Element_print(root_element);
	putchar('\n');
	putchar('\n');

	Element_nuke(root_element);
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
