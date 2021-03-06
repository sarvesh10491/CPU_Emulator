// // Datasheet : http://www.obelisk.me.uk/6502/

#include "6502_cpu.h"

int main(){
    m6502::CPU cpu;    // Create CPU
    m6502::Mem mem;    // Create Memory

    m6502::Word codeSegAddr = 0xE000;
    // m6502::Word dataSegAddr = 0x7471;

    cpu.reset(codeSegAddr, mem);

    //******************************************
    // inline data segment : start
    mem[0x7471] = 0x24;
    mem[0x74A7] = 0x49;
    // inline data segment : end

    // inline code segment : start
    mem[codeSegAddr++] = m6502::CPU::INS_LDX_IM;
    mem[codeSegAddr++] = 0x36;
    mem[codeSegAddr++] = m6502::CPU::INS_LDA_ABSX;
    mem[codeSegAddr++] = 0x71;
    mem[codeSegAddr++] = 0x74;
    // inline code segment : end
    //******************************************

    cout << "\nInitial register status\n";
    cpu.printStatus();
    cpu.exec(6, mem);
    cout << "Final register status\n";
    cpu.printStatus();
    

    return 0;
}