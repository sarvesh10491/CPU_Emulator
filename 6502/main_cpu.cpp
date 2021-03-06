// // Datasheet : http://www.obelisk.me.uk/6502/

#include <iostream>
using namespace std;

namespace m6502{
	using signByte = char;
	using Byte = unsigned char;
	using Word = unsigned short;

	using u32 = unsigned int;
	using s32 = signed int;

	struct CPU;
	struct Mem;
    struct Flags;
}

struct m6502::Flags{	
	Byte C : 1;         //0: Carry Flag	
	Byte Z : 1;         //1: Zero Flag
	Byte I : 1;         //2: Interrupt disable
	Byte D : 1;         //3: Decimal mode
	Byte B : 1;         //4: Break
	Byte Unused : 1;    //5: Unused
	Byte V : 1;         //6: Overflow
	Byte N : 1;         //7: Negative
};

struct m6502::Mem
{
	static constexpr u32 MAX_MEM = 1024 * 64;
	Byte Data[MAX_MEM];

	void initialise(){
		for(u32 i=0; i<MAX_MEM; i++){
			Data[i] = 0;
		}
	}

    // Read 1 byte
	Byte operator[](u32 Address) const{
		// assert here when Address is < MAX_MEM
		return Data[Address];
	}

	// Write 1 byte
	Byte& operator[](u32 Address){
		// assert here when Address is < MAX_MEM
		return Data[Address];
	}
};

struct m6502::CPU{
    Word PC;        // Program counter
	Byte SP;        // Stack pointer

    Byte A, X, Y;   // Registers set
    
    union{          // Processor status
		Byte PS;	
		Flags stflag;
	};


    // opcodes
	static constexpr Byte
		//LDA
		INS_LDA_IM = 0xA9,
		INS_LDA_ZP = 0xA5,
		INS_LDA_ZPX = 0xB5,
		INS_LDA_ABS = 0xAD,
		INS_LDA_ABSX = 0xBD,
		INS_LDA_ABSY = 0xB9,
		INS_LDA_INDX = 0xA1,
		INS_LDA_INDY = 0xB1;


    // Functions
    Byte fetchByte(u32& cycles, Mem& memory){
        Byte data = memory[PC];
        PC++;
        cycles--;

        return data;
    }

    Word fetchWord(u32& cycles, Mem& memory){
        Word data = memory[PC];
        PC++;

        data |= (memory[PC] << 8);
        PC++;

        cycles -= 2;

        // if(PLATFORM_BIG_ENDIAN){
        //     swapBytes(data);
        // }

        return data;
    }

    Byte readByte(u32& cycles, Word addr, Mem& memory){
        Byte data = memory[addr];
        cycles--;

        return data;
    }

    void printStatus(){
        printf("--------------------------------------\n");
        printf( "A: 0x%x  X: 0x%x  Y: 0x%x\n", A, X, Y );
	    printf( "PC: 0x%x  SP: 0x%x\n", PC, SP );
	    printf( "PS: 0x%x\n", PS );
        printf("======================================\n");
    }


    void reset(Mem& memory){
		PC = 0xFFFB;
        SP = 0x00FF;
        stflag.C = stflag.Z = stflag.I = stflag.D = stflag.B = stflag.V = stflag.N = 0;
        A = X = Y = 0;
        memory.initialise();
	}

    void exec(u32 cycles, Mem& memory){
        while(cycles > 0){
            Byte instr = fetchByte(cycles, memory);     // Read instruction opcode

            switch(instr){
                case INS_LDA_IM:{
                    Byte value = fetchByte(cycles, memory);
                    A = value;
                    stflag.Z = (A==0);
                    stflag.N = (A & 0b10000000) > 0;
                } break;

                case INS_LDA_ZP:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    A = readByte(cycles, zeroPageAddr, memory);
                    stflag.Z = (A==0);
                    stflag.N = (A & 0b10000000) > 0;
                } break;

                case INS_LDA_ZPX:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);

                    zeroPageAddr += X;
                    cycles--;

                    A = readByte(cycles, zeroPageAddr, memory);
                    stflag.Z = (A==0);
                    stflag.N = (A & 0b10000000) > 0;
                } break;

                case INS_LDA_ABS:{
                    Word absAddr = fetchWord(cycles, memory);
                    A = readByte(cycles, absAddr, memory);
                    stflag.Z = (A==0);
                    stflag.N = (A & 0b10000000) > 0;
                } break;

                default:{
                    cout << "Unhandled instruction\n";
                    cycles = 0;
                } break;
            }
        }
    }

};

int main(){
    m6502::CPU cpu;    // Create CPU
    m6502::Mem mem;    // Create Memory

    cpu.reset(mem);

    //******************************************
    // inline data segment : start
    mem[0x7471] = 0x24;
    // inline data segment : end

    // inline code segment : start
    mem[0xFFFB] = m6502::CPU::INS_LDA_ABS;
    mem[0xFFFC] = 0x71;
    mem[0xFFFD] = 0x74;
    // inline code segment : end
    //******************************************

    cpu.printStatus();
    cpu.exec(4, mem);
    cpu.printStatus();
    

    return 0;
}