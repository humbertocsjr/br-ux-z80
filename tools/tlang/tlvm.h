
/*

TLANG 16 Bit VM Registers

BA32 = B16(High Word) A16(Low Word)
A16 = Word
B16 = Word
A8 = Byte
B8 = Byte

All 32 Bit Math/Logic are external functions.

Eg.:

b16 = 0x0123
a16 = 0x4567
push ba32
b16 = 0x0000
a16 = 0x0010
push ba32
call mul32
result ba32 <- 0x12345670
result b16 <- 0x1234
result a16 <- 0x5670

*/

enum
{
    CG_NONE,
    CG_SECTION_CODE,
    CG_SECTION_DATA,
    CG_SECTION_STRINGS,
    CG_NUM_LABEL,
    CG_NAME_LABEL,
    CG_PUBLIC_LABEL,
    CG_EMIT_NUM_LABEL_ADDRESS,
    CG_EMIT_STRING_ASCIZ,
    CG_FILL_WITH_ZEROS,
    CG_FUNCTION_START,
    CG_FUNCTION_END,
    CG_LOCAL_VAR,
    CG_GLOBAL_VAR,
    CG_ARG_VAR,
    CG_PUSH_A8,
    CG_PUSH_A16,
    CG_PUSH_B8,
    CG_PUSH_B16,
    CG_PUSH_BA32,
    CG_POP_A8,
    CG_POP_A16,
    CG_POP_B8,
    CG_POP_B16,
    CG_POP_BA32,
    CG_SET_A8,
    CG_SET_A16,
    CG_SET_B8,
    CG_SET_B16,
    CG_SET_A16_AS_NUM_LABEL_PTR,
    CG_SET_B16_AS_NUM_LABEL_PTR,
    CG_SET_A16_AS_LOCAL_PTR,
    CG_SET_B16_AS_LOCAL_PTR,
    CG_SET_A16_AS_GLOBAL_PTR,
    CG_SET_B16_AS_GLOBAL_PTR,
    CG_COPY_A8_TO_B8,
    CG_COPY_B8_TO_A8,
    CG_COPY_A16_TO_B16,
    CG_COPY_B16_TO_A16,
    CG_CONV_A8_TO_A16,
    CG_CONV_B8_TO_B16,
    CG_CONV_A8_TO_BA32,
    CG_CONV_A16_TO_A8,
    CG_CONV_B16_TO_B8,
    CG_CONV_A16_TO_BA32,
    CG_CONV_BA32_TO_A8,
    CG_CONV_BA32_TO_A16,
    CG_XCHG_A8_B8,
    CG_XCHG_A16_B16,
    CG_ADD_8,
    CG_ADD_16,
    CG_SUB_8,
    CG_SUB_16,
    CG_MUL_8,
    CG_MUL_16,
    CG_DIV_8,
    CG_DIV_16,
    CG_MOD_8,
    CG_MOD_16,
    CG_SHL_8,
    CG_SHL_16,
    CG_SHR_8,
    CG_SHR_16,
    CG_SET_IF_EQ_16,
    CG_SET_IF_NE_16,
    CG_SET_IF_LT_16,
    CG_SET_IF_LE_16,
    CG_SET_IF_GT_16,
    CG_SET_IF_GE_16,
    CG_JMP_IF_TRUE,
    CG_JMP_IF_FALSE,
    CG_JMP,
    CG_PRE_CALL,
    CG_CALL,
    CG_POST_CALL,
    CG_OUT_PORT,
    CG_IN_PORT,
    CG_LOAD_8,
    CG_LOAD_16,
    CG_LOAD_32,
    CG_STORE_8,
    CG_STORE_16,
    CG_STORE_32_FROM_STACK,
    CG_ASM,
};