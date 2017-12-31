COMMON =\
	stream.c \
	lexer.c \
	error.c \
	utils.c \
	keywords.c \
	buffer.c \
	parser.c \
	expression.c \
	ir.c \
	hash_table.c \
	symbol_table.c \
	arm_core.c \
	riscos_arm.c \
	riscos_arm2.c \
	arm_gen.c \
	arm_walker.c \
	arm_reg_alloc.c \
	arm_encode.c

COMPILER =\
	 compiler.c

TESTS =\
	unit_tests.c \
	lexer_test.c \
	parser_test.c \
	symbol_table_test.c \
	vm.c \
	ir_test.c \
	arm_core_test.c \
	test_cases.c \
	arm_vm.c \
	arm_test.c

CFLAGS ?= -O3
CFLAGS += -Wall -Werror -MMD

basicc: $(COMPILER:%.c=%.o) $(COMMON:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^

unit_tests: $(TESTS:%.c=%.o) $(COMMON:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm basicc *.o *.d unit_tests

check: unit_tests
	./unit_tests

-include $(COMPILER:%.c=%.d)
-include $(COMMON:%.c=%.d)
-include $(TESTS:%.c=%.d)
