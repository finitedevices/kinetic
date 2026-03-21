#!/usr/bin/env -S deno -q run --allow-read --allow-write

const opcodes = {
    RET:    0b00000000,
    POP:    0b00001000,
    PUSH:   0b00010000,
    IDXA:   0b00011000,
    LDA:    0b00100000,
    LDB:    0b00101000,
    STA:    0b00110000,
    STB:    0b00111000,
    SUB:    0b01000000,
    ADD:    0b01001000,
    DIV:    0b01010000,
    MUL:    0b01011000,
    OR:     0b01100000,
    XOR:    0b01101000,
    AND:    0b01110000,
    NOT:    0b01111000,
    EQ:     0b10000000,
    MOD:    0b10001000,
    GTN:    0b10010000,
    BSR:    0b10011000,
    DEC:    0b10100000,
    LTN:    0b10101000,
    BSL:    0b10110000,
    INC:    0b10111000,
    JUMP:   0b11000000,
    CALL:   0b11001000,
    STCB:   0b11010000,
    INT:    0b11011000,
    CLRA:   0b11100000,
    CLRB:   0b11101000,
    CIF:    0b11110000,
    IF:     0b11111000
};

var bareInstructions = ["RET", "POP", "NOT", "DEC", "INC"];

var args = [...Deno.args];
var inputPath = null;
var outputPath = null;
var bytes = [];
var currentPath = null;
var currentLine = 0;

function die(message) {
    console.error(currentLine ? `${currentPath ?? "<stdin>"} line ${currentLine}: ${message}` : message);

    Deno.exit(1);
}

function parseValue(part, word = false) {
    if (!part) {
        die("No value specified");
    }

    var match;

    if (match = part.match(/^(\d+)$/)) {
        var int = parseInt(match);

        if (word) {
            if (int > 0xFFFF) {
                die("Value too large to fit in word");
            }

            return [int & 0xFF, (int & 0xFF00) >> 8];
        } else {
            if (int > 0xFF) {
                die("Value too large to fit in byte");
            }

            return [int];
        }
    }

    if (word && (match = part.match(/^\$([0-9a-fA-F]{1,2})([0-9a-fA-F]{2})$/))) {
        return [parseInt(match[2], 16), parseInt(match[1], 16)];
    }

    if (match = part.match(/^\$([0-9a-fA-F]{1,2})$/)) {
        return word ? [parseInt(match[1], 16), 0] : [parseInt(match[1], 16)];
    }

    die("Syntax error");
}

function parseCode(path, code) {
    var lines = code.split("\n");
    var currentLocalLine = 0;

    lineLoop:
    for (var line of lines) {
        currentLocalLine++;
        currentPath = path;
        currentLine = currentLocalLine;

        var parts = line.split(";")[0].trim().split(/\s+/g);

        if (parts.join("") == "") {
            continue;
        }

        if (parts[0].toLowerCase() == ".byte") {
            bytes.push(...parseValue(parts[1]));
            continue;
        }

        if (parts[0].toLowerCase() == ".word") {
            bytes.push(...parseValue(parts[1], true));
            continue;
        }

        for (var mnemonic in opcodes) {
            var instructionPart = parts[0].toUpperCase();

            if (instructionPart.replace(/[wW]$/, "") == mnemonic) {
                var instruction = opcodes[mnemonic];
                var wordMode = instructionPart.toUpperCase().endsWith("W");

                if (wordMode) {
                    instruction |= 0b100;
                }

                if (bareInstructions.includes(mnemonic)) {
                    bytes.push(instruction);
                } else {
                    if (parts[1]?.toLowerCase() == "a") {
                        bytes.push(instruction | 0b010);
                    } else if (parts[1]?.toLowerCase() == "b") {
                        bytes.push(instruction | 0b011);
                    } else if (parts[1]?.startsWith("*")) {
                        bytes.push(instruction | 0b001);
                        bytes.push(...parseValue(parts[1].substring(1), true));
                    } else if (wordMode) {
                        bytes.push(instruction);
                        bytes.push(...parseValue(parts[1], true));
                    } else {
                        bytes.push(instruction);
                        bytes.push(...parseValue(parts[1]));
                    }
                }

                continue lineLoop;
            }
        }

        die("Syntax error");
    }
}

while (args.length > 0) {
    var arg = args.shift();

    if (arg == "-o") {
        if (outputPath) {
            die("Output path already specified");
        }

        outputPath = args.shift();

        continue;
    }

    if (inputPath) {
        die("Input path already specified");
    }

    inputPath = arg;
}

if (!inputPath) {
    die("No input path specified");
}

if (!outputPath) {
    die("No output path specified");
}

try {
    var inputFile = new TextDecoder().decode(await Deno.readFile(inputPath));
} catch (error) {
    die("Could not read file at input path: " + error.message);
}

parseCode(inputPath, inputFile);

await Deno.writeFile(outputPath, new Uint8Array(bytes));