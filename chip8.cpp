#include <fstream>
#include "chip8.h"
using namespace std;

const unsigned int START_ADDRESS = 0x200;

void Chip8::LoadROM(char const* filename){
    ifstream file(filename, ios::binary | ios::ate);
    if (file.is_open()){
        streampos size = file.tellg();
        char * buffer = new char [size];
        file.seekg(0, ios::beg);
        file.read(buffer, size);
        file.close();

        for(long i = 0; i < size; i++){
            memory[START_ADDRESS + i] = buffer[i];
        }
        delete[] buffer;
    }
}

const unsigned int FONTSET_START_ADDRESS = 0x50;

Chip8::Chip8() : randGen(chrono::system_clock::now().time_since_epoch().count()),randByte(0, 255U) {
    pc = START_ADDRESS;
    for(unsigned int i = 0; i < FONTSET_SIZE; i++){
        memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
}
//clears screen and fills with 0's
void Chip8:: OP_00E0() {
    memset(video, 0, sizeof(video));
}
//returns from subroutine
void Chip8::OP_00EE(){
    --sp;
    pc = stack[sp];
}
//jumps a program execution to another spot in memory
void Chip8::OP_1nnn(){
    uint16_t address = opcode & 0x0FFFu; //extract the last 12 bits of opcode(0-11), set others to 0. IF EITHER opcode or mask have 0, keep 0. else is 1.
    pc = address;
}
//calls subroutine along with the main one
void Chip8::OP_2nnn(){
    uint16_t address = opcode & 0x0FFFu;
    stack[sp] = pc; //saves current program counter to stack.
    ++sp; //stack pointer keeps track of the top of the stack, points to next open spot in stack.
    pc = address;
}
//skips instruction if Vx and byte are equal
void Chip8::OP_3xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; //takes bits 8-11 in opcode and mask, performs bitwise operation shifting to right 8 bits.
    uint8_t byte = opcode & 0x00FFu; //takes last 8 bits in opcode and mask. all others set to 0
    if(registers[Vx] == byte){
        pc += 2;
    }
}
//skips instruction if Vx and byte are not equal
void Chip8::OP_4xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if(registers[Vx] != byte){
        pc += 2;
    }
}
//skips instruction if registers equal in value
void Chip8::OP_5xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; //takes only bits 4-7, then shifts 4 bits to the right

    if(registers[Vx] == registers[Vy]){
        pc += 2;
    }
}
//sets register value at Vx to value of byte
void Chip8::OP_6xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;
    registers[Vx] = byte;
}
//increases register value at Vx by the value at byte
void Chip8::OP_7xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;
    registers [Vx] += byte;
}
// copies the register value at Vy into Vx
void Chip8::OP_8xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    registers[Vx] = registers[Vy];
}
// keeps any bit set in Vx OR Vy
void Chip8::OP_8xy1(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    registers[Vx] |= registers[Vy];
}
// keeps only bit sets from BOTH Vx AND Vy.
void Chip8::OP_8xy2(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    registers[Vx] &= registers[Vy];
}
//sets bits at Vx if Vx and Vy are different at that specific spot
void Chip8::OP_8xy3(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    registers[Vx] ^= registers[Vy];
}
// if the sum of registers Vx and Vy exceed 255, VF becomes 1(carry) or 0.
void Chip8::OP_8xy4(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    uint16_t sum = registers[Vx] + registers[Vy];

    if(sum > 255U) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }
    registers[Vx] = sum & 0xFFu; //value at register Vx is set to the last 8 bits of sum.
}
//subtracts Vy from Vx, sets Vf to 0 if no borrow
void Chip8::OP_8xy5(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;

    if(registers[Vx] > registers[Vy]){
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }
    registers[Vx] -= registers[Vy];
}
//shifts Vx to the right by 1. VF gets old least significant bit
void Chip8::OP_8xy6(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    registers[0xF] = (registers[Vx] & 0x1u); //special register VF gets the value of the least significant bit(rightmost bit of Vx)
    registers[Vx] >>= 1; //shift all bits in register Vx to the left by 1, empty spot is 0. same as multiplying by 2
}
//subtracts Vy from Vx
void Chip8::OP_8xy7(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    if(registers[Vy] > registers[Vx]){
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }
    registers[Vx] = registers[Vy] - registers[Vx];
}
//shifts Vx to the left by 1. VF gets old most significant bit
void Chip8::OP_8xyE(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    registers[0xF] = (registers[Vx] & 0x80u) >> 7u; //saves the most significant bit in Vx(leftmost bit) into Vf. shifts that bit 7 to the right(to the rightmost,least significant)
    registers[Vx] <<= 1;//saves most significant bit in Vx so that its not lost when shifting all to the left.
}
//skips instruction if value at register Vx is not equal to the value at Vy
void Chip8::OP_9xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
    if(registers[Vx] != registers[Vy]){
        pc += 2;
    }
}
//sets register values at address into index
void Chip8::OP_Annn(){
    uint16_t address = opcode & 0x0FFFu;
    index = address;
}
//jumps to a location of the register value from address plus the register at V0
void Chip8::OP_Bnnn(){
    uint16_t address = opcode & 0x0FFFu;
    pc = registers[0] + address; //sets program counter to the sum of register V0 and register at address
}
// puts a random number into the register at Vx
void Chip8::OP_Cnnn(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;
    registers[Vx] = randByte(randGen) & byte; //generates a random # 0-255(any possible value for a byte), only keeps the immediate amount of bits in the opcode
}
//puts random number into register at Vx
void Chip8::OP_Cxkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;
    registers[Vx] = randByte(randGen) & byte;
}
void Chip8::OP_Dxyn(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x0FF0u) >> 4u;
    uint8_t height = opcode & 0x000Fu;
    uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
    uint8_t yPos = registers[Vy] % VIDEO_HEIGHT;

    registers[0xF] = 0;
    for(unsigned int row = 0; row < height; ++row){
        uint8_t spriteByte = memory[index + row];
        for(unsigned int col = 0; col < 8; ++col){
            uint8_t spritePixel = spriteByte & (0x80 >> col);
            uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];
            if(spritePixel){
                if(*screenPixel == 0xFFFFFFFF) {
                    registers[0xF] = 1;
                }
                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}
void Chip8::OP_Ex9E(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];
    if(keypad[key]){
        pc += 2;
    }
}
void Chip8::OP_ExA1(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t key = registers[Vx];
    if(!keypad[key]){
        pc += 2;
    }
}
void Chip8::OP_Fx07(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    registers[Vx] = delayTimer;
}
void Chip8::OP_Fx0A(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    if(keypad[0]){
        registers[Vx] = 0;
    } else if(keypad[1]) {
        registers[Vx] = 1;
    } else if(keypad[2]){
        registers[Vx] = 2;
    } else if(keypad[3]){
        registers[Vx] = 3;
    } else if(keypad[4]) {
        registers[Vx] = 4;
    } else if(keypad[5]){
        registers[Vx] = 5;
    } else if(keypad[6]){
        registers[Vx] = 6;
    } else if(keypad[7]){
        registers[Vx] = 7;
    } else if(keypad[8]){
        registers[Vx] = 8;
    } else if(keypad[9]){
        registers[Vx] = 9;
    } else if(keypad[10]){
        registers[Vx] = 10;
    } else if(keypad[11]){
        registers[Vx] = 11;
    } else if(keypad[12]){
        registers[Vx] = 12;
    } else if(keypad[13]){
        registers[Vx] = 13;
    } else if(keypad[14]){
        registers[Vx] = 14;
    } else if(keypad[15]){
        registers[Vx] = 15;
    } else {
        pc -= 2;
    }
}
void Chip8::OP_Fx15(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    delayTimer = registers[Vx];
}
void Chip8::OP_Fx18(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    soundTimer = registers[Vx];
}
void Chip8::OP_Fx1E(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    index += registers[Vx];
}
void Chip8::OP_Fx29(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t digit = registers[Vx];
    index = FONTSET_START_ADDRESS + (5 * digit);
}
void Chip8::OP_Fx33(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = registers[Vx];
    memory[index + 2] = value % 10;
    value /= 10;
    memory[index + 1] = value % 10;
    value /= 10;
    memory[index] = value % 10;
}
void Chip8::OP_Fx55(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    for(uint8_t i = 0; i <= Vx; ++i){
        memory[index + i] = registers[i];
    }
}
//comment
void Chip8::OP_Fx65(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    for(uint8_t i = 0; i <= Vx; ++i){
        registers[i] = memory[index + i];
    }
}