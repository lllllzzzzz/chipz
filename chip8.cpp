#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "chip8.h"

static const unsigned char fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
    0x20, 0x60, 0x20, 0x20, 0x70,   // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
    0xF0, 0x80, 0xF0, 0x80, 0x80    // F
};

static const unsigned char chip8_font8x10[160] =
{
    0x00, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00,   //0
    0x00, 0x08, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3E, 0x00,   //1
    0x00, 0x38, 0x44, 0x04, 0x08, 0x10, 0x20, 0x44, 0x7C, 0x00,   //2
    0x00, 0x38, 0x44, 0x04, 0x18, 0x04, 0x04, 0x44, 0x38, 0x00,   //3
    0x00, 0x0C, 0x14, 0x24, 0x24, 0x7E, 0x04, 0x04, 0x0E, 0x00,   //4
    0x00, 0x3E, 0x20, 0x20, 0x3C, 0x02, 0x02, 0x42, 0x3C, 0x00,   //5
    0x00, 0x0E, 0x10, 0x20, 0x3C, 0x22, 0x22, 0x22, 0x1C, 0x00,   //6
    0x00, 0x7E, 0x42, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00,   //7
    0x00, 0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x42, 0x3C, 0x00,   //8
    0x00, 0x3C, 0x42, 0x42, 0x42, 0x3E, 0x02, 0x04, 0x78, 0x00,   //9
    0x00, 0x18, 0x08, 0x14, 0x14, 0x14, 0x1C, 0x22, 0x77, 0x00,   //A
    0x00, 0x7C, 0x22, 0x22, 0x3C, 0x22, 0x22, 0x22, 0x7C, 0x00,   //B
    0x00, 0x1E, 0x22, 0x40, 0x40, 0x40, 0x40, 0x22, 0x1C, 0x00,   //C
    0x00, 0x78, 0x24, 0x22, 0x22, 0x22, 0x22, 0x24, 0x78, 0x00,   //D
    0x00, 0x7E, 0x22, 0x28, 0x38, 0x28, 0x20, 0x22, 0x7E, 0x00,   //E
    0x00, 0x7E, 0x22, 0x28, 0x38, 0x28, 0x20, 0x20, 0x70, 0x00    //F
};

Chip8::Chip8()
{
}

Chip8::~Chip8()
{
}

unsigned char* Chip8::getMainMemory()
{
    return state_.mainMemory;
}

unsigned char* Chip8::getVideoMemory()
{
    return state_.videoMemory;
}

unsigned char Chip8::getDelayTimer()
{
    return state_.regs.delayTimer;
}

unsigned char Chip8::getSoundTimer()
{
    return state_.regs.soundTimer;
}

unsigned int Chip8::getFlags()
{
    return flags_;
}

void Chip8::setFlags(unsigned int flags)
{
    this->flags_ = flags;
}

bool Chip8::getFlag(unsigned int flag)
{
    return flags_ & flag;
}

void Chip8::setFlag(unsigned int flag)
{
    this->flags_ |= flag;
}

void Chip8::resetFlag(unsigned int flag)
{
    flags_ &= ~flag;
}

void Chip8::toggleFlag(unsigned int flag)
{
    flags_ = (flags_ & flag) ? flags_ & ~flag : flags_ | flag;
}

unsigned char* Chip8::getKeyInput()
{
    return keyState_;
}

void Chip8::setKeys(char keyState[16])
{
    //_keyInput[key] = 0;
    memcpy(this->keyState_, keyState, 16);

//    for (int i = 0; i < 16; i++)
//    {
//        if (keyState_[i] == 1)
//        {
//            flags_ & CPU_FLAG_KEYDOWN;
//            return;
//        }
//    }
//    flags_ & ~CPU_FLAG_KEYDOWN;
}

void Chip8::tickDelayTimer()
{
    if (state_.regs.delayTimer > 0) {
       state_.regs.delayTimer--;
    }
}

void Chip8::tickSoundTimer()
{
    if (state_.regs.soundTimer > 0) {
//      if (state_.regs.soundTimer == 1) {
//            PlayBeep();
//            Beep(500, 200);
//      }
       state_.regs.soundTimer--;
    }
}

//void Chip8::playBeep()
//{
//
//}

unsigned short Chip8::getProgramCounter()
{
    return state_.regs.pc;
}

unsigned short Chip8::getOpcode(unsigned short address)
{
    if (address < CHIP8_ROM_ADDRESS || address > 0x0FFF) {
        #ifdef DEBUG
        printf("Error: address 0x%04X outside memory range.\n", address);
        #endif // DEBUG
        return 0;
    } else {
        return (state_.mainMemory[address] << 8) | state_.mainMemory[address + 1];
    }
}

unsigned short Chip8::getRomSize()
{
    return romSize_;
}

registers Chip8::getRegisters()
{
    return state_.regs;
}

unsigned short* Chip8::getStack()
{
    return state_.stack;
}

void Chip8::debug()
{
    for (int i = 0; i < 16; i++) {
        printf("V%01X:%02X ", i, state_.regs.V[i]);
    }
    printf("\n");
}

/*|===============================================================================|
  |  InitializeCpu()                                                              |
  |                                                                               |
  |  Initialize the state of the emulator. Sets program counter to ROM entry      |
  |  point (0x200) and initializes the flags bitmask to default values.           |
  |   All registers, timers and memory are reset.                                 |
  |-------------------------------------------------------------------------------|*/
bool Chip8::initializeCpu()
{
   state_.regs.pc = CHIP8_ROM_ADDRESS;
   state_.regs.sp = 0;
   state_.regs.opcode = 0;
   state_.regs.I = 0;

   state_.regs.delayTimer = 0;
   state_.regs.soundTimer = 0;

   memset(state_.mainMemory, 0, CHIP8_MEM_SIZE);
   memset(state_.videoMemory, 0, CHIP8_VIDMEM_SIZE);
   memset(state_.regs.V, 0, CHIP8_NUM_REGISTERS);
   memset(state_.stack, 0, CHIP8_STACK_SIZE * 2);
   memcpy(state_.mainMemory, fontset, CHIP8_FONTSET_SIZE);

   flags_ = CPU_FLAG_DETECTCOLLISION | CPU_FLAG_HWRAP | CPU_FLAG_VWRAP;

   srand(time(NULL));

   return true;
}

/*|===============================================================================|
  |  ResetCpu()                                                                   |
  |                                                                               |
  |  Reset the state_ of the emulator. This method is the same as                 |
  |  InitializeCpu(), except a reset does not clear RAM, so a game can be         |
  |  restarted without loading the rom again.                                     |
  |-------------------------------------------------------------------------------|*/
bool Chip8::resetCpu()
{
   state_.regs.pc = CHIP8_ROM_ADDRESS;
   state_.regs.sp = 0;
   state_.regs.opcode = 0;
   state_.regs.I = 0;

   state_.regs.delayTimer = 0;
   state_.regs.soundTimer = 0;

   memset(state_.videoMemory, 0, CHIP8_VIDMEM_SIZE);
   memset(state_.regs.V, 0, CHIP8_NUM_REGISTERS);
   memset(state_.stack, 0, CHIP8_STACK_SIZE * 2);

   // Set default flags
   //flags_ = CPU_FLAG_DETECTCOLLISION | CPU_FLAG_HWRAP | CPU_FLAG_VWRAP;

   srand(time(NULL));

   return true;
}

/*|===============================================================================|
  |  LoadRom(const char *szFilename)                                              |
  |                                                                               |
  |  Load a Chip8/SCHIP rom at 0x200. Chip8 supports 4096 bytes of memory,        |
  |  So a rom can be no larger than 3584 bytes (0xFFF - 0x200).                   |
  |-------------------------------------------------------------------------------|*/
bool Chip8::loadRom(const char *romPath)
{
    FILE* pFile = fopen(romPath, "rb");

    if (!pFile) {
        return false;
    }

    fseek(pFile, 0, SEEK_END);
    romSize_ = ftell(pFile);
    rewind(pFile);

    if (romSize_ > CHIP8_MAX_ROM_SIZE) {
        fclose(pFile);
        return false;
    }

    size_t bytesRead = fread(&state_.mainMemory[CHIP8_ROM_ADDRESS], sizeof (unsigned char), romSize_, pFile);

    if (bytesRead != romSize_) {
        fclose(pFile);
        return false;
    }

    fclose(pFile);
    return true;
}

bool Chip8::unloadRom()
{
    return true;
}

/*|===============================================================================|
  |  SaveState()                                                                  |
  |                                                                               |
  |  Save the state of the emulator, copying registers, memory and the stack      |
  |  into a save state. This will save game progress, which can then be loaded    |
  |  later on.                                                                    |
  |-------------------------------------------------------------------------------|*/
void Chip8::saveCpuState()
{
    saveState_.regs = state_.regs;
    memcpy(saveState_.mainMemory, state_.mainMemory, CHIP8_MEM_SIZE);
    memcpy(saveState_.videoMemory, state_.videoMemory, CHIP8_VIDMEM_WIDTH * CHIP8_VIDMEM_HEIGHT);
    memcpy(saveState_.stack, state_.stack, CHIP8_STACK_SIZE * 2);
}

/*|===============================================================================|
  |  LoadState()                                                                  |
  |                                                                               |
  |  Load the state of the emulator, copying registers, memory and the stack      |
  |  from a save state. This will load a game save.                               |
  |-------------------------------------------------------------------------------|*/
void Chip8::loadCpuState()
{
    state_.regs = saveState_.regs;
    memcpy(state_.mainMemory, saveState_.mainMemory, CHIP8_MEM_SIZE);
    memcpy(state_.videoMemory, saveState_.videoMemory, CHIP8_VIDMEM_WIDTH * CHIP8_VIDMEM_HEIGHT);
    memcpy(state_.stack, saveState_.stack, CHIP8_STACK_SIZE * 2);
}

/*|===============================================================================|
  |  00CN: Scroll screen N lines down.                                      SCHIP |
  |                                                                               |
  |  Scroll the display N lines down. Some SCHIP games use this instruction       |
  |  for vertical scrolling.                                                      |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_00CN()
{
    for (int y = 0; y < (64 - N); y++) {
        for (int x = 0; x < 128; x++) {
            state_.videoMemory[((y + N) * 64) + x] = state_.mainMemory[(y * 64) + x];
            state_.videoMemory[(y * 64) + x] = 0;
        }
    }

   state_.regs.pc += 2;
}

/*|===============================================================================|
  |  00EE: Return from subroutine.                                                |
  |                                                                               |
  |  Stack pointer is decremented, and program counter is popped off the stack.   |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_00EE()
{
    if (state_.regs.sp == 0) {
        #ifdef DEBUG
        printf("0x%04X: Stack empty, cannot return from subroutine!\n", pc);
        #endif // DEBUG
        resetCpu();
    } else {
        state_.regs.sp--;
        state_.regs.pc = state_.stack[state_.regs.sp];
        state_.stack[state_.regs.sp] = 0;
    }
}

/*|===============================================================================|
  |  00E0: Clear screen.                                                          |
  |                                                                               |
  |  Reset 2048 bytes of video memory, clearing the screen.                       |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_00E0()
{
    memset(state_.videoMemory, 0, 8196);
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  00FB: Scroll screen 4 pixels right.                                    SCHIP |
  |                                                                               |
  |  Scroll the display 4 pixels to the right. This instruction performs          |
  |  horizontal scrolling e.g. Ant.                                               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_00FB()
{
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128 - 4; x++) {
            state_.videoMemory[(y * 64) + x + 4] = state_.mainMemory[(y * 64) + x];
            state_.videoMemory[(y * 64) + x] = 0;
        }
    }

   state_.regs.pc += 2;
}

/*|===============================================================================|
  |  00FC: Scroll screen 4 pixels left.                                     SCHIP |
  |                                                                               |
  |  Scroll the display 4 pixels to the left. This instruction performs           |
  |  horizontal scrolling.                                                        |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_00FC()
{
    for (int y = 0; y < 64; y++) {
        for (int x = 4; x < 128; x++) {
            state_.videoMemory[(y * 64) + x - 4] = state_.mainMemory[(y * 64) + x];
            state_.videoMemory[(y * 64) + x] = 0;
        }
    }

   state_.regs.pc += 2;
}

/*|===============================================================================|
  |  00FD: Exit the interpreter.                                            SCHIP |
  |                                                                               |
  |  Exit the SCHIP interpreter, stopping execution.                              |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_00FD()
{
    initializeCpu();
}

/*|===============================================================================|
  |  00FE: Disable extended screen mode.                                    SCHIP |
  |                                                                               |
  |  Disable extended screen mode used by SCHIP games (128x64) and revert         |
  |  to original Chip8 resolution (64x32).                                        |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_00FE()
{
    flags_ &= ~CPU_FLAG_SCHIP;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  00FF: Enable extended screen mode for full screen.                     SCHIP |
  |                                                                               |
  |  Enable extended screen mode (128x64) for full screen.                        |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_00FF()
{
    flags_ |= CPU_FLAG_SCHIP;
//    memcpy(state_.mainMemory, chip8_font8x10, sizeof(chip8_font8x10));
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  1NNN: Jump to address NNN                                                    |
  |                                                                               |
  |  Program counter is set to address NNN.                                       |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_1NNN()
{
    if (NNN < CHIP8_ROM_ADDRESS || NNN > CHIP8_MEM_SIZE - 3) {
        #ifdef DEBUG
        printf("0x%04X: Illegal jump, resetting rom...\n", pc);
        #endif // DEBUG
        resetCpu();
    } else if (flags_ & CPU_FLAG_DETECTGAMEOVER && state_.regs.pc == NNN) {
        #ifdef DEBUG
        printf("0x%04X: Game over detected, resetting rom...\n", pc);
        #endif // DEBUG
        resetCpu();
    } else if (NNN < CHIP8_ROM_ADDRESS || NNN > CHIP8_MEM_SIZE - 3) {
        #ifdef DEBUG
        printf("0x%04X: Illegal jump, resetting rom...\n", pc);
        #endif // DEBUG
        resetCpu();
    } else {
       state_.regs.pc = NNN;
    }
}

/*|===============================================================================|
  |  2NNN: Call subroutine.                                                       |
  |                                                                               |
  |  The current value of the program counter + 2 is pushed on the stack,         |
  |  storing the address of the next instruction which will be executed when the  |
  |  subroutine returns. The stack pointer is incremented, creating stack space   |
  |  for another call. The program counter is then set to NNN, which is the       |
  |  address of the subroutine.                                                   |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_2NNN()
{
    if (NNN << CHIP8_ROM_ADDRESS || NNN >> CHIP8_MEM_SIZE - 3) {
        #ifdef DEBUG
        printf("0x%04X: Illegal call, address out of memory range! Resetting rom...\n", pc);
        #endif // DEBUG
        resetCpu();
    }

    if (state_.regs.sp == 32) {
        #ifdef DEBUG
        printf("0x%04X: Stack full, cannot push return address! Resetting rom...\n", pc);
        #endif // DEBUG
        resetCpu();
    }

    state_.stack[state_.regs.sp] = state_.regs.pc + 2;
    state_.regs.sp++;
    state_.regs.pc = NNN;
}

/*|===============================================================================|
  |  3XNN: Skip the next opcode if Vx equals NN.                                  |
  |                                                                               |
  |  The next instruction is skipped if Vx and NN contain the same values.        |
  |  Otherwise, execute the instruction stored at the next 2 bytes.               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_3XNN()
{
    state_.regs.pc += (Vx == NN) ? 4 : 2;
}

/*|===============================================================================|
  |  4XNN: Skip the next opcode if Vx does NOT equal NN.                          |
  |                                                                               |
  |  The next instruction is skipped if Vx and NN contain different values.       |
  |  Otherwise, execute the instruction stored at the next 2 bytes.               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_4XNN()
{
    state_.regs.pc += (Vx != NN) ? 4 : 2;
}

/*|===============================================================================|
  |  5XNN: Skip the next opcode if Vx equals Vy.                                  |
  |                                                                               |
  |  The next instruction is skipped if Vx and Vy contain the same values.        |
  |  Otherwise, execute the instruction stored at the next 2 bytes.               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_5XNN()
{
    state_.regs.pc += (Vx == Vy) ? 4 : 2;
}

/*|===============================================================================|
  |  6XNN: Assign NN to Vx.                                                       |
  |                                                                               |
  |  Store the value NN in Vx.                                                    |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_6XNN()
{
    Vx = NN;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  7XNN: Increment Vx by NN.                                                    |
  |                                                                               |
  |  Add the value NN to Vx.                                                      |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_7XNN()
{
    Vx += NN;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  8XY0: Assign Vy to Vx.                                                       |
  |                                                                               |
  |  Store the value contained in Vy in Vx.                                       |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XY0()
{
    Vx = Vy;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  8XY1: Vx logical OR Vy.                                                      |
  |                                                                               |
  |  Logical OR Vx by Vy, storing the new value in Vx.                            |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XY1()
{
    Vx |= Vy;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  8XY2: Vx logical AND Vy.                                                     |
  |                                                                               |
  |  Logical AND Vx by Vy, storing the new value in Vx.                           |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XY2()
{
    Vx &= Vy;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  8XY3: Vx logical XOR Vy.                                                     |
  |                                                                               |
  |  Logical XOR Vx by Vy, storing the new value in Vx.                           |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XY3()
{
    Vx ^= Vy;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  8XY4: Add Vy to Vx.                                                          |
  |                                                                               |
  |  Incremented Vx by Vy. Set Vf if the operation results in a carry,            |
  |  i.e. the result is greater than 0xFF. Otherwise reset Vf.                    |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XY4()
{
    Vf = (Vx + Vy) > 0xFF;
    Vx += Vy;
    //Vf = Vx > 0xFF;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  8XY5: Subtract Vy from Vx.                                                   |
  |                                                                               |
  |  Subtract Vy from Vx. Set Vf if the operation results in a borrow,            |
  |  i.e. the result is negative. Otherwise reset Vf.                             |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XY5()
{
    Vf = Vx > Vy;
    Vx -= Vy;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |   8XY6: Set Vf to LSB of Vx, then right shift Vx.                             |
  |                                                                               |
  |   Set Vf to the least significant bit of Vx, then right shift                 |
  |   Vx one digit.                                                               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XY6()
{
    Vf = Vx & 0x1;
    Vx >>= 1;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |   8XY7: Set Vx to Vy minus Vx.                                                |
  |                                                                               |
  |   Subtract Vx from Vy, and store the result in Vx.                            |
  |   Set Vf if the operation results in a borrow, i.e. the result is negative.   |
  |   Otherwise reset Vf.                                                         |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XY7()
{
    Vf = Vy > Vx;
    Vx = Vy - Vx;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  8XYE: Set Vf to MSB of Vx, then left shift Vx.                               |
  |                                                                               |
  |  Set Vf to the most significant bit of Vx, then left shift Vx one digit.      |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_8XYE()
{
    Vf = Vx >> 7;
    Vx <<= 1;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  9XY0: Skip the next opcode if Vx does NOT equal Vy                           |
  |                                                                               |
  |  The next instruction is skipped if Vx and Vy do NOT contain the same values. |
  |  Otherwise, execute the instruction stored at the next 2 bytes.               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_9XY0()
{
    state_.regs.pc += (Vx != Vy) ? 4 : 2;
}

/*|===============================================================================|
  |  ANNN: Load I with the value of NNN.                                          |
  |                                                                               |
  |  Load index register I with address NNN. The value of I is used by a few      |
  |  instructions, such as DXYN which draws sprites to video memory.              |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_ANNN()
{
    state_.regs.I = NNN;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  BNNN: Jump to address NNN + V0.                                              |
  |                                                                               |
  |  Load the program counter with the address formed by adding the value         |
  |  of V0 to NNN. NNN is a base address and V0 is added as an offset.            |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_BNNN()
{
    if (NNN + state_.regs.V[0x0] < CHIP8_ROM_ADDRESS || NNN + state_.regs.V[0x0] > CHIP8_MEM_SIZE - 3) {
        #ifdef DEBUG
        printf("0x%04X: Illegal jump, address out of memory range. Resetting rom...", pc);
        #endif // DEBUG
        resetCpu();
    } else {
       state_.regs.pc = NNN + state_.regs.V[0x0];
    }
}

/*|===============================================================================|
  |  CXNN: Set Vx to a random int (0-2550 AND NN.                                 |
  |                                                                               |
  |  Generate a random int between 0 and 255, logical AND this value by NN and    |
  |  store the result in Vx.                                                      |
  |  Using rand() for RNG, seeded with value of RTC (time(NULL)).                 |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_CXNN()
{
    Vx = (rand() % 255) & NN;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  DXYN: Draw a sprite at coordinates (Vx,Vy); height N.                        |
  |                                                                               |
  |  Draw a sprite to video memory. X and Y are the indexes of the registers      |
  |  containing the coordinates of the sprite, and N specifies the height of the  |
  |  sprite which is 16 pixels max.                                               |
  |                                                                               |
  |  Pixels are plotted by XOR'ing each byte of video memory. If the XOR          |
  |  operation results in a pixel being reset, that means the pixel was already   |
  |  set, which is detected as a collision. Collisions are signalled by setting   |
  |  Vf.                                                                          |
  |                                                                               |
  |  If horizontal/vertical sprite wrapping are enabled, a modulo operation is    |
  |  used to wrap pixel coordinates. Otherwise, pixels drawn outside the          |
  |  dimensions of the video memory (64x32) will not be visible on the screen.    |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_DXYN()
{
    unsigned char x = Vx;
    unsigned char y = Vy;
    unsigned char height = N;
    unsigned char row;

    if (height == 0) {
        height = 16;
    }

    // Set carry flag to 0
    Vf = 0;

    if (!(flags_ & CPU_FLAG_SCHIP)) {
        for (int yline = 0; yline < height; yline++) {
            // Sprite stored at address I
            row = state_.mainMemory[state_.regs.I + yline];
            for (int xline = 0; xline < 8; xline++) {
                // Shift pixels to draw row
                if ((row & (0x80 >> xline)) != 0) {
                    // If collision detection flag is set, check for collision
                    if (flags_ & CPU_FLAG_DETECTCOLLISION) {
                        if (state_.videoMemory[((yline + y) * 64) + xline + x] == 1) {
                            Vf = 1;
                        }
                    }

                    // Coordinates of pixel to write to vidmem
                    unsigned char pixelX = xline + x;
                    unsigned char pixelY = yline + y;

                    // Wrap pixel around x-axis if horizontal wrapping enabled
                    if (flags_ & CPU_FLAG_HWRAP) {
                        pixelX %= 64;
                    }

                    // Do not update pixel if pixelX exceeds size of x-axis
                    else if (pixelX >= 64) {
                        continue;
                    }

                    // Wrap pixel around y-axis if vertical wrapping enabled
                    if (flags_ & CPU_FLAG_VWRAP) {
                        pixelY %= 32;
                    }

                    // Do not update pixel if pixelY exceeds size of y-axis
                    else if (pixelY >= 32) {
                        continue;
                    }

                    // XOR this byte to set/reset pixel
                    //_state_.videoMemory[((yline + y) * 64) + (xline + x)] ^= 1;
                    state_.videoMemory[(pixelY * 64) + pixelX] ^= 1;
                }
            }
        }
    }

    else {
        for (int yline = 0; yline < height; yline++) {
            // Sprite stored at address I
            row = state_.mainMemory[state_.regs.I + yline];
            for (int xline = 0; xline < 8; xline++) {
                // Shift pixels to draw row
                if ((row & (0x80 >> xline)) != 0) {
                    // If collision detection flag is set, check for collision
                    if (flags_ & CPU_FLAG_DETECTCOLLISION) {
                        if (state_.videoMemory[((yline + y) * 64) + xline + x] == 1) {
                            Vf = 1;
                        }
                    }

                    // XOR this byte to set/reset pixel
                    state_.videoMemory[((yline + y) * 128) + (xline + x)] ^= 1;
                }
            }
        }
    }

    // All pixels plotted, now update screen
    flags_ |= CPU_FLAG_DRAW;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  EX9E: Skip the next instruction if key in Vx is pressed.                     |
  |                                                                               |
  |  The next instruction is skipped if the key specified by Vx is pressed.       |
  |  Otherwise, execute the instruction stored at the next 2 bytes.               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_EX9E()
{
    state_.regs.pc += (keyState_[Vx]) ? 4 : 2;
}

/*|===============================================================================|
  |  EXA1: Skip the next instruction if key in Vx is NOT pressed.                 |
  |                                                                               |
  |  The next instruction is skipped if the key specified by Vx is NOT pressed.   |
  |  Otherwise, execute the instruction stored at the next 2 bytes.               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_EXA1()
{
    state_.regs.pc += !(keyState_[Vx]) ? 4 : 2;
}

/*|===============================================================================|
  |  FX07: Set Vx to the value of the delay timer.                                |
  |                                                                               |
  |  Set Vx to the value of the delay timer.                                      |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX07()
{
    Vx = state_.regs.delayTimer;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX0A: Pause execution until key is pressed, store in Vx.                     |
  |                                                                               |
  |  Execution is paused at this instruction until a key is pressed.              |
  |  Wait until the key is pressed AND released; otherwise Connect4 and Guess     |
  |  are nearly unplayable because key input will be way too fast.                |
  |  Store pressed key in Vx.                                                     |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX0A()
{
    bool keyPress = false;

    for (int i = 0; i < 16; i++) {
        if (keyState_[i] == 1) {
            Vx = i;
            keyPress = true;
            break;
        }
    }

    if (!keyPress) {
        return;
    }

   state_.regs.pc += 2;

//    // Check if key is down
//    if (flags_ & ~CPU_FLAG_KEYDOWN) {
//        for (int i = 0; i < 16; i++) {
//            if (keyState_[i] == 1) {
//                Vx = i;
//                flags_ |= CPU_FLAG_KEYDOWN;
//                // return;
//            }
//        }
//        // return;
//    } else if (flags_ & CPU_FLAG_KEYDOWN) {   // Check if key is up
//        if (keyState_[Vx] == 0) {
//            flags_ &= ~CPU_FLAG_KEYDOWN;
//            state_.regs.pc += 2;
//        }
//    }
}

/*|===============================================================================|
  |  FX15: Set the delay timer to the value of Vx.                                |
  |                                                                               |
  |  Set the delay timer to the value of Vx.                                      |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX15()
{
    state_.regs.delayTimer = Vx;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX18: Set the sound timer to the value of Vx.                                |
  |                                                                               |
  |  Set the sound timer to the value of Vx.                                      |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX18()
{
    state_.regs.soundTimer = Vx;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX1E: Add Vx to I.                                                           |
  |                                                                               |
  |  Add the value of Vx to index register I. Vx is an offset added to the I,     |
  |  which is a base address.                                                     |
  |  Set Vf if the operation results in a carry, i.e. the result is greater       |
  |  than 0xFF. Otherwise reset Vf.                                               |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX1E()
{
    Vf = (state_.regs.I + Vx) > 0x0FFF;
    state_.regs.I += Vx;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX29: Load I with Vx, which contains address of font sprite.                 |
  |                                                                               |
  |  Load I with the value of Vx * 5, which is the address of a character         |
  |  from the font set. Multiple the value of Vx by 5, because each character     |
  |  is 5 bytes (where each byte specifies one row of the sprite).                |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX29()
{
    if ((Vx * 0x5) < CHIP8_ROM_ADDRESS || (Vx * 0x5) > CHIP8_MEM_SIZE - 5) {
        #ifdef DEBUG
        printf("0x%04X: Illegal opcode, 0x%04X outside memory range! Resetting rom...", _state_.regs.pc, Vx * 0x5);
        #endif // DEBUG
        //_ResetCpu();
    }

    state_.regs.I = Vx * 5;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX30: Load I with address of 10-byte font sprite to draw digit in Vx.  SCHIP |
  |                                                                               |
  |  Load I with the address of the 10-byte sprite from the SCHIP font set        |
  |  to draw the character sprite indexed by Vx.                                  |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX30()
{
    if ((Vx * 0x5) < CHIP8_ROM_ADDRESS || (Vx * 0x5) > CHIP8_MEM_SIZE - 9) {
        #ifdef DEBUG
        printf("0x%04X: Illegal opcode, 0x%04X outside memory range! Resetting rom...", _state_.regs.pc, Vx * 0x5);
        #endif // DEBUG
        //_ResetCpu();
    }

   state_.regs.I = Vx * 10;
   state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX33: Load I with Vx (BCD).                                                  |
  |                                                                               |
  |  Load I with the value of Vx in binary-coded decimal representation.          |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX33()
{
    state_.mainMemory[state_.regs.I] = Vx / 100;
    state_.mainMemory[state_.regs.I + 1] = (Vx / 10) % 10;
    state_.mainMemory[state_.regs.I + 2] = (Vx % 100) % 10;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX55: Store V0 to Vx in memory from address I.                               |
  |                                                                               |
  |  Iterate over a number of primary registers, storing each value in a          |
  |  corresponding byte of memory from base address I.                            |
  |                                                                               |
  |  There is an undocumented operation of this instruction which loads I         |
  |  with X + 1. However, this breaks Connect4 as the discs appear to be drawn    |
  |  at random locations instead of in straight lines.                            |
  |  This can be toggled on/off in case a game relies on it to run properly.      |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX55()
{
    if (state_.regs.I < CHIP8_ROM_ADDRESS || state_.regs.I > (0x0FFF - X)) {
        #ifdef DEBUG
        printf("0x%04X: Illegal opcode, 0x%04X outside memory range! Resetting rom...", pc, VX * 0x5);
        #endif // DEBUG
        resetCpu();
    } else {
        for (int i = 0; i <= X; i++) {
            state_.mainMemory[state_.regs.I + i] = state_.regs.V[i];
        }
    }

    //I += X + 1;
   state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX65: Store values from memory from address I into V0 to Vx.                 |
  |                                                                               |
  |  Iterate over a number of contiguous bytes from memory, storing each byte     |
  |  in a corresponding primary register.                                         |
  |                                                                               |
  |  There is an undocumented operation of this instruction which loads I         |
  |  with X + 1. However, this breaks Connect4 as the discs appear to be drawn    |
  |  at random locations instead of in straight lines.                            |
  |  This can be toggled on/off in case a game relies on it to run properly.      |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX65()
{
    if (state_.regs.I > (0x0FFF - X)) {
        #ifdef DEBUG
        printf("0x%04X: Illegal ld, address 0x%04X exceeds memory range! Resetting rom...", );
        #endif // DEBUG
        resetCpu();
    } else {
        for (int i = 0; i <= X; i++) {
           state_.regs.V[i] = state_.mainMemory[state_.regs.I + i];
        }
    }

    //I += X + 1;
    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX75: Store V0-Vx in R0-Rx.                                            SCHIP |
  |                                                                               |
  |  Store the values contained in registers V0 to Vx in the RPL user flags_.      |
  |  Flags R0 to R7 are only used by SCHIP games.                                 |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX75()
{
    #ifdef DEBUG
    if (X > 7) {
        printf("0x%04X: Illegal store, cannot access more than 8 RPL flags_! Resetting rom...", pc);
        resetCPU();
    }
    #endif

    for (int i = 0; i <= X; i++) {
       state_.regs.R[i] = state_.regs.V[i];
    }

    state_.regs.pc += 2;
}

/*|===============================================================================|
  |  FX85: Store R0-Rx in V0-Vx                                             SCHIP |
  |                                                                               |
  |  Store the values contained in RPL user flags_ R0-Rx in registers V0-Vx.       |
  |  Flags R0 to R7 are only used by SCHIP games.                                 |
  |-------------------------------------------------------------------------------|*/
void Chip8::Exec_FX85()
{
    #ifdef DEBUG
    if (X > 7) {
        printf("0x%04X: Illegal store, cannot access more than 8 RPL flags! Resetting rom...", pc);
        resetCPU();
    }
    #endif

    for (int i = 0; i <= X; i++) {
       state_.regs.V[i] = state_.regs.R[i];
    }

   state_.regs.pc += 2;
}

void Chip8::IllegalOpcode()
{
    #ifdef DEBUG
    printf("0x%04X: 0x%04X\tIllegal opcode!");
    #endif // DEBUG
    state_.regs.pc += 2;
}


/*|===============================================================================|
  |  EmulateCycles(int nCycles)                                                   |
  |                                                                               |
  |  Execute the number of cycles given by the argument nCycles. Because all      |
  |  Chip8 instructions take one cycle to execute, effectively this method        |
  |  executes nCycles instructions.                                               |
  |-------------------------------------------------------------------------------|*/
void Chip8::emulateCycles(unsigned int nCycles)
{
    if (!flags_ & CPU_FLAG_PAUSED) {
        return;
    }

    if (nCycles < 0) {
        #ifdef DEBUG
        puts("Error: nCycles must be 0 or greater!")
        #endif // DEBUG
        return;
    }

    for (unsigned int i = nCycles; i > 0; i--) {
       state_.regs.opcode = fetch(state_.regs.pc);
       //printf("%04X: %04X\n", state_.regs.opcode, state_.regs.pc);

        switch (state_.regs.opcode & 0xF000) {
            case 0x0000:
                switch (state_.regs.opcode & 0x00F0) {
                    case 0x00C0 : Exec_00CN(); break;
                }

                switch (state_.regs.opcode & 0x00FF) {
                    case 0x00EE : Exec_00EE(); break;
                    case 0x00E0 : Exec_00E0(); break;
                    case 0x00FB : Exec_00FB(); break;
                    case 0x00FC : Exec_00FC(); break;
                    case 0x00FD : Exec_00FD(); break;
                    case 0x00FE : Exec_00FE(); break;
                    case 0x00FF : Exec_00FF(); break;
                }
                break;

            case 0x1000 : Exec_1NNN(); break;
            case 0x2000 : Exec_2NNN(); break;
            case 0x3000 : Exec_3XNN(); break;
            case 0x4000 : Exec_4XNN(); break;
            case 0x5000 : Exec_5XNN(); break;
            case 0x6000 : Exec_6XNN(); break;
            case 0x7000 : Exec_7XNN(); break;

            case 0x8000:
                switch (state_.regs.opcode & 0x000F) {
                    case 0x0000 : Exec_8XY0(); break;
                    case 0x0001 : Exec_8XY1(); break;
                    case 0x0002 : Exec_8XY2(); break;
                    case 0x0003 : Exec_8XY3(); break;
                    case 0x0004 : Exec_8XY4(); break;
                    case 0x0005 : Exec_8XY5(); break;
                    case 0x0006 : Exec_8XY6(); break;
                    case 0x0007 : Exec_8XY7(); break;
                    case 0x000E : Exec_8XYE(); break;
                }
                break;

            case 0x9000 : Exec_9XY0(); break;
            case 0xA000 : Exec_ANNN(); break;
            case 0xB000 : Exec_BNNN(); break;
            case 0xC000 : Exec_CXNN(); break;
            case 0xD000 : Exec_DXYN(); break;

            case 0xE000:
                switch (state_.regs.opcode & 0x00FF) {
                    case 0x009E : Exec_EX9E(); break;
                    case 0x00A1 : Exec_EXA1(); break;
                }
                break;

            case 0xF000:
                switch (state_.regs.opcode & 0x00FF) {
                    case 0x0007 : Exec_FX07(); break;
                    case 0x000A : Exec_FX0A(); break;
                    case 0x0015 : Exec_FX15(); break;
                    case 0x0018 : Exec_FX18(); break;
                    case 0x001E : Exec_FX1E(); break;
                    case 0x0029 : Exec_FX29(); break;
                    case 0x0030 : Exec_FX30(); break;
                    case 0x0033 : Exec_FX33(); break;
                    case 0x0055 : Exec_FX55(); break;
                    case 0x0065 : Exec_FX65(); break;
                    case 0x0075 : Exec_FX75(); break;
                    case 0x0085 : Exec_FX85(); break;
                }
                break;

            default:
                IllegalOpcode();
                break;
        }
    }
}
