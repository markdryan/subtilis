/*
 * Copyright (c) 2020 Mark Ryan
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ptd_swi.h"

/* clang-format off */

const subtilis_arm_swi_t subtilis_ptd_swi_list[] = {
	{ 0x00, "OS_WriteC", 0x1, 0x0 },
	{ 0x01, "OS_WriteS", 0x0, 0x0 },
	{ 0x02, "OS_Write0", 0x1, 0x1 },
	{ 0x03, "OS_NewLine", 0x0, 0x0 },
	{ 0x04, "OS_ReadC", 0x0, 0x1 },
	{ 0x05, "OS_CLI", 0x1, 0x0 },
	{ 0x06, "OS_Byte", 0x7, 0x6 },
	{ 0x07, "OS_Word", 0x3, 0x0 },
	{ 0x08, "OS_File", 0x1ff, 0x7f },
	{ 0x09, "OS_Args", 0x7, 0x5 },
	{ 0x0A, "OS_BGet", 0x2, 0x1 },
	{ 0x0B, "OS_BPut", 0x3, 0x0 },
	{ 0x0C, "OS_GBPB", 0x7f, 0x1e },
	{ 0x0D, "OS_Find", 0x7, 0x1 },
	{ 0x0E, "OS_ReadLine", 0x1f, 0xf },
	{ 0x0F, "OS_Control", 0xf, 0xf },
	{ 0x10, "OS_GetEnv", 0x0, 0x7 },
	{ 0x11, "OS_Exit", 0x7, 0x0 },
	{ 0x12, "OS_SetEnv", 0xf3, 0xf3 },
	{ 0x13, "OS_IntOn", 0x0, 0x0 },
	{ 0x14, "OS_IntOff", 0x0, 0x0 },
	{ 0x15, "OS_CallBack", 0x3, 0x3 },
	{ 0x16, "OS_EnterOS", 0x0, 0x0 },
	{ 0x17, "OS_BreakPt", 0x0, 0x0 },
	{ 0x18, "OS_BreakCtrl", 0x3, 0x3 },
	{ 0x19, "OS_UnusedSWI", 0x1, 0x1 },
	{ 0x1A, "OS_UpdateMEMC", 0x3, 0x3 },
	{ 0x1B, "OS_SetCallBack", 0x0, 0x0 },
	{ 0x1C, "OS_Mouse", 0x0, 0xf },
	{ 0x1D, "OS_Heap", 0xf, 0xf },
	{ 0x1E, "OS_Module", 0xf, 0x7e },
	{ 0x1F, "OS_Claim", 0x7, 0x0 },
	{ 0x20, "OS_Release", 0x7, 0x0 },
	{ 0x21, "OS_ReadUnsigned", 0x7, 0x6 },
	{ 0x22, "OS_GenerateEvent", 0x1f, 0x0 },
	{ 0x23, "OS_ReadVarVal", 0x1f, 0x1c },
	{ 0x24, "OS_SetVarVal", 0x1f, 0x18 },
	{ 0x25, "OS_GSInit", 0x5, 0x7 },
	{ 0x26, "OS_GSRead", 0x5, 0x7 },
	{ 0x27, "OS_GSTrans", 0x7, 0x7 },
	{ 0x28, "OS_BinaryToDecimal", 0x7, 0x4 },
	{ 0x29, "OS_FSControl", 0x1ff, 0x6f },
	{ 0x2A, "OS_ChangeDynamicArea", 0x3, 0x2 },
	{ 0x2B, "OS_GenerateError", 0x1, 0x0 },
	{ 0x2C, "OS_ReadEscapeState", 0x0, 0x0 },
	{ 0x2D, "OS_EvaluateExpression", 0x7, 0x6 },
	{ 0x2E, "OS_SpriteOp", 0xff, 0x7e },
	{ 0x2F, "OS_ReadPalette", 0x3, 0xc },
	{ 0x30, "OS_ServiceCall", 0x1ff, 0x1ff },
	{ 0x31, "OS_ReadVduVariables", 0x3, 0x0 },
	{ 0x32, "OS_ReadPoint", 0x3, 0x1c },
	{ 0x33, "OS_UpCall", 0x3ff, 0x1 },
	{ 0x34, "OS_CallAVector", 0x3ff, 0x7ff },
	{ 0x35, "OS_ReadModeVariable", 0x3, 0x4 },
	{ 0x36, "OS_RemoveCursors", 0x0, 0x0 },
	{ 0x37, "OS_RestoreCursors", 0x0, 0x0 },
	{ 0x38, "OS_SWINumberToString", 0x7, 0x4 },
	{ 0x39, "OS_SWINumberFromString", 0x2, 0x1 },
	{ 0x3A, "OS_ValidateAddress", 0x3, 0x0 },
	{ 0x3B, "OS_CallAfter", 0x7, 0x0 },
	{ 0x3C, "OS_CallEvery", 0x7, 0x0 },
	{ 0x3D, "OS_RemoveTickerEvent", 0x3, 0x0 },
	{ 0x3E, "OS_InstallKeyHandler", 0x1, 0x1 },
	{ 0x3F, "OS_CheckModeValid", 0x1, 0x1 },
	{ 0x40, "OS_ChangeEnvironment", 0xf, 0xe },
	{ 0x41, "OS_ClaimScreenMemory", 0x3, 0x6 },
	{ 0x42, "OS_ReadMonotonicTime", 0x0, 0x1 },
	{ 0x43, "OS_SubstituteArgs", 0x1f, 0x4 },
	{ 0x44, "OS_PrettyPrint", 0x7, 0x0 },
	{ 0x45, "OS_Plot", 0x7, 0x7 },
	{ 0x46, "OS_WriteN", 0x3, 0x0 },
	{ 0x47, "OS_AddToVector", 0x7, 0x0 },
	{ 0x48, "OS_WriteEnv", 0x3, 0x0 },
	{ 0x49, "OS_ReadArgs", 0xf, 0x8 },
	{ 0x4A, "OS_ReadRAMFsLimits", 0x0, 0x3 },
	{ 0x4B, "OS_ClaimDeviceVector", 0x1f, 0x0 },
	{ 0x4C, "OS_ReleaseDeviceVector", 0x1f, 0x0 },
	{ 0x4D, "OS_DelinkApplication", 0x3, 0x2 },
	{ 0x4E, "OS_RelinkApplication", 0x1, 0x0 },
	{ 0x4F, "OS_HeapSort", 0x7f, 0x0 },
};

const size_t subtilis_ptd_known_swis = sizeof(subtilis_ptd_swi_list) /
	sizeof(subtilis_arm_swi_t);

const size_t subtilis_ptd_swi_index[] = {
	71, // "OS_AddToVector"
	9, // "OS_Args"
	10, // "OS_BGet"
	11, // "OS_BPut"
	40, // "OS_BinaryToDecimal"
	24, // "OS_BreakCtrl"
	23, // "OS_BreakPt"
	6, // "OS_Byte"
	5, // "OS_CLI"
	52, // "OS_CallAVector"
	59, // "OS_CallAfter"
	21, // "OS_CallBack"
	60, // "OS_CallEvery"
	42, // "OS_ChangeDynamicArea"
	64, // "OS_ChangeEnvironment"
	63, // "OS_CheckModeValid"
	31, // "OS_Claim"
	75, // "OS_ClaimDeviceVector"
	65, // "OS_ClaimScreenMemory"
	15, // "OS_Control"
	77, // "OS_DelinkApplication"
	22, // "OS_EnterOS"
	45, // "OS_EvaluateExpression"
	17, // "OS_Exit"
	41, // "OS_FSControl"
	8, // "OS_File"
	13, // "OS_Find"
	12, // "OS_GBPB"
	37, // "OS_GSInit"
	38, // "OS_GSRead"
	39, // "OS_GSTrans"
	43, // "OS_GenerateError"
	34, // "OS_GenerateEvent"
	16, // "OS_GetEnv"
	29, // "OS_Heap"
	79, // "OS_HeapSort"
	62, // "OS_InstallKeyHandler"
	20, // "OS_IntOff"
	19, // "OS_IntOn"
	30, // "OS_Module"
	28, // "OS_Mouse"
	3, // "OS_NewLine"
	69, // "OS_Plot"
	68, // "OS_PrettyPrint"
	73, // "OS_ReadArgs"
	4, // "OS_ReadC"
	44, // "OS_ReadEscapeState"
	14, // "OS_ReadLine"
	53, // "OS_ReadModeVariable"
	66, // "OS_ReadMonotonicTime"
	47, // "OS_ReadPalette"
	50, // "OS_ReadPoint"
	74, // "OS_ReadRAMFsLimits"
	33, // "OS_ReadUnsigned"
	35, // "OS_ReadVarVal"
	49, // "OS_ReadVduVariables"
	32, // "OS_Release"
	76, // "OS_ReleaseDeviceVector"
	78, // "OS_RelinkApplication"
	54, // "OS_RemoveCursors"
	61, // "OS_RemoveTickerEvent"
	55, // "OS_RestoreCursors"
	57, // "OS_SWINumberFromString"
	56, // "OS_SWINumberToString"
	48, // "OS_ServiceCall"
	27, // "OS_SetCallBack"
	18, // "OS_SetEnv"
	36, // "OS_SetVarVal"
	46, // "OS_SpriteOp"
	67, // "OS_SubstituteArgs"
	25, // "OS_UnusedSWI"
	51, // "OS_UpCall"
	26, // "OS_UpdateMEMC"
	58, // "OS_ValidateAddress"
	7, // "OS_Word"
	2, // "OS_Write0"
	0, // "OS_WriteC"
	72, // "OS_WriteEnv"
	70, // "OS_WriteN"
	1, // "OS_WriteS"
};
