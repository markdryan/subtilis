DEPS = stream.h lexer.h config.h error.h utils.h keywords.h buffer.h expression.h ir.h
OBJ = stream.o lexer.o error.o utils.o keywords.o buffer.o parser.o expression.o ir.o
UNIT_OBJS = unit_tests.o lexer_test.o parser_test.o

CFLAGS ?= -O3 -Wall -Werror

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

basicc: compiler.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

unit_tests: $(UNIT_OBJS) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm basicc *.o unit_tests

check: unit_tests
	./unit_tests
