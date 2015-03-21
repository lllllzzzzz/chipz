#ifndef CHIP8_H_INCLUDED
#define CHIP8_H_INCLUDED

#define CHIP8_MEM_SIZE                  4096
#define CHIP8_VIDMEM_WIDTH              64
#define CHIP8_VIDMEM_HEIGHT             32
#define CHIP8_VIDMEM_SIZE               2048
#define SCHIP_VIDMEM_WIDTH              128
#define SCHIP_VIDMEM_HEIGHT             64
#define SCHIP_VIDMEM_SIZE               8192
#define CHIP8_STACK_SIZE                16
#define CHIP8_NUM_REGISTERS             16
#define SCHIP_NUM_RPLFLAGS              8
#define CHIP8_FONTSET_SIZE              80
#define CHIP8_ROM_ADDRESS               0x200
#define CHIP8_MAX_ROM_SIZE              3584

#define CPU_FLAG_DRAW                   0x1
#define CPU_FLAG_PAUSED                 0x2
#define CPU_FLAG_DETECTCOLLISION        0x8
#define CPU_FLAG_HWRAP                  0x10
#define CPU_FLAG_VWRAP                  0x20
#define CPU_FLAG_DETECTGAMEOVER         0x40
#define CPU_FLAG_KEYDOWN                0x80
#define CPU_FLAG_SCHIP                  0x100

#define fetch(addr)                     ((state_.mainMemory[addr] << 8) | state_.mainMemory[addr + 1])
#define X                               ((state_.regs.opcode & 0x0F00) >> 8)
#define Y                               ((state_.regs.opcode & 0x00F0) >> 4)
#define N                               (state_.regs.opcode & 0x000F)
#define NN                              (state_.regs.opcode & 0x00FF)
#define NNN                             (state_.regs.opcode & 0x0FFF)
#define Vx                              (state_.regs.V[(state_.regs.opcode & 0x0F00) >> 8])
#define Vy                              (state_.regs.V[(state_.regs.opcode & 0x00F0) >> 4])
#define Vf                              (state_.regs.V[0xF])

typedef struct registers
{
    unsigned short  pc;                             /* Program counter stores the address of next opcode */
    unsigned char   sp;                             /* Stack pointer points to top of stack */
    unsigned short  opcode;                         /* Next opcode to execute; all opcodes are 2 bytes */
    unsigned short  I;                              /* Index register stores address from memory */

    unsigned char   delayTimer;                     /* Delay timer decrements at 60Hz */
    unsigned char   soundTimer;                     /* Sound timer decrements at 60Hz; beep while non-zero */

    unsigned char   V[CHIP8_NUM_REGISTERS];         /* 16 8-bit registers V0 - VF; VF is carry flag */
    unsigned char   R[SCHIP_NUM_RPLFLAGS];          /* 8 8-bit RPL flags R0 - R7; only used in SCHIP mode */
} registers;

typedef struct Chip8State
{
    registers       regs;
    unsigned char   mainMemory[CHIP8_MEM_SIZE];
    unsigned char   videoMemory[CHIP8_VIDMEM_WIDTH * CHIP8_VIDMEM_HEIGHT];
    unsigned short  stack[CHIP8_STACK_SIZE];
} Chip8State;

class Chip8
{
    public:
        Chip8();
        ~Chip8();

        unsigned char*  getMainMemory();
        unsigned char*  getVideoMemory();
        registers       getRegisters();
        unsigned short* getStack();
        unsigned char   getDelayTimer();
        unsigned char   getSoundTimer();
        unsigned short  getProgramCounter();
        unsigned short  getOpcode(unsigned short address);
        unsigned short  getRomSize();

        unsigned int    getFlags();
        void            setFlags(unsigned int flags);
        bool            getFlag(unsigned int flag);
        void            setFlag(unsigned int flag);
        void            resetFlag(unsigned int flag);
        void            toggleFlag(unsigned int flag);

        unsigned char*  getKeyInput();
        void            setKeyDown(unsigned char key);
        void            setKeyUp(unsigned char key);
        void            setKeys(char keyState[16]);

        bool            initializeCpu();
        bool            resetCpu();
        void            debug();
        bool            loadRom(const char* szFilename);
        bool            unloadRom();
        void            emulateCycles(unsigned int nCycles);
        void            saveCpuState();
        void            loadCpuState();
        void            tickDelayTimer();
        void            tickSoundTimer();

    private:
        unsigned int    flags_;
        Chip8State      state_, saveState_;

        unsigned char   keyState_[16];
        size_t          romSize_;

        void Exec_00CN();
        void Exec_00EE();
        void Exec_00E0();
        void Exec_00FB();
        void Exec_00FC();
        void Exec_00FD();
        void Exec_00FE();
        void Exec_00FF();
        void Exec_1NNN();
        void Exec_2NNN();
        void Exec_3XNN();
        void Exec_4XNN();
        void Exec_5XNN();
        void Exec_6XNN();
        void Exec_7XNN();
        void Exec_8XY0();
        void Exec_8XY1();
        void Exec_8XY2();
        void Exec_8XY3();
        void Exec_8XY4();
        void Exec_8XY5();
        void Exec_8XY6();
        void Exec_8XY7();
        void Exec_8XYE();
        void Exec_9XY0();
        void Exec_ANNN();
        void Exec_BNNN();
        void Exec_CXNN();
        void Exec_DXYN();
        void Exec_EX9E();
        void Exec_EXA1();
        void Exec_FX07();
        void Exec_FX0A();
        void Exec_FX15();
        void Exec_FX18();
        void Exec_FX1E();
        void Exec_FX29();
        void Exec_FX30();
        void Exec_FX33();
        void Exec_FX55();
        void Exec_FX65();
        void Exec_FX75();
        void Exec_FX85();
        void IllegalOpcode();
};

#endif // CHIP8_H_INCLUDED
