/*
 * Copyright (c) 2023 Mark Ryan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RV_OPCODES_H
#define RV_OPCODES_H

#include <stddef.h>
#include <stdint.h>

struct rv_opcode_t_ {
	uint8_t opcode;
	uint8_t funct3;
	uint8_t funct7;
};

typedef struct rv_opcode_t_ rv_opcode_t;

extern const rv_opcode_t rv_opcodes[];
extern const size_t rv_opcode_len;

#endif
