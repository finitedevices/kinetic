#include "kinetic.h"

void kinetic_init(Kinetic* vm, KReadHandler readHandler, KWriteHandler writeHandler) {
    vm->main = vm->protect = (KineticContext) {
        .ip = 0, .csp = 0, .dsp = 0,
        .a = 0, .b = 0, .c = 0,
        .offset = 0, .limit = -1, .fuel = 0
    };

    vm->currentContext = &vm->main;
    vm->readHandler = readHandler;
    vm->writeHandler = writeHandler;
    vm->interruptHandler = 0;
}

KByte readByte(Kinetic* vm, KWord addr) {
    if (addr >= vm->currentContext->limit) return 0;
    return vm->readHandler(vm, addr + vm->currentContext->offset);
}

void writeByte(Kinetic* vm, KWord addr, KByte data) {
    if (addr >= vm->currentContext->limit) return;
    vm->writeHandler(vm, addr + vm->currentContext->offset, data);
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

    if ((mode & 0x03) == KMODE_LIT) {
        word = mode & KMODE_WORD ? readWord(vm, vm->currentContext->ip++) : readByte(vm, vm->currentContext->ip++);
        if (mode & KMODE_WORD) vm->currentContext->ip++;
    } else if ((mode & 0x03) == KMODE_MEM) {
        KWord addr = readByte(vm, vm->currentContext->ip++) | (readByte(vm, vm->currentContext->ip++) << 8);
        word = mode & KMODE_WORD ? readWord(vm, addr) : readByte(vm, addr);
    } else if ((mode & 0x03) == KMODE_A) {
        word = vm->currentContext->a & (mode & KMODE_WORD ? 0xFFFF : 0xFF);
    } else if ((mode & 0x03) == KMODE_B) {
        word = vm->currentContext->b & (mode & KMODE_WORD ? 0xFFFF : 0xFF);
    }

    return word;
}

void handleInterrupt(Kinetic* vm, KWord code) {
    if (vm->currentContext == &vm->protect) {
        vm->currentContext = &vm->main;
        vm->main.a = vm->protect.a;
        vm->main.b = code;
        return;
    }

    if (vm->interruptHandler && vm->interruptHandler(vm, code)) {
        return;
    }

    switch (code) {
        case KINT_SETUP:
            vm->protect.offset = readWord(vm, vm->main.a);
            vm->protect.limit = readWord(vm, vm->main.a + 2);
            vm->protect.fuel = readWord(vm, vm->main.a + 4);
            return;
        case KINT_PROTECT:
            vm->protect.a = vm->main.a;
            vm->currentContext = &vm->protect;
            return;
        case KINT_SAVE:
            writeWord(vm, vm->main.a, vm->protect.ip);
            writeWord(vm, vm->main.a + 2, vm->protect.csp);
            writeWord(vm, vm->main.a + 4, vm->protect.dsp);
            writeWord(vm, vm->main.a + 6, vm->protect.a);
            writeWord(vm, vm->main.a + 8, vm->protect.b);
            writeByte(vm, vm->main.a + 10, vm->protect.c);
            writeWord(vm, vm->main.a + 12, vm->protect.fuel);
            return;
        case KINT_LOAD:
            vm->protect.ip = readWord(vm, vm->main.a);
            vm->protect.csp = readWord(vm, vm->main.a + 2);
            vm->protect.dsp = readWord(vm, vm->main.a + 4);
            vm->protect.a = readWord(vm, vm->main.a + 6);
            vm->protect.b = readWord(vm, vm->main.a + 8);
            vm->protect.c = readByte(vm, vm->main.a + 10);
            vm->protect.fuel = readWord(vm, vm->main.a + 12);
            return;
        default: break;
    }
}

void kinetic_push(Kinetic* vm, KMode mode, KWord value) {
    KineticContext* ctx = vm->currentContext;

    ctx->dsp--;

    if (mode & KMODE_WORD) {
        ctx->dsp--;
        writeWord(vm, ctx->dsp, value);
    } else {
        writeByte(vm, ctx->dsp, value);
    }
}

KWord kinetic_pop(Kinetic* vm, KMode mode) {
    KineticContext* ctx = vm->currentContext;
    KWord result = mode & KMODE_WORD ? readWord(vm, ctx->dsp) : readByte(vm, ctx->dsp);
    ctx->dsp++;
    if (mode & KMODE_WORD) ctx->dsp++;
    return result;
}

void kinetic_step(Kinetic* vm) {
    KineticContext* ctx = vm->currentContext;
    KByte instr = readByte(vm, ctx->ip++);
    KByte opcode = instr & 0xF8, mode = instr & 0x07;
    KWord result, operand, addr;
    KDWord multiplyResult;

    if (vm->currentContext == &vm->protect && vm->protect.fuel == 0) {
        vm->currentContext = &vm->main;
        vm->main.a = 0;
        vm->main.b = 2;
        return;
    }

    if (vm->protect.fuel > 0) {
        vm->protect.fuel--;
    }

    switch (opcode) {
        case KOP_RET:
            ctx->ip = readWord(vm, ctx->csp); ctx->csp += 2; break;
        case KOP_POP:
            ctx->a = kinetic_pop(vm, mode); break;
        case KOP_PUSH:
            kinetic_push(vm, mode, getOperand(vm, mode)); break;
        case KOP_IDXA:
            ctx->a += getOperand(vm, mode);
            ctx->a = readWord(vm, ctx->a);
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
            operand = getOperand(vm, mode);
            ctx->c = ctx->a < operand;
            ctx->a -= operand;
            break;
        case KOP_ADD:
            result = ctx->a + getOperand(vm, mode);
            ctx->c = result < ctx->a;
            ctx->a = result;
            break;
        case KOP_DIV:
            operand = getOperand(vm, mode);
            if (operand == 0) ctx->a = 0;
            else ctx->a /= operand;
            break;
        case KOP_MUL:
            multiplyResult = ctx->a * getOperand(vm, mode);
            ctx->c = !!(multiplyResult >> 16);
            ctx->a = multiplyResult;
            break;
        case KOP_OR:
            ctx->a |= getOperand(vm, mode); break;
        case KOP_XOR:
            ctx->a ^= getOperand(vm, mode); break;
        case KOP_NOT:
            ctx->a = !ctx->a; break;
        case KOP_EQ:
            ctx->a = ctx->a == getOperand(vm, mode); break;
        case KOP_MOD:
            operand = getOperand(vm, mode);
            if (operand == 0) ctx->a = 0;
            else ctx->a %= operand;
            break;
        case KOP_GTN:
            ctx->a = ctx->a > getOperand(vm, mode); break;
        case KOP_BSR:
            operand = getOperand(vm, mode);
            ctx->c = operand & 1;
            ctx->a >>= operand;
            break;
        case KOP_DEC:
            ctx->a--; break;
        case KOP_LTN:
            ctx->a = ctx->a < getOperand(vm, mode); break;
        case KOP_BSL:
            operand = getOperand(vm, mode);
            ctx->c = !!(operand & 0x8000);
            ctx->a <<= operand;
            break;
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
            ctx->a = ctx->c; break;
        case KOP_INT:
            handleInterrupt(vm, getOperand(vm, mode)); break;
        case KOP_CLRA:
            ctx->a = 0; break;
        case KOP_CLRB:
            ctx->b = 0; break;
        default: break;
    }
}