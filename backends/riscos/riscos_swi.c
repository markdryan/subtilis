/*
 * Copyright (c) 2020-2021 Mark Ryan
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

#include "riscos_swi.h"

/* clang-format off */

const subtilis_arm_swi_t subtilis_riscos_swi_list[] = {
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
	{ 0x50, "OS_ExitAndDie", 0xf, 0x0 },
	{ 0x51, "OS_ReadMemMapInfo", 0x0, 0x3 },
	{ 0x52, "OS_ReadMemMapEntries", 0x1, 0x0 },
	{ 0x53, "OS_SetMemMapEntries", 0x1, 0x0 },
	{ 0x54, "OS_AddCallBack", 0x3, 0x0 },
	{ 0x55, "OS_ReadDefaultHandler", 0x1, 0xe },
	{ 0x56, "OS_SetECFOrigin", 0x3, 0x0 },
	{ 0x57, "OS_SerialOp", 0x7, 0x6 },
	{ 0x58, "OS_ReadSysInfo", 0x1, 0x1f },
	{ 0x59, "OS_Confirm", 0x0, 0x1 },
	{ 0x5A, "OS_ChangedBox", 0x1, 0x3 },
	{ 0x5B, "OS_CRC", 0xf, 0x1 },
	{ 0x5C, "OS_ReadDynamicArea", 0x1, 0x7 },
	{ 0x5D, "OS_PrintChar", 0x1, 0x0 },
	{ 0x5E, "OS_ChangeRedirection", 0x3, 0x3 },
	{ 0x5F, "OS_RemoveCallBack", 0x3, 0x0 },
	{ 0x60, "OS_FindMemMapEntries", 0x1, 0x0 },
	{ 0x61, "OS_SetColour", 0x3, 0x0 },
	{ 0x64, "OS_Pointer", 0x15e, 0x1c },
	{ 0x65, "OS_ScreenMode", 0xc7, 0xc6 },
	{ 0x66, "OS_DynamicArea", 0x1ff, 0x1fe },
	{ 0x68, "OS_Memory", 0x3, 0x6 },
	{ 0x69, "OS_ClaimProcessorVector", 0x7, 0x2 },
	{ 0x6A, "OS_Reset", 0x0, 0x0 },
	{ 0x6B, "OS_MMUControl", 0x7, 0x6 },
	{ 0xC0, "OS_ConvertStandardDateAndTime", 0x7, 0x7 },
	{ 0xC1, "OS_ConvertDateAndTime", 0xf, 0x7 },
	{ 0xD0, "OS_ConvertHex1", 0x7, 0x7 },
	{ 0xD1, "OS_ConvertHex2", 0x7, 0x7 },
	{ 0xD2, "OS_ConvertHex3", 0x7, 0x7 },
	{ 0xD3, "OS_ConvertHex4", 0x7, 0x7 },
	{ 0xD4, "OS_ConvertHex8", 0x7, 0x7 },
	{ 0xD5, "OS_ConvertCardinal1", 0x7, 0x7 },
	{ 0xD6, "OS_ConvertCardinal2", 0x7, 0x7 },
	{ 0xD7, "OS_ConvertCardinal3", 0x7, 0x7 },
	{ 0xD8, "OS_ConvertCardinal4", 0x7, 0x7 },
	{ 0xD9, "OS_ConvertInteger1", 0x7, 0x7 },
	{ 0xDA, "OS_ConvertInteger2", 0x7, 0x7 },
	{ 0xDB, "OS_ConvertInteger3", 0x7, 0x7 },
	{ 0xDC, "OS_ConvertInteger4", 0x7, 0x7 },
	{ 0xDD, "OS_ConvertBinary1", 0x7, 0x7 },
	{ 0xDE, "OS_ConvertBinary2", 0x7, 0x7 },
	{ 0xDF, "OS_ConvertBinary3", 0x7, 0x7 },
	{ 0xE0, "OS_ConvertBinary4", 0x7, 0x7 },
	{ 0xE1, "OS_ConvertSpacedCardinal1", 0x7, 0x7 },
	{ 0xE2, "OS_ConvertSpacedCardinal2", 0x7, 0x7 },
	{ 0xE3, "OS_ConvertSpacedCardinal3", 0x7, 0x7 },
	{ 0xE4, "OS_ConvertSpacedCardinal4", 0x7, 0x7 },
	{ 0xE5, "OS_ConvertSpacedInteger1", 0x7, 0x7 },
	{ 0xE6, "OS_ConvertSpacedInteger2", 0x7, 0x7 },
	{ 0xE7, "OS_ConvertSpacedInteger3", 0x7, 0x7 },
	{ 0xE8, "OS_ConvertSpacedInteger4", 0x7, 0x7 },
	{ 0xE9, "OS_ConvertFixedNetStation", 0x7, 0x7 },
	{ 0xEA, "OS_ConvertNetStation", 0x7, 0x7 },
	{ 0xEB, "OS_ConvertFixedFileSize", 0x7, 0x7 },
	{ 0xEC, "OS_ConvertFileSize", 0x7, 0x7 },
	{ 0x40080, "Font_CacheAddr", 0x0, 0xD },
	{ 0x40081, "Font_FindFont", 0x3e, 0x31 },
	{ 0x40082, "Font_LoseFont", 0x1, 0x0 },
	{ 0x40083, "Font_ReadDefn", 0xb, 0xfc },
	{ 0x40084, "Font_ReadInfo", 0x1, 0x1e },
	{ 0x40085, "Font_StringWidth", 0x3e, 0x3e },
	{ 0x40086, "Font_Paint", 0xff, 0x0 },
	{ 0x40087, "Font_Caret", 0x1f, 0x0 },
	{ 0x40088, "Font_ConverttoOS", 0x6, 0x6 },
	{ 0x40089, "Font_Converttopoints", 0x6, 0x7 },
	{ 0x4008A, "Font_SetFont", 0x1, 0x0 },
	{ 0x4008B, "Font_CurrentFont", 0x0, 0xf },
	{ 0x4008C, "Font_FutureFont", 0x0, 0xf },
	{ 0x4008D, "Font_FindCaret", 0xe, 0x3e },
	{ 0x4008E, "Font_CharBBox", 0x7, 0x1e },
	{ 0x4008F, "Font_ReadScaleFactor", 0x0, 0x6 },
	{ 0x40090, "Font_SetScaleFactor", 0x6, 0x0 },
	{ 0x40091, "Font_ListFonts", 0x7e, 0x2c },
	{ 0x40092, "Font_SetFontColours", 0xf, 0x0 },
	{ 0x40093, "Font_SetPalette", 0x7e, 0x0 },
	{ 0x40094, "Font_ReadThresholds", 0x2, 0x0 },
	{ 0x40095, "Font_SetThresholds", 0x2, 0x0 },
	{ 0x40096, "Font_FindCaretJ", 0x3e, 0x3e },
	{ 0x40097, "Font_StringBBox", 0x2, 0x1e },
	{ 0x40098, "Font_ReadColourTable", 0x2, 0x0 },
	{ 0x40099, "Font_MakeBitmap", 0x7e, 0x0 },
	{ 0x4009A, "Font_UnCacheFile", 0x6, 0x0 },
	{ 0x4009B, "Font_SetFontMax", 0xff, 0x0 },
	{ 0x4009C, "Font_ReadFontMax", 0x0, 0xff },
	{ 0x4009D, "Font_ReadFontPrefix", 0x7, 0x6 },
	{ 0x4009E, "Font_SwitchOutputToBuffer", 0x3, 0x3 },
	{ 0x4009F, "Font_ReadFontMetrics", 0xff, 0xff },
	{ 0x400A0, "Font_DecodeMenu", 0x1f, 0x1c },
	{ 0x400A1, "Font_ScanString", 0xff, 0x9a },
	{ 0x400A3, "Font_CurrentRGB", 0x0, 0xf },
	{ 0x400A4, "Font_FutureRGB", 0x0, 0xf },
	{ 0x400A5, "Font_ReadEncodingFilename", 0x7, 0x7 },
	{ 0x400A6, "Font_FindField", 0x6, 0x6 },
	{ 0x400A7, "Font_ApplyFields", 0xf, 0x8 },
	{ 0x400A8, "Font_LookupFont", 0x7, 0xfc },
	{ 0x400C0, "Wimp_Initialise", 0xf, 0x3 },
	{ 0x400C1, "Wimp_CreateWindow", 0x2, 0x1 },
	{ 0x400C2, "Wimp_CreateIcon", 0x3, 0x1 },
	{ 0x400C3, "Wimp_DeleteWindow", 0x2, 0x2 },
	{ 0x400C4, "Wimp_DeleteIcon", 0x2, 0x2 },
	{ 0x400C5, "Wimp_OpenWindow", 0x2f, 0x6 },
	{ 0x400C6, "Wimp_CloseWindow", 0xe, 0x1 },
	{ 0x400C7, "Wimp_Poll", 0xb, 0x7 },
	{ 0x400C8, "Wimp_RedrawWindow", 0x2, 0x1 },
	{ 0x400C9, "Wimp_UpdateWindow", 0x2, 0x1 },
	{ 0x400CA, "Wimp_GetRectangle", 0x2, 0x1 },
	{ 0x400CB, "Wimp_GetWindowState", 0x6, 0xe },
	{ 0x400CC, "Wimp_GetWindowInfo", 0x2, 0x2 },
	{ 0x400CD, "Wimp_SetIconState", 0x2, 0x1 },
	{ 0x400CE, "Wimp_GetIconState", 0x2, 0x2 },
	{ 0x400CF, "Wimp_GetPointerInfo", 0x2, 0x1 },
	{ 0x400D0, "Wimp_DragBox", 0xe, 0x1 },
	{ 0x400D1, "Wimp_ForceRedraw", 0x1f, 0x1 },
	{ 0x400D2, "Wimp_SetCaretPosition", 0x3f, 0x0 },
	{ 0x400D3, "Wimp_GetCaretPosition", 0x2, 0x1 },
	{ 0x400D4, "Wimp_CreateMenu", 0xe, 0x1 },
	{ 0x400D5, "Wimp_DecodeMenu", 0xe, 0x1 },
	{ 0x400D6, "Wimp_WhichIcon", 0xf, 0x1 },
	{ 0x400D7, "Wimp_SetExtent", 0x3, 0x1 },
	{ 0x400D8, "Wimp_SetPointerShape", 0x3f, 0x1 },
	{ 0x400D9, "Wimp_OpenTemplate", 0x2, 0x1 },
	{ 0x400DA, "Wimp_CloseTemplate", 0x0, 0x1 },
	{ 0x400DB, "Wimp_LoadTemplate", 0x7e, 0x7d },
	{ 0x400DC, "Wimp_ProcessKey", 0x1, 0x1 },
	{ 0x400DD, "Wimp_CloseDown", 0x3, 0x1 },
	{ 0x400DE, "Wimp_StartTask", 0x1, 0x1 },
	{ 0x400DF, "Wimp_ReportError", 0x3f, 0x3 },
	{ 0x400E0, "Wimp_GetWindowOutline", 0x2, 0x3 },
	{ 0x400E1, "Wimp_PollIdle", 0xf, 0x7 },
	{ 0x400E2, "Wimp_PlotIcon", 0x36, 0x1 },
	{ 0x400E3, "Wimp_SetMode", 0x1, 0x1 },
	{ 0x400E4, "Wimp_SetPalette", 0x2, 0x1 },
	{ 0x400E5, "Wimp_ReadPalette", 0x6, 0x1 },
	{ 0x400E6, "Wimp_SetColour", 0x1, 0x1 },
	{ 0x400E7, "Wimp_SendMessage", 0xf, 0x5 },
	{ 0x400E8, "Wimp_CreateSubMenu", 0xe, 0x1 },
	{ 0x400E9, "Wimp_SpriteOp", 0xff, 0x7e },
	{ 0x400EA, "Wimp_BaseOfSprites", 0x0, 0x3 },
	{ 0x400EB, "Wimp_BlockCopy", 0x7f, 0x0 },
	{ 0x400EC, "Wimp_SlotSize", 0x3, 0x7 },
	{ 0x400ED, "Wimp_ReadPixTrans", 0xc7, 0xc1 },
	{ 0x400EE, "Wimp_ClaimFreeMemory", 0x3, 0x7, },
	{ 0x400EF, "Wimp_CommandWindow", 0x1, 0x1 },
	{ 0x400F0, "Wimp_TextColour", 0x1, 0x1 },
	{ 0x400F1, "Wimp_TransferBlock", 0x1f, 0x1 },
	{ 0x400F2, "Wimp_ReadSysInfo", 0x1, 0x3 },
	{ 0x400F3, "Wimp_SetFontColours", 0x6, 0x1 },
	{ 0x400F4, "Wimp_GetMenuState", 0xf, 0x3 },
	{ 0x400F5, "Wimp_RegisterFilter", 0x0, 0x0 },
	{ 0x400F6, "Wimp_AddMessages", 0x1, 0x0 },
	{ 0x400F7, "Wimp_RemoveMessages", 0x0 },
	{ 0x400F8, "Wimp_SetColourMapping", 0xff, 0x0 },
	{ 0x400F9, "Wimp_TextOp", 0x1, 0x1 },
	{ 0x400FA, "Wimp_SetWatchdogState", 0x3, 0x0 },
	{ 0x400FB, "Wimp_Extend", 0x1, 0x0 },
	{ 0x400FC, "Wimp_ResizeIcon", 0x3f, 0x0 },
};

const size_t subtilis_riscos_known_swis = sizeof(subtilis_riscos_swi_list) /
	sizeof(subtilis_arm_swi_t);

const size_t subtilis_riscos_swi_index[] = {
	174, // "Font_ApplyFields"
	136, // "Font_CacheAddr"
	143, // "Font_Caret"
	150, // "Font_CharBBox"
	144, // "Font_ConverttoOS"
	145, // "Font_Converttopoints"
	147, // "Font_CurrentFont"
	170, // "Font_CurrentRGB"
	168, // "Font_DecodeMenu"
	149, // "Font_FindCaret"
	158, // "Font_FindCaretJ"
	173, // "Font_FindField"
	137, // "Font_FindFont"
	148, // "Font_FutureFont"
	171, // "Font_FutureRGB"
	153, // "Font_ListFonts"
	175, // "Font_LookupFont"
	138, // "Font_LoseFont"
	161, // "Font_MakeBitmap"
	142, // "Font_Paint"
	160, // "Font_ReadColourTable"
	139, // "Font_ReadDefn"
	172, // "Font_ReadEncodingFilename"
	164, // "Font_ReadFontMax"
	167, // "Font_ReadFontMetrics"
	165, // "Font_ReadFontPrefix"
	140, // "Font_ReadInfo"
	151, // "Font_ReadScaleFactor"
	156, // "Font_ReadThresholds"
	169, // "Font_ScanString"
	146, // "Font_SetFont"
	154, // "Font_SetFontColours"
	163, // "Font_SetFontMax"
	155, // "Font_SetPalette"
	152, // "Font_SetScaleFactor"
	157, // "Font_SetThresholds"
	159, // "Font_StringBBox"
	141, // "Font_StringWidth"
	166, // "Font_SwitchOutputToBuffer"
	162, // "Font_UnCacheFile"
	84, // "OS_AddCallBack"
	71, // "OS_AddToVector"
	9, // "OS_Args"
	10, // "OS_BGet"
	11, // "OS_BPut"
	40, // "OS_BinaryToDecimal"
	24, // "OS_BreakCtrl"
	23, // "OS_BreakPt"
	6, // "OS_Byte"
	5, // "OS_CLI"
	91, // "OS_CRC"
	52, // "OS_CallAVector"
	59, // "OS_CallAfter"
	21, // "OS_CallBack"
	60, // "OS_CallEvery"
	42, // "OS_ChangeDynamicArea"
	64, // "OS_ChangeEnvironment"
	94, // "OS_ChangeRedirection"
	90, // "OS_ChangedBox"
	63, // "OS_CheckModeValid"
	31, // "OS_Claim"
	75, // "OS_ClaimDeviceVector"
	102, // "OS_ClaimProcessorVector"
	65, // "OS_ClaimScreenMemory"
	89, // "OS_Confirm"
	15, // "OS_Control"
	120, // "OS_ConvertBinary1"
	121, // "OS_ConvertBinary2"
	122, // "OS_ConvertBinary3"
	123, // "OS_ConvertBinary4"
	112, // "OS_ConvertCardinal1"
	113, // "OS_ConvertCardinal2"
	114, // "OS_ConvertCardinal3"
	115, // "OS_ConvertCardinal4"
	106, // "OS_ConvertDateAndTime"
	135, // "OS_ConvertFileSize"
	134, // "OS_ConvertFixedFileSize"
	132, // "OS_ConvertFixedNetStation"
	107, // "OS_ConvertHex1"
	108, // "OS_ConvertHex2"
	109, // "OS_ConvertHex3"
	110, // "OS_ConvertHex4"
	111, // "OS_ConvertHex8"
	116, // "OS_ConvertInteger1"
	117, // "OS_ConvertInteger2"
	118, // "OS_ConvertInteger3"
	119, // "OS_ConvertInteger4"
	133, // "OS_ConvertNetStation"
	124, // "OS_ConvertSpacedCardinal1"
	125, // "OS_ConvertSpacedCardinal2"
	126, // "OS_ConvertSpacedCardinal3"
	127, // "OS_ConvertSpacedCardinal4"
	128, // "OS_ConvertSpacedInteger1"
	129, // "OS_ConvertSpacedInteger2"
	130, // "OS_ConvertSpacedInteger3"
	131, // "OS_ConvertSpacedInteger4"
	105, // "OS_ConvertStandardDateAndTime"
	77, // "OS_DelinkApplication"
	100, // "OS_DynamicArea"
	22, // "OS_EnterOS"
	45, // "OS_EvaluateExpression"
	17, // "OS_Exit"
	80, // "OS_ExitAndDie"
	41, // "OS_FSControl"
	8, // "OS_File"
	13, // "OS_Find"
	96, // "OS_FindMemMapEntries"
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
	104, // "OS_MMUControl"
	101, // "OS_Memory"
	30, // "OS_Module"
	28, // "OS_Mouse"
	3, // "OS_NewLine"
	69, // "OS_Plot"
	98, // "OS_Pointer"
	68, // "OS_PrettyPrint"
	93, // "OS_PrintChar"
	73, // "OS_ReadArgs"
	4, // "OS_ReadC"
	85, // "OS_ReadDefaultHandler"
	92, // "OS_ReadDynamicArea"
	44, // "OS_ReadEscapeState"
	14, // "OS_ReadLine"
	82, // "OS_ReadMemMapEntries"
	81, // "OS_ReadMemMapInfo"
	53, // "OS_ReadModeVariable"
	66, // "OS_ReadMonotonicTime"
	47, // "OS_ReadPalette"
	50, // "OS_ReadPoint"
	74, // "OS_ReadRAMFsLimits"
	88, // "OS_ReadSysInfo"
	33, // "OS_ReadUnsigned"
	35, // "OS_ReadVarVal"
	49, // "OS_ReadVduVariables"
	32, // "OS_Release"
	76, // "OS_ReleaseDeviceVector"
	78, // "OS_RelinkApplication"
	95, // "OS_RemoveCallBack"
	54, // "OS_RemoveCursors"
	61, // "OS_RemoveTickerEvent"
	103, // "OS_Reset"
	55, // "OS_RestoreCursors"
	57, // "OS_SWINumberFromString"
	56, // "OS_SWINumberToString"
	99, // "OS_ScreenMode"
	87, // "OS_SerialOp"
	48, // "OS_ServiceCall"
	27, // "OS_SetCallBack"
	97, // "OS_SetColour"
	86, // "OS_SetECFOrigin"
	18, // "OS_SetEnv"
	83, // "OS_SetMemMapEntries"
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
	230, // "Wimp_AddMessages"
	218, // "Wimp_BaseOfSprites"
	219, // "Wimp_BlockCopy"
	222, // "Wimp_ClaimFreeMemory"
	205, // "Wimp_CloseDown"
	202, // "Wimp_CloseTemplate"
	182, // "Wimp_CloseWindow"
	223, // "Wimp_CommandWindow"
	178, // "Wimp_CreateIcon"
	196, // "Wimp_CreateMenu"
	216, // "Wimp_CreateSubMenu"
	177, // "Wimp_CreateWindow"
	197, // "Wimp_DecodeMenu"
	180, // "Wimp_DeleteIcon"
	179, // "Wimp_DeleteWindow"
	192, // "Wimp_DragBox"
	235, // "Wimp_Extend"
	193, // "Wimp_ForceRedraw"
	195, // "Wimp_GetCaretPosition"
	190, // "Wimp_GetIconState"
	228, // "Wimp_GetMenuState"
	191, // "Wimp_GetPointerInfo"
	186, // "Wimp_GetRectangle"
	188, // "Wimp_GetWindowInfo"
	208, // "Wimp_GetWindowOutline"
	187, // "Wimp_GetWindowState"
	176, // "Wimp_Initialise"
	203, // "Wimp_LoadTemplate"
	201, // "Wimp_OpenTemplate"
	181, // "Wimp_OpenWindow"
	210, // "Wimp_PlotIcon"
	183, // "Wimp_Poll"
	209, // "Wimp_PollIdle"
	204, // "Wimp_ProcessKey"
	213, // "Wimp_ReadPalette"
	221, // "Wimp_ReadPixTrans"
	226, // "Wimp_ReadSysInfo"
	184, // "Wimp_RedrawWindow"
	229, // "Wimp_RegisterFilter"
	231, // "Wimp_RemoveMessages"
	207, // "Wimp_ReportError"
	236, // "Wimp_ResizeIcon"
	215, // "Wimp_SendMessage"
	194, // "Wimp_SetCaretPosition"
	214, // "Wimp_SetColour"
	232, // "Wimp_SetColourMapping"
	199, // "Wimp_SetExtent"
	227, // "Wimp_SetFontColours"
	189, // "Wimp_SetIconState"
	211, // "Wimp_SetMode"
	212, // "Wimp_SetPalette"
	200, // "Wimp_SetPointerShape"
	234, // "Wimp_SetWatchdogState"
	220, // "Wimp_SlotSize"
	217, // "Wimp_SpriteOp"
	206, // "Wimp_StartTask"
	224, // "Wimp_TextColour"
	233, // "Wimp_TextOp"
	225, // "Wimp_TransferBlock"
	185, // "Wimp_UpdateWindow"
	198, // "Wimp_WhichIcon"
};
