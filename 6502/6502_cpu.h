// Datasheet : http://www.obelisk.me.uk/6502/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
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
		// LDA
		INS_LDA_IM = 0xA9,
		INS_LDA_ZP = 0xA5,
		INS_LDA_ZPX = 0xB5,
		INS_LDA_ABS = 0xAD,
		INS_LDA_ABSX = 0xBD,
		INS_LDA_ABSY = 0xB9,
		INS_LDA_INDX = 0xA1,
		INS_LDA_INDY = 0xB1,
        
        // LDX
		INS_LDX_IM = 0xA2,
		INS_LDX_ZP = 0xA6,
		INS_LDX_ZPY = 0xB6,
		INS_LDX_ABS = 0xAE,
		INS_LDX_ABSY = 0xBE,
		
        // LDY
		INS_LDY_IM = 0xA0,
		INS_LDY_ZP = 0xA4,
		INS_LDY_ZPX = 0xB4,
		INS_LDY_ABS = 0xAC,
		INS_LDY_ABSX = 0xBC,
        
        // STA
		INS_STA_ZP = 0x85,
		INS_STA_ZPX = 0x95,
		INS_STA_ABS = 0x8D,
		INS_STA_ABSX = 0x9D,
		INS_STA_ABSY = 0x99,
		INS_STA_INDX = 0x81,
		INS_STA_INDY = 0x91,
		
        // STX
		INS_STX_ZP = 0x86,
		INS_STX_ZPY = 0x96,
		INS_STX_ABS = 0x8E,
		
        // STY
		INS_STY_ZP = 0x84,
		INS_STY_ZPX = 0x94,
		INS_STY_ABS = 0x8C,

        // Transfer Registers
		INS_TAX = 0xAA,
		INS_TAY = 0xA8,
		INS_TXA = 0x8A,
		INS_TYA = 0x98;


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

    Byte readWord(u32& cycles, Word addr, Mem& memory){
        Byte loByte = readByte(cycles, addr, memory);
		Byte hiByte = readByte(cycles, addr+1, memory);

		return loByte | (hiByte << 8);
    }

	void writeByte(Byte value, u32& cycles, Word addr, Mem& memory){
		memory[addr] = value;
		cycles--;
	}

	void writeWord(Word value, u32& cycles, Word addr, Mem& memory){
		memory[addr] = value & 0xFF;
		memory[addr + 1] = (value >> 8);
		cycles -= 2;
	}

    void setZeroAndNegativeFlags(Byte reg){
        stflag.Z = (reg==0);
        stflag.N = (reg & 0b10000000) > 0;
    }

    void printStatus(){
        printf("--------------------------------------\n");
        printf( "A: 0x%02x  X: 0x%02x  Y: 0x%02x\n", A, X, Y );
	    printf( "PC: 0x%04x  SP: 0x%02x\n", PC, SP );
	    printf( "PS: 0x%02x\n", PS );

        // printf("0x%04x      0x%02x   | 0x%02x     0x%02x     0x%02x     | 0x%02x  \n", PC, SP, A, X, Y, PS);

        printf("======================================\n");
    }


    void reset(Word resetVector, Mem& memory){
		PC = resetVector;
        SP = 0x00FF;
        stflag.C = stflag.Z = stflag.I = stflag.D = stflag.B = stflag.V = stflag.N = 0;
        A = X = Y = 0;
        memory.initialise();
	}

    void loadROM(Word& codeSegAddr, Mem& memory){
        string line;
        ifstream myfile("ROM.asm");

        if(myfile.is_open()){
            while(getline(myfile, line)){

                std::stringstream hexStringStream; 
                hexStringStream >> std::hex;

                hexStringStream.clear();
                hexStringStream.str(line);

                int tmpValue = 0;
                hexStringStream >> tmpValue;

                memory[codeSegAddr++] = static_cast<unsigned char>(tmpValue);
            }
            myfile.close();
        }
    }

    void exec(u32 cycles, Mem& memory){
        while(cycles > 0){
            Byte instr = fetchByte(cycles, memory);     // Read instruction opcode

            switch(instr){
                case INS_LDA_IM:{
                    A = fetchByte(cycles, memory);
                    setZeroAndNegativeFlags(A);
                } break;

                case INS_LDA_ZP:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    A = readByte(cycles, zeroPageAddr, memory);
                    setZeroAndNegativeFlags(A);
                } break;

                case INS_LDA_ZPX:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);

                    zeroPageAddr += X;
                    cycles--;

                    A = readByte(cycles, zeroPageAddr, memory);
                    setZeroAndNegativeFlags(A);
                } break;

                case INS_LDA_ABS:{
                    Word absAddr = fetchWord(cycles, memory);
                    A = readByte(cycles, absAddr, memory);
                    setZeroAndNegativeFlags(A);
                } break;

                case INS_LDA_ABSX:{
                    Word absAddr = fetchWord(cycles, memory);

                    Word absAddrX = absAddr + X;

                    const bool pageBoundaryCrossed = (absAddr ^ absAddrX) >> 8;
                    if(pageBoundaryCrossed)
                        cycles--;

                    A = readByte(cycles, absAddrX, memory);
                    setZeroAndNegativeFlags(A);
                } break;

                case INS_LDA_ABSY:{
                    Word absAddr = fetchWord(cycles, memory);

                    Word absAddrY = absAddr + Y;

                    const bool pageBoundaryCrossed = (absAddr ^ absAddrY) >> 8;
                    if(pageBoundaryCrossed)
                        cycles--;

                    A = readByte(cycles, absAddrY, memory);
                    setZeroAndNegativeFlags(A);
                } break;

                case INS_LDA_INDX:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    zeroPageAddr += X;
                    cycles--;
                    Word effectiveAddr = readWord(cycles, zeroPageAddr, memory);

                    A = readByte(cycles, effectiveAddr, memory);
                    setZeroAndNegativeFlags(A);
                } break;

                case INS_LDA_INDY:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    zeroPageAddr += Y;
                    cycles--;
                    Word effectiveAddr = readWord(cycles, zeroPageAddr, memory);

                    A = readByte(cycles, effectiveAddr, memory);
                    setZeroAndNegativeFlags(A);
                } break;


                case INS_LDX_IM:{
                    X = fetchByte(cycles, memory);
                    setZeroAndNegativeFlags(X);
                } break;

                case INS_LDX_ZP:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    X = readByte(cycles, zeroPageAddr, memory);
                    setZeroAndNegativeFlags(X);
                } break;

                case INS_LDX_ZPY:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);

                    zeroPageAddr += Y;
                    cycles--;

                    X = readByte(cycles, zeroPageAddr, memory);
                    setZeroAndNegativeFlags(X);
                } break;

                case INS_LDX_ABS:{
                    Word absAddr = fetchWord(cycles, memory);
                    X = readByte(cycles, absAddr, memory);
                    setZeroAndNegativeFlags(X);
                } break;

                case INS_LDX_ABSY:{
                    Word absAddr = fetchWord(cycles, memory);

                    Word absAddrY = absAddr + Y;

                    const bool pageBoundaryCrossed = (absAddr ^ absAddrY) >> 8;
                    if(pageBoundaryCrossed)
                        cycles--;

                    X = readByte(cycles, absAddrY, memory);
                    setZeroAndNegativeFlags(X);
                } break;


                case INS_LDY_IM:{
                    Y = fetchByte(cycles, memory);
                    setZeroAndNegativeFlags(Y);
                } break;

                case INS_LDY_ZP:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    Y = readByte(cycles, zeroPageAddr, memory);
                    setZeroAndNegativeFlags(Y);
                } break;

                case INS_LDY_ZPX:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);

                    zeroPageAddr += X;
                    cycles--;

                    Y = readByte(cycles, zeroPageAddr, memory);
                    setZeroAndNegativeFlags(Y);
                } break;

                case INS_LDY_ABS:{
                    Word absAddr = fetchWord(cycles, memory);
                    Y = readByte(cycles, absAddr, memory);
                    setZeroAndNegativeFlags(Y);
                } break;

                case INS_LDY_ABSX:{
                    Word absAddr = fetchWord(cycles, memory);

                    Word absAddrX = absAddr + X;

                    const bool pageBoundaryCrossed = (absAddr ^ absAddrX) >> 8;
                    if(pageBoundaryCrossed)
                        cycles--;

                    Y = readByte(cycles, absAddrX, memory);
                    setZeroAndNegativeFlags(Y);
                } break;


                case INS_STA_ZP:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    writeByte(A, cycles, zeroPageAddr, memory);
                } break;

                case INS_STA_ZPX:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);

                    zeroPageAddr += X;
                    cycles--;

                    writeByte(A, cycles, zeroPageAddr, memory);
                } break;

                case INS_STA_ABS:{
                    Word absAddr = fetchWord(cycles, memory);
                    writeByte(A, cycles, absAddr, memory);
                } break;

                case INS_STA_ABSX:{
                    Word absAddr = fetchWord(cycles, memory);

                    Word absAddrX = absAddr + X;

                    const bool pageBoundaryCrossed = (absAddr ^ absAddrX) >> 8;
                    if(pageBoundaryCrossed)
                        cycles--;

                    writeByte(A, cycles, absAddrX, memory);
                } break;

                case INS_STA_ABSY:{
                    Word absAddr = fetchWord(cycles, memory);

                    Word absAddrY = absAddr + Y;

                    const bool pageBoundaryCrossed = (absAddr ^ absAddrY) >> 8;
                    if(pageBoundaryCrossed)
                        cycles--;

                    writeByte(A, cycles, absAddrY, memory);
                } break;

                case INS_STA_INDX:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    zeroPageAddr += X;
                    cycles--;
                    Word effectiveAddr = readWord(cycles, zeroPageAddr, memory);

                    writeByte(A, cycles, effectiveAddr, memory);
                } break;

                case INS_STA_INDY:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    zeroPageAddr += Y;
                    cycles--;
                    Word effectiveAddr = readWord(cycles, zeroPageAddr, memory);

                    writeByte(A, cycles, effectiveAddr, memory);
                } break;


                case INS_STX_ZP:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    writeByte(X, cycles, zeroPageAddr, memory);
                } break;

                case INS_STX_ZPY:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);

                    zeroPageAddr += Y;
                    cycles--;

                    writeByte(X, cycles, zeroPageAddr, memory);
                } break;

                case INS_STX_ABS:{
                    Word absAddr = fetchWord(cycles, memory);
                    writeByte(X, cycles, absAddr, memory);
                } break;


                case INS_STY_ZP:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);
                    writeByte(Y, cycles, zeroPageAddr, memory);
                } break;

                case INS_STY_ZPX:{
                    Byte zeroPageAddr = fetchByte(cycles, memory);

                    zeroPageAddr += X;
                    cycles--;

                    writeByte(Y, cycles, zeroPageAddr, memory);
                } break;

                case INS_STY_ABS:{
                    Word absAddr = fetchWord(cycles, memory);
                    writeByte(Y, cycles, absAddr, memory);
                } break;


                case INS_TAX:{
                    X = A;
                    cycles--;
                    setZeroAndNegativeFlags(X);
                } break;

                case INS_TAY:{
                    Y = A;
                    cycles--;
                    setZeroAndNegativeFlags(Y);
                } break;

                case INS_TXA:{
                    A = X;
                    cycles--;
                    setZeroAndNegativeFlags(A);
                } break;

                case INS_TYA:{
                    A = Y;
                    cycles--;
                    setZeroAndNegativeFlags(A);
                } break;


                default:{
                    cout << "Unhandled instruction\n";
                    cycles = 0;
                } break;
            }
        }
    }

};