#include "stk500registers.h"

bool ChipRegisters::registerInfoInit = false;
ChipRegisterInfo ChipRegisters::registerInfo[CHIPREG_BUFFSIZE];
ChipRegisterInfo* ChipRegisters::registerInfoByIndex[CHIPREG_COUNT];
ChipRegisterInfo ChipRegisters::registerInfoHeader;
QList<PinMapInfo> ChipRegisters::pinmapInfo;
PinMapInfo ChipRegisters::pinmapInfoHeader;

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

    for (int i = 0; i < entries.count(); i++) {
        ChipRegisterInfo &entry = entries[i];
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
    pinmapInfo = QList<PinMapInfo>();
    QFile pinmapFile(":/data/pinmapping.csv");
    if (pinmapFile.open(QIODevice::ReadOnly)) {
        QTextStream textStream(&pinmapFile);
        while (!textStream.atEnd()) {
            // Read the next entry
            PinMapInfo entry(textStream.readLine().split(","));
            if (entry.pin == -1) {
                // Header
                pinmapInfoHeader = entry;
            } else {
                // Add valid entry
                pinmapInfo.append(entry);
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
    this->pin = this->values[4].toInt(&pinSucc, 10);
    if (!pinSucc) this->pin = -1;

    // Textual values
    this->name = this->values[3];
    this->module = this->values[1];
    this->function = this->values[2];
    this->port = this->values[0];

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
    this->module = this->values[1];
    this->function = this->values[2];
    this->name = this->values[3];
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
    analogDataIndex = 0;
    regDataWasRead = false;
}

bool ChipRegisters::changed(int address) {
    // If not read before, the last registers indicate access flags
    if (!regDataWasRead) {
        return (regDataLast[address] == 0xFF);
    }

    // General check
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

void ChipRegisters::resetChanges() {
    memcpy(regDataLast, regDataRead, sizeof(regDataRead));
}

void ChipRegisters::applyUserChanges(const ChipRegisters& other) {
    for (int i = 0; i < CHIPREG_BUFFSIZE; i++) {
        quint8 changeMask = other.regData[i] ^ other.regDataRead[i];
        if (changeMask) {
            this->regData[i] &= ~changeMask;
            this->regData[i] |= (other.regData[i] & changeMask);
        }
    }
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

const QList<PinMapInfo> &ChipRegisters::pinmap() {
    initRegisters();
    return pinmapInfo;
}

const PinMapInfo &ChipRegisters::pinmapHeader() {
    initRegisters();
    return pinmapInfoHeader;
}

int ChipRegisters::findRegisterAddress(const QString &name) {
    initRegisters();
    for (int i = 0; i < CHIPREG_COUNT; i++) {
        if (registerInfoByIndex[i]->name == name) {
            return registerInfoByIndex[i]->addressValue;
        }
    }
    return -1;
}

quint8& ChipRegisters::operator [](int i) {
    // Keep track of access if not read before
    if (!regDataWasRead) regDataLast[i] = 0xFF;

    // Return reference to data
    return regData[i];
}
quint8 ChipRegisters::operator [](const QString &name) const {
    int idx = findRegisterAddress(name);
    return (idx==-1) ? 0 : regData[idx];
}

quint8& ChipRegisters::operator [](const QString &name) {
    int idx = findRegisterAddress(name);
    if (idx == -1) idx = 0x1FF;
    return (*this)[idx];
}

bool ChipRegisters::getChangedRange(int* address, int* count) {
    bool found = false;
    for (int i = CHIPREG_ADDR_START; i < CHIPREG_BUFFSIZE; i++) {
        if (changed(i)) {
            // First element
            if (!found) {
                found = true;
                *address = i;
            }
            // Last changed element
            *count = (i - *address) + 1;
        }
    }
    return found;
}

void ChipRegisters::setupUART(int idx, qint32 baudRate) {
    // Calculate baud rate register value
    qreal F_CPU = 16000000;
    quint16 baudData = (quint16) (F_CPU/(((qreal) baudRate)*8.0)-0.5);

    // For baud values exceeding 0x0FFF, disable double-speed
    quint8 UCSRA = 0x62;
    if (baudData > 0x0FFF) {
        baudData /= 2;
        UCSRA &= ~0x2;
    }

    // Set the registers
    ChipRegisters &reg = *this;
    QString n = QString::number(idx, 10);
    reg[QString("UCSR%1A").arg(n)] = UCSRA;
    reg[QString("UCSR%1B").arg(n)] = 0x18;
    reg[QString("UCSR%1C").arg(n)] = 0x06;
    reg[QString("UBRR%1L").arg(n)] = (baudData & 0xFF);
    reg[QString("UBRR%1H").arg(n)] = (baudData >> 8);
}

/* stk500 Register handler functions */

void stk500registers::write(ChipRegisters &registers) {
    // For any bytes that differ, write the byte to the chip
    // Only write out the bits that changed for added safety
    // When done, reset the value to the read one
    if (registers.regDataWasRead) {
        // Compare old to new
        for (int addr = CHIPREG_ADDR_START; addr < CHIPREG_BUFFSIZE; addr++) {
            quint8 changeMask = registers.regData[addr] ^ registers.regDataRead[addr];
            if (changeMask) {
                _handler->RAM_writeByte(addr, registers[addr], changeMask);
            }
        }
    } else {
        // Use access flag
        for (int addr = CHIPREG_ADDR_START; addr < CHIPREG_BUFFSIZE; addr++) {
            if (registers.regDataLast[addr] == 0xFF) {
                _handler->RAM_writeByte(addr, registers[addr]);
            }
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

    // Mark as read
    registers.regDataWasRead = true;
}

void stk500registers::readADC(ChipRegisters &registers) {
    // Refresh the analog inputs
    for (int cnt = 0; cnt < ANALOG_PIN_INCREMENT; cnt++) {
        int pin = registers.analogDataIndex;
        registers.analogData[pin] = _handler->ANALOG_read(pin);
        registers.analogDataIndex = (pin + 1) % ANALOG_PIN_COUNT;
    }
}
