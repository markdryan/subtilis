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
	string_pool.c \
	type.c \
	builtins.c \
	call.c

ARM =\
	arm_core.c \
	riscos_arm.c \
	riscos_arm2.c \
	arm_gen.c \
	arm_walker.c \
	arm_reg_alloc.c \
	arm_encode.c \
	arm_link.c \
	arm2_div.c

COMPILER =\
	compiler.c

INTER =\
	inter.c \
	vm.c

RUNARM =\
	runarm.c \
	arm_vm.c \
	arm_disass.c \
	arm_core.c \
	arm_walker.c

TESTS =\
	unit_tests.c \
	lexer_test.c \
	parser_test.c \
	symbol_table_test.c \
	vm.c \
	ir_test.c \
	test_cases.c \
	arm_core_test.c \
	arm_vm.c \
	arm_test.c \
	arm_reg_alloc_test.c \
	arm_disass.c

CFLAGS ?= -O3
CFLAGS += -Wall -Werror -MMD

basicc: $(COMPILER:%.c=%.o) $(COMMON:%.c=%.o) $(ARM:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^

inter: $(INTER:%.c=%.o) $(COMMON:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^

runarm: $(RUNARM:%.c=%.o) $(COMMON:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^

unit_tests: $(TESTS:%.c=%.o) $(COMMON:%.c=%.o) $(ARM:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm basicc *.o *.d unit_tests

check: unit_tests
	./unit_tests

-include $(ARM:%.c=%.d)
-include $(COMPILER:%.c=%.d)
-include $(COMMON:%.c=%.d)
-include $(INTER:%.c=%.d)
-include $(RUNARM:%.c=%.d)
-include $(TESTS:%.c=%.d)
