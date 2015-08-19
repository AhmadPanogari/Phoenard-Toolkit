#include "stk500registers.h"

bool ChipRegisters::registerInfoInit = false;
ChipRegisterInfo ChipRegisters::registerInfo[CHIPREG_BUFFSIZE];
ChipRegisterInfo* ChipRegisters::registerInfoByIndex[CHIPREG_COUNT];
ChipRegisterInfo ChipRegisters::registerInfoHeader;
QList<PinMapInfo> ChipRegisters::pinMapInfo;
PinMapInfo ChipRegisters::pinMapInfoHeader;

void ChipRegisters::initRegisters() {
    if (registerInfoInit) return;
    registerInfoInit = true;

    // Default entry if none exists
    ChipRegisterInfo defaultEntry;

    // Read all entries
    QList<ChipRegisterInfo> entries;
    QFile registersFile(":/data/registers.csv");
    if (registersFile.open(QIODevice::ReadOnly)) {
        QTextStream textStream(&registersFile);
        while (!textStream.atEnd()) {
            // Read the next entry
            ChipRegisterInfo entry(textStream.readLine().split(","));

            // Handle different entry types
            if (entry.address == "OTHER") {
                // OTHER default constant
                defaultEntry = entry;
            } else if (entry.addressValue == -1) {
                // Header - no valid address
                registerInfoHeader = entry;
            } else {
                // Entry with valid address
                entries.append(entry);
            }
        }
        registersFile.close();
    }

    // Initialize all register info entries to the default values
    for (int addr = 0; addr < CHIPREG_BUFFSIZE; addr++) {
        registerInfo[addr] = defaultEntry;
        registerInfo[addr].addressValue = addr;
        registerInfo[addr].address = stk500::getHexText(addr);
        registerInfo[addr].values[0] = registerInfo[addr].address;
    }

    // Store all entries at the addresses
    int index = 0;
    for (ChipRegisterInfo &entry : entries) {
        int addr = entry.addressValue;
        if ((addr >= 0) && (addr < CHIPREG_COUNT)) {
            registerInfo[addr] = entry;
            registerInfo[addr].index = index;
            registerInfoByIndex[index] = &registerInfo[addr];
            index++;
        }
    }

    // Fill the remaining entries with the 'OTHER' registers
    for (int i = CHIPREG_ADDR_START; i < CHIPREG_BUFFSIZE; i++) {
        if (registerInfo[i].index == -1) {
            registerInfo[i].index = index;
            registerInfoByIndex[index] = &registerInfo[i];
            index++;
        }
    }

    // Load all registers into a list
    pinMapInfo = QList<PinMapInfo>();
    QFile pinmapFile(":/data/pinmapping.csv");
    if (pinmapFile.open(QIODevice::ReadOnly)) {
        QTextStream textStream(&pinmapFile);
        while (!textStream.atEnd()) {
            // Read the next entry
            PinMapInfo entry(textStream.readLine().split(","));
            if (entry.pin == -1) {
                // Header
                pinMapInfoHeader = entry;
            } else {
                // Add valid entry
                pinMapInfo.append(entry);
            }
        }
        pinmapFile.close();
    }
}

/* Pin Mapping information element initialization and logic */

PinMapInfo::PinMapInfo() {
    load(QStringList());
}

PinMapInfo::PinMapInfo(const QStringList &values) {
    load(values);
}

void PinMapInfo::load(const QStringList &values) {
    for (int i = 0; i < PINMAP_DATA_COLUMNS; i++) {
        this->values[i] = (i < values.count()) ? values[i] : "-";
    }

    // Parse the pin number
    bool pinSucc = false;
    this->pin = this->values[0].toInt(&pinSucc, 10);
    if (!pinSucc) this->pin = -1;

    // Textual values
    this->port = this->values[1];
    this->name = this->values[2];
    this->module = this->values[3];
    this->function = this->values[4];

    // Parse port name into the addresses and mask to use
    this->addr_pin = this->addr_ddr = this->addr_port = -1;
    this->addr_mask = 0;
    if (this->port.length() == 2) {
        this->addr_mask = (1 << this->port.right(1).toInt());
        this->addr_pin = ChipRegisters::findRegisterAddress("PIN" + this->port[0]);
        this->addr_ddr = ChipRegisters::findRegisterAddress("DDR" + this->port[0]);
        this->addr_port = ChipRegisters::findRegisterAddress("PORT" + this->port[0]);
    }
}

/* Chip register information element initialization and logic */

ChipRegisterInfo::ChipRegisterInfo() {
    load(QStringList());
}

ChipRegisterInfo::ChipRegisterInfo(const QStringList &values) {
    load(values);
}

void ChipRegisterInfo::load(const QStringList &values) {
    for (int i = 0; i < CHIPREG_DATA_COLUMNS; i++) {
        this->values[i] = (i < values.count()) ? values[i] : "-";
    }

    this->index = -1;
    this->address = this->values[0];
    this->name = this->values[1];
    this->module = this->values[2];
    this->function = this->values[3];
    for (int i = 0; i < 8; i++) {
        this->bitNames[7-i] = this->values[i + 4];
    }
    bool succ = false;
    this->addressValue = this->address.toInt(&succ, 16);
    if (!succ) this->addressValue = -1;
}

/* Chip register container initialization and properties */

ChipRegisters::ChipRegisters() {
    memset(regData, 0, sizeof(regData));
    memset(regDataLast, 0, sizeof(regDataLast));
    memset(regDataRead, 0, sizeof(regDataRead));
    memset(regDataError, 0, sizeof(regDataError));
}

bool ChipRegisters::changed(int address) {
    return regDataLast[address] != regData[address];
}

quint8 ChipRegisters::error(int address) {
    return regDataError[address];
}

bool ChipRegisters::hasUserChanges() {
    return memcmp(regDataRead, regData, sizeof(regData)) != 0;
}

void ChipRegisters::resetUserChanges() {
    memcpy(regData, regDataRead, sizeof(regData));
}

const ChipRegisterInfo &ChipRegisters::info(int address) {
    initRegisters();
    if ((address >= CHIPREG_ADDR_START) && (address < CHIPREG_BUFFSIZE)) {
        return registerInfo[address];
    } else {
        return registerInfoHeader;
    }
}

const ChipRegisterInfo &ChipRegisters::infoByIndex(int index) {
    initRegisters();
    if ((index >= 0) && (index < CHIPREG_COUNT)) {
        return *registerInfoByIndex[index];
    } else {
        return registerInfoHeader;
    }
}

const ChipRegisterInfo &ChipRegisters::infoHeader() {
    initRegisters();
    return registerInfoHeader;
}

const QList<PinMapInfo> &ChipRegisters::pinMap() {
    initRegisters();
    return pinMapInfo;
}

int ChipRegisters::findRegisterAddress(const QString &name) {
    for (int i = 0; i < CHIPREG_COUNT; i++) {
        if (registerInfoByIndex[i]->name == name) {
            return registerInfoByIndex[i]->addressValue;
        }
    }
    return -1;
}

/* stk500 Register handler functions */

void stk500registers::write(ChipRegisters &registers) {
    // For any bytes that differ, write the byte to the chip
    // Only write out the bits that changed for added safety
    // When done, reset the value to the read one
    for (int addr = CHIPREG_ADDR_START; addr < CHIPREG_BUFFSIZE; addr++) {
        quint8 changeMask = registers.regData[addr] ^ registers.regDataRead[addr];
        if (changeMask) {
            _handler->RAM_writeByte(addr, registers[addr], changeMask);
        }
    }
}

void stk500registers::read(ChipRegisters &registers) {
    // Read the latest registers
    char reg[CHIPREG_COUNT];
    _handler->RAM_read(CHIPREG_ADDR_START, reg, CHIPREG_COUNT);

    // Store the previously read values into the last registers
    memcpy(registers.regDataLast, registers.regDataRead, sizeof(registers.regDataLast));

    // Refresh the registers
    memcpy(registers.regDataRead + CHIPREG_ADDR_START, reg, CHIPREG_COUNT);

    // Compare the last written values to the newly read values
    // If there is a difference, we failed to write those particular bits
    for (int i = CHIPREG_ADDR_START; i < CHIPREG_BUFFSIZE; i++) {
        if (registers.regDataLast[i] != registers.regData[i]) {
            registers.regDataError[i] = (registers.regDataRead[i] ^ registers.regData[i]);
        } else {
            registers.regDataError[i] = 0;
        }
    }

    // Synchronize the current register values with the last ones read
    memcpy(registers.regData, registers.regDataRead, sizeof(registers.regData));
}
