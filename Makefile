DEPS = stream.h lexer.h config.h error.h utils.h keywords.h buffer.h expression.h ir.h hash_table.h symbol_table.h vm.h
OBJ = stream.o lexer.o error.o utils.o keywords.o buffer.o parser.o expression.o ir.o hash_table.o symbol_table.o
UNIT_OBJS = unit_tests.o lexer_test.o parser_test.o symbol_table_test.o vm.o ir_test.o

CFLAGS ?= -O3
CFLAGS += -Wall -Werror

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
