# Kinetic
A simple 16-bit VM-based runtime architecture designed for ease of portability.

## Instruction set summary
Instructions are variable in length (depending on the operand mode), but the first byte contains the opcode and the operand mode, and the subsequent bytes contain either the operand (in the case of _literal_ mode) or the address of the operand value to use in memory (in the case of _memory_ mode). When using a register as an operand (_register_ mode), then the instruction will only be one byte long, as the operand mode specifies which register (A or B) to use.

The most significant 5 bits of first byte specify the opcode:

| Opcode  | Mnem.  | Opcode  | Mnem.  | Opcode  | Mnem.  | Opcode  | Mnem.  |
|---------|--------|---------|--------|---------|--------|---------|--------|
| `00000` | `ret`  | `01000` | `sub`  | `10000` | `eq`   | `11000` | `jump` |
| `00001` | `pop`  | `01001` | `add`  | `10001` | `mod`  | `11001` | `call` |
| `00010` | `push` | `01010` | `div`  | `10010` | `gtn`  | `11010` | `stcb` |
| `00011` | `idxa` | `01011` | `mul`  | `10011` | `bsr`  | `11011` | `int`  |
| `00100` | `lda`  | `01100` | `or`   | `10100` | `dec`  | `11100` | `clra` |
| `00101` | `ldb`  | `01101` | `xor`  | `10101` | `ltn`  | `11101` | `clrb` |
| `00110` | `sta`  | `01110` | `and`  | `10110` | `bsl`  | `11110` | `cif`  |
| `00111` | `stb`  | `01111` | `not`  | `10111` | `inc`  | `11111` | `if`   |

The least significant 3 bits of first byte specify the operand mode:

| Mode  | Operand interpretation                                   |
|-------|----------------------------------------------------------|
| `000` | Read next byte as literal                                |
| `001` | Read next word as memory address to read as byte operand |
| `010` | Use low byte of register A as byte operand               |
| `011` | Use low byte of register B as byte operand               |
| `100` | Read next word as literal                                |
| `101` | Read next word as memory address to read as word operand |
| `110` | Use register A as word operand                           |
| `111` | Use register B as word operand                           |