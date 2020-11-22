VPATH = common frontend test_cases arch/arm32 backends/riscos backends/riscos_common

COMMON =\
	stream.c \
	lexer.c \
	error.c \
	utils.c \
	basic_keywords.c \
	buffer.c \
	parser.c \
	parser_array.c \
	parser_assignment.c \
	parser_call.c \
	parser_compound.c \
	parser_cond.c \
	parser_exp.c \
	parser_loops.c \
	parser_math.c \
	parser_error.c \
	parser_graphics.c \
	parser_input.c \
	parser_mem.c \
	parser_output.c \
	parser_os.c \
	parser_string.c \
	parser_rnd.c \
	expression.c \
	ir.c \
	hash_table.c \
	symbol_table.c \
	constant_pool.c \
	string_pool.c \
	type.c \
	builtins.c \
	builtins_ir.c \
	call.c \
	variable.c \
	globals.c \
	type_if.c \
	float64_type.c \
	int32_type.c \
	array_int32_type.c \
	array_float64_type.c \
	array_string_type.c \
	array_type.c \
	sizet_vector.c \
	string_type_if.c \
	string_type.c \
	reference_type.c \
	local_buffer_type.c

RISCOS_COMMON =\
	riscos_arm.c

RISCOS_ARM2 =\
	riscos_swi.c \
	riscos_arm2.c

ARM =\
	arm_core.c \
	arm_gen.c \
	arm_walker.c \
	arm_reg_alloc.c \
	arm_int_dist.c \
	arm_fpa_dist.c \
	arm_encode.c \
	arm_link.c \
	arm2_div.c \
	arm_dump.c \
	fpa.c \
	fpa_gen.c \
	bitset.c \
	arm_sub_section.c \
	arm_peephole.c \
	arm_mem.c \
	arm_heap.c \
	arm_keywords.c \
	assembler.c \
	arm_expression.c

COMPILER =\
	compiler.c

INTER =\
	inter.c \
	vm.c \
	vm_heap.c

RUNARM =\
	runarm.c \
	arm_vm.c \
	arm_disass.c \
	arm_core.c \
	arm_walker.c \
	arm_dump.c \
	fpa.c \
	vm_heap.c

TESTS =\
	unit_tests.c \
	lexer_test.c \
	parser_test.c \
	symbol_table_test.c \
	vm.c \
	ir_test.c \
	test_cases.c \
	bad_test_cases.c \
	arm_core_test.c \
	arm_vm.c \
	arm_test.c \
	arm_reg_alloc_test.c \
	arm_disass.c \
	fpa_test.c \
	bitset_test.c \
	vm_heap.c

CFLAGS ?= -O3
CFLAGS += -Wall -MMD

subtro: $(COMPILER:%.c=%.o) $(COMMON:%.c=%.o) $(ARM:%.c=%.o) $(RISCOS_ARM2:%.c=%.o) $(RISCOS_COMMON:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^ -lm

inter: $(INTER:%.c=%.o) $(COMMON:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^ -lm

runarm: $(RUNARM:%.c=%.o) $(COMMON:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^ -lm

unit_tests: $(TESTS:%.c=%.o) $(COMMON:%.c=%.o) $(ARM:%.c=%.o) $(RISCOS_ARM2:%.c=%.o) $(RISCOS_COMMON:%.c=%.o)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm subtro *.o *.d unit_tests

check: unit_tests
	./unit_tests

-include $(ARM:%.c=%.d)
-include $(RISCOS_COMMON:%.c=%.d)
-include $(RISCOS_ARM2:%.c=%.d)
-include $(COMPILER:%.c=%.d)
-include $(COMMON:%.c=%.d)
-include $(INTER:%.c=%.d)
-include $(RUNARM:%.c=%.d)
-include $(TESTS:%.c=%.d)
