#include "kinetic.h"

void kinetic_init(Kinetic* vm, KReadHandler readHandler, KWriteHandler writeHandler) {
    vm->main = vm->protect = (KineticContext) {
        .ip = 0, .csp = 0, .dsp = 0, .isr = 0,
        .a = 0, .b = 0, .c = 0,
        .offset = 0, .limit = -1
    };

    vm->currentContext = &vm->main;
    vm->readHandler = readHandler;
    vm->writeHandler = writeHandler;
    vm->interruptHandler = 0;
}

KByte readByte(Kinetic* vm, KWord addr) {
    if (addr >= vm->currentContext->limit) return 0;
    return vm->readHandler(addr + vm->currentContext->offset);
}

void writeByte(Kinetic* vm, KWord addr, KByte data) {
    if (addr >= vm->currentContext->limit) return;
    vm->writeHandler(addr + vm->currentContext->offset, data);
}

KWord readWord(Kinetic* vm, KWord addr) {
    return readByte(vm, addr) | (readByte(vm, addr + 1) << 8);
}

void writeWord(Kinetic* vm, KWord addr, KWord word) {
    writeByte(vm, addr, word & 0xFF);
    writeByte(vm, addr + 1, (word & 0xFF00) >> 8);
}

KWord getOperand(Kinetic* vm, KMode mode) {
    KWord word = 0;

    if (mode == KMODE_LIT) {
        word = mode & KMODE_WORD ? readWord(vm, vm->currentContext->ip++) : readByte(vm, vm->currentContext->ip++);
        if (mode & KMODE_WORD) vm->currentContext->ip++;
    } else if (mode == KMODE_MEM) {
        KWord addr = readByte(vm, vm->currentContext->ip++) | (readByte(vm, vm->currentContext->ip++) << 8);
        word = mode & KMODE_WORD ? readWord(vm, addr) : readByte(vm, addr);
    } else if (mode == KMODE_A) {
        word = vm->currentContext->a & (mode & KMODE_WORD ? 0xFFFF : 0xFF);
    } else if (mode == KMODE_B) {
        word = vm->currentContext->b & (mode & KMODE_WORD ? 0xFFFF : 0xFF);
    }

    return word;
}

void kinetic_step(Kinetic* vm) {
    KineticContext* ctx = vm->currentContext;
    KByte instr = readByte(vm, ctx->ip++);
    KByte opcode = instr & 0xF8, mode = instr & 0x07;
    KWord operand, addr;

    switch (opcode) {
        case KOP_RET:
            ctx->ip = readWord(vm, ctx->csp); ctx->csp += 2; break;
        case KOP_POP:
            ctx->a = mode & KMODE_WORD ? readWord(vm, ctx->dsp) : readByte(vm, ctx->dsp);
            ctx->dsp++;
            if (mode & KMODE_WORD) ctx->dsp++;
            break;
        case KOP_MOD:
            operand = getOperand(vm, mode);
            if (operand == 0) ctx->a = 0;
            else ctx->a %= operand;
            break;
        case KOP_PUSH:
            ctx->dsp--;
            if (mode & KMODE_WORD) ctx->dsp--;
            writeWord(vm, ctx->dsp, getOperand(vm, mode));
            break;
        case KOP_LDA:
            ctx->a = getOperand(vm, mode); break;
        case KOP_LDB:
            ctx->b = getOperand(vm, mode); break;
        case KOP_STA:
            if (mode & KMODE_WORD) writeWord(vm, getOperand(vm, mode), ctx->a);
            else writeByte(vm, getOperand(vm, mode), ctx->a);
            break;
        case KOP_STB:
            if (mode & KMODE_WORD) writeWord(vm, getOperand(vm, mode), ctx->b);
            else writeByte(vm, getOperand(vm, mode), ctx->b);
            break;
        case KOP_SUB:
            ctx->a -= getOperand(vm, mode); break;
        case KOP_ADD:
            ctx->a += getOperand(vm, mode); break;
        case KOP_DIV:
            operand = getOperand(vm, mode);
            if (operand == 0) ctx->a = 0;
            else ctx->a /= operand;
            break;
        case KOP_MUL:
            ctx->a *= getOperand(vm, mode); break;
        case KOP_OR:
            ctx->a |= getOperand(vm, mode); break;
        case KOP_XOR:
            ctx->a ^= getOperand(vm, mode); break;
        case KOP_NOT:
            ctx->a = !ctx->a;
        case KOP_NEQ:
            ctx->a = ctx->a != getOperand(vm, mode); break;
        case KOP_EQ:
            ctx->a = ctx->a == getOperand(vm, mode); break;
        case KOP_GTN:
            ctx->a = ctx->a > getOperand(vm, mode); break;
        case KOP_BSR:
            ctx->a >>= getOperand(vm, mode); break;
        case KOP_DEC:
            ctx->a--; break;
        case KOP_LTN:
            ctx->a = ctx->a < getOperand(vm, mode); break;
        case KOP_BSL:
            ctx->a <<= getOperand(vm, mode); break;
        case KOP_INC:
            ctx->a++; break;
        case KOP_IF:
            if (!ctx->a) break; // Fall through if true
        case KOP_JUMP:
            ctx->ip = getOperand(vm, mode); break;
        case KOP_CIF:
            if (!ctx->a) break; // Fall through if true
        case KOP_CALL:
            addr = getOperand(vm, mode);
            ctx->csp -= 2;
            writeWord(vm, ctx->csp, ctx->ip);
            ctx->ip = addr;
            break;
        case KOP_STCB:
            // TODO: Implement
            break;
        case KOP_INT:
            // TODO: Implement
            break;
        case KOP_CLRA:
            ctx->a = 0; break;
        case KOP_CLRB:
            ctx->b = 0; break;
    }
}