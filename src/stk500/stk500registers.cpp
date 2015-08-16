#include "stk500registers.h"

ChipRegisters::ChipRegisters() {
    memset(regData, 0, sizeof(regData));
    memset(regDataLast, 0, sizeof(regDataLast));
}

bool ChipRegisters::changed(int index) {
    return regDataLast[index] != regData[index];
}

void ChipRegisters::resetChanges() {
    memcpy(regDataLast, regData, sizeof(regDataLast));
}

// Redefine the values to represent case statements inside the function
#undef CHIPREG_V
#define CHIPREG_V(index, name) case (index - CHIPREG_OFFSET) : return #name;

QString ChipRegisters::name(int index) {
    switch (index) {
    CHIPREG_ENUM_VALUES
    default: return "RESERVED";
    }
}

void stk500registers::write(const ChipRegisters &registers) {
    // For any bytes that differ, write the byte to the chip
    // Only write out the bits that changed for added safety
    // There is a possibility to write all bytes out in one go
    // For the moment, we do it safe and 'slow'
    for (int i = 0; i < registers.count(); i++) {
        quint8 changeMask = registers[i] ^ lastReg[i];
        if (changeMask) {
            lastReg[i] = _handler->RAM_writeByte(i + CHIPREG_OFFSET, registers[i], changeMask);
        }
    }
}

void stk500registers::read(ChipRegisters &registers) {
    // Clear any changes stored within
    registers.resetChanges();

    // Read the current registers
    _handler->RAM_read(CHIPREG_OFFSET, (char*) registers.data(), registers.count());

    // Update the last known register values
    lastReg = registers;
}
