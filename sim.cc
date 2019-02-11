#include <iostream>
#include <fstream>
#include <streambuf>
#include <cmath>
#include <cassert>

union fandi {
  uint32_t i;
  float f;
};

int main(int argc, char * argv[]) {
  std::string filename = (argc > 1) ? std::string(argv[1]) : "bin";
  char *buffer;
  std::ifstream t;
  int length;
  t.open(filename);      // open input file
  t.seekg(0, std::ios::end);    // go to the end
  length = t.tellg();           // report location (this is the length)
  t.seekg(0, std::ios::beg);    // go back to the beginning
  buffer = new char[0xffffffff];    // allocate memory for a buffer of appropriate dimension
  t.read(buffer, length);       // read the whole file into the buffer
  t.close();                    // close file handle
  uint32_t stack_limit = 0xffffffff;
  std::size_t addr = 0;
  uint32_t ireg[32] = {0};
  uint32_t freg[32] = {0};
  uint64_t counter = 0;
  uint64_t brcount = 0;
  uint64_t memcount = 0;
  // std::cout << "simulate: " << filename << std::endl;

  // uint64_t dstart = 1487800;
  // uint64_t dend = 1487859;

  uint64_t dstart = 0xffffffff;
  uint64_t dend   = 0xffffffff;
  // uint64_t dstart = 0x00;
  // uint64_t dend   = 0xffffffff;
  // uint64_t dstart = 851461;// 896221;
  // uint64_t dend   = 896982; // 0xffffffff;

  // uint64_t distart = 0x000014a0;
  // uint64_t diend = 0x00001c10;
  // uint64_t distart = 0x00002be0;
  // uint64_t diend  = 0x00003fd0;
  uint64_t distart = 0xffffffff;
  uint64_t diend  = 0xffffffff;

  while (addr != 0xffffffff) {
    counter++;
    uint32_t inst = (((buffer[addr] & 0x00ff) << 24) |
                     ((buffer[addr + 1] & 0x00ff) << 16) |
                     ((buffer[addr + 2] & 0x00ff) << 8) |
                     (buffer[addr + 3] & 0x00ff));
    uint8_t opcode = inst >> 26;
    uint8_t reg1 = (inst & 0x03e00000) >> 21;
    uint8_t reg2 = (inst & 0x001f0000) >> 16;
    uint8_t reg3 = (inst & 0x0000f800) >> 11;
    uint32_t imm = (inst & 0x0000ffff);
    uint32_t imm20 = (inst & 0x000fffff);
    int32_t br_addr = (imm < 0x00008000) ? (addr + imm) : ((int32_t)addr + (0xffff0000 | imm));

    switch(opcode) {
    case 0x0: // NOP
      break;
    case 0x1: // LOAD
      {
        ++memcount;
        // if (reg1 == 28) {
        uint32_t mem_a = ireg[reg2] + imm;
        uint32_t mem = (((buffer[mem_a] & 0x00ff) << 24) |
                        ((buffer[mem_a + 1] & 0x00ff) << 16) |
                        ((buffer[mem_a + 2] & 0x00ff) << 8) |
                        (buffer[mem_a + 3] & 0x00ff));
        ireg[reg1] = mem;
      }
      break;
    case 0x38: // LOADT
      {
        ++memcount;
        // if (reg1 == 28) {
        uint32_t mem_a = ireg[reg2] + imm;
        uint32_t mem = (buffer[mem_a] & 0x00ff);
        ireg[reg1] = mem;
      }
      break;
    case 0x39: // LOADTB
      {
        ++memcount;
        // if (reg1 == 28) {
        uint32_t mem_a = ireg[reg2] - imm;
        uint32_t mem = (buffer[mem_a] & 0x00ff);
        ireg[reg1] = mem;
      }
      break;
    case 0x1c: // LOADF
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] + imm;
        uint32_t mem = (((buffer[mem_a] & 0x00ff) << 24) |
                        ((buffer[mem_a + 1] & 0x00ff) << 16) |
                        ((buffer[mem_a + 2] & 0x00ff) << 8) |
                        (buffer[mem_a + 3] & 0x00ff));
        freg[reg1] = mem;
      }
      break;
    case 0x22: // LOADB
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] - imm;
        uint32_t mem = (((buffer[mem_a] & 0x00ff) << 24) |
                        ((buffer[mem_a + 1] & 0x00ff) << 16) |
                        ((buffer[mem_a + 2] & 0x00ff) << 8) |
                        (buffer[mem_a + 3] & 0x00ff));
        ireg[reg1] = mem;
      }
      break;
    case 0x26: // LOADFB
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] - imm;
        uint32_t mem = (((buffer[mem_a] & 0x00ff) << 24) |
                        ((buffer[mem_a + 1] & 0x00ff) << 16) |
                        ((buffer[mem_a + 2] & 0x00ff) << 8) |
                        (buffer[mem_a + 3] & 0x00ff));
        freg[reg1] = mem;
      }
      break;
    case 0x2: // LOADR
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] + ireg[reg3];
        uint32_t mem = (((buffer[mem_a] & 0x00ff) << 24) |
                        ((buffer[mem_a + 1] & 0x00ff) << 16) |
                        ((buffer[mem_a + 2] & 0x00ff) << 8) |
                        (buffer[mem_a + 3] & 0x00ff));
        ireg[reg1] = mem;
      }
      break;
    case 0x1d: // LOADFR
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] + ireg[reg3];
        uint32_t mem = (((buffer[mem_a] & 0x00ff) << 24) |
                        ((buffer[mem_a + 1] & 0x00ff) << 16) |
                        ((buffer[mem_a + 2] & 0x00ff) << 8) |
                        (buffer[mem_a + 3] & 0x00ff));
        freg[reg1] = mem;
      }
      break;
    case 0x3: // STORE
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] + imm;
        buffer[mem_a]     = (ireg[reg1] & 0xff000000) >> 24;
        buffer[mem_a + 1] = (ireg[reg1] & 0x00ff0000) >> 16;
        buffer[mem_a + 2] = (ireg[reg1] & 0x0000ff00) >> 8;
        buffer[mem_a + 3] = (ireg[reg1] & 0x000000ff);
      }
      break;
    case 0x36: // STORET (byte store)
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] + imm;
        buffer[mem_a] = (ireg[reg1] & 0x000000ff);
      }
      break;
    case 0x37: // STORETB (byte store)
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] - imm;
        buffer[mem_a] = (ireg[reg1] & 0x000000ff);
      }
      break;
    case 0x1e: // STOREF
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] + imm;
        assert(mem_a < stack_limit && "STOREF");
        buffer[mem_a]     = (freg[reg1] & 0xff000000) >> 24;
        buffer[mem_a + 1] = (freg[reg1] & 0x00ff0000) >> 16;
        buffer[mem_a + 2] = (freg[reg1] & 0x0000ff00) >> 8;
        buffer[mem_a + 3] = (freg[reg1] & 0x000000ff);
      }
      break;
    case 0x23: // STOREB
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] - imm;
        buffer[mem_a]     = (ireg[reg1] & 0xff000000) >> 24;
        buffer[mem_a + 1] = (ireg[reg1] & 0x00ff0000) >> 16;
        buffer[mem_a + 2] = (ireg[reg1] & 0x0000ff00) >> 8;
        buffer[mem_a + 3] = (ireg[reg1] & 0x000000ff);
      }
      break;
    case 0x27: // STOREFB
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] - imm;
        buffer[mem_a]     = (freg[reg1] & 0xff000000) >> 24;
        buffer[mem_a + 1] = (freg[reg1] & 0x00ff0000) >> 16;
        buffer[mem_a + 2] = (freg[reg1] & 0x0000ff00) >> 8;
        buffer[mem_a + 3] = (freg[reg1] & 0x000000ff);
      }
      break;
    case 0x4: // STORER
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] + ireg[reg3];
        buffer[mem_a]     = (ireg[reg1] & 0xff000000) >> 24;
        buffer[mem_a + 1] = (ireg[reg1] & 0x00ff0000) >> 16;
        buffer[mem_a + 2] = (ireg[reg1] & 0x0000ff00) >> 8;
        buffer[mem_a + 3] = (ireg[reg1] & 0x000000ff);
      }
      break;
    case 0x1f: // STOREFR
      {
        ++memcount;
        uint32_t mem_a = ireg[reg2] + ireg[reg3];
        buffer[mem_a]     = (freg[reg1] & 0xff000000) >> 24;
        buffer[mem_a + 1] = (freg[reg1] & 0x00ff0000) >> 16;
        buffer[mem_a + 2] = (freg[reg1] & 0x0000ff00) >> 8;
        buffer[mem_a + 3] = (freg[reg1] & 0x000000ff);
      }
      break;
    case 0x5: // SAVE
      ireg[reg1] = addr + 8;
      break;
    case 0x6: // BLT
      {
        ++brcount;
        int32_t a = ireg[reg1];
        int32_t b = ireg[reg2];
        if (a < b) {
          addr = br_addr;
          continue;
        }
      }
      break;
    case 0x31: // BULT
      {
        ++brcount;
        uint32_t a = ireg[reg1];
        uint32_t b = ireg[reg2];
        if (a < b) {
          addr = br_addr;
          continue;
        }
      }
      break;
    case 0x2b: // BLTF
      {
        ++brcount;
        fandi a, b;
        a.i = freg[reg1];
        b.i = freg[reg2];
        if (a.f < b.f) {
          addr = br_addr;
          continue;
        }
      }
      break;
    case 0x2c: // BEQF
      {
        ++brcount;
        fandi a, b;
        a.i = freg[reg1];
        b.i = freg[reg2];
        if (a.f == b.f) {
          addr = br_addr;
          continue;
        }
      }
      break;
    case 0x7: // BEQ
      ++brcount;
      if (ireg[reg1] == ireg[reg2]) {
        addr = br_addr;
        continue;
      }
      break;
    case 0x8: // JMP
      addr = imm20;
      continue;
    case 0x9: // JMPR
      addr = ireg[reg1];
      continue;
    case 0xa: // SETI1
      // ireg[reg1] = (ireg[reg1] & 0xffff0000) | imm;
      ireg[reg1] = imm; // 非対称
      break;
    case 0xb: // SETI2
      ireg[reg1] = (ireg[reg1] & 0x0000ffff) | (imm << 16);
      break;
    case 0x1a: // SETF1
      freg[reg1] = (freg[reg1] & 0xffff0000) | imm;
      break;
    case 0x1b: // SETF2
      // freg[reg1] = (freg[reg1] & 0x0000ffff) | (imm << 16);
      freg[reg1] = (imm << 16);
      break;
    case 0xc: // ADD
      ireg[reg1] = ireg[reg2] + ireg[reg3];
      break;
    case 0x13: // OR
      ireg[reg1] = ireg[reg2] | ireg[reg3];
      break;
    case 0x14: // AND
      ireg[reg1] = ireg[reg2] & ireg[reg3];
      break;
    case 0x15: // XOR
      ireg[reg1] = ireg[reg2] ^ ireg[reg3];
      break;
    case 0xd: // SUB
      ireg[reg1] = ireg[reg2] - ireg[reg3];
      break;
    case 0xe: // ADDI
      ireg[reg1] = ireg[reg2] + imm;
      break;
    case 0xf: // SUBI
      ireg[reg1] = ireg[reg2] - imm;
      break;
    case 0x10: // SL
      ireg[reg1] = ireg[reg2] << ireg[reg3];
      break;
    case 0x11: // LSR (unsigned)
      {
        uint32_t tgt = ireg[reg2];
        ireg[reg1] = tgt >> ireg[reg3];
      }
      break;
    case 0x12: // ASR (signed)
      {
        int32_t tgt = ireg[reg2];
        ireg[reg1] = tgt >> ireg[reg3];
      }
      break;
    case 0x20: // OUTPUT
      putchar(ireg[reg1]);
      break;
    case 0x24: // INPUT
      {
        int c = getchar();
        ireg[reg1] = 0x00ff & c;
      }
      break;
    case 0x16: // FADD
      {
        union fandi freg1, freg2, freg3;
        freg2.i = freg[reg2];
        freg3.i = freg[reg3];
        freg1.f = freg2.f + freg3.f;
        freg[reg1] = freg1.i;
      }
      break;
    case 0x17: // FSUB
      {
        union fandi freg1, freg2, freg3;
        freg2.i = freg[reg2];
        freg3.i = freg[reg3];
        freg1.f = freg2.f - freg3.f;
        freg[reg1] = freg1.i;
      }
      break;
    case 0x18: // FDIV
      {
        union fandi freg1, freg2, freg3;
        freg2.i = freg[reg2];
        freg3.i = freg[reg3];
        freg1.f = freg2.f / freg3.f;
        freg[reg1] = freg1.i;
      }
      break;
    case 0x19: // FMUL
      {
        union fandi freg1, freg2, freg3;
        freg2.i = freg[reg2];
        freg3.i = freg[reg3];
        freg1.f = freg2.f * freg3.f;
        freg[reg1] = freg1.i;
      }
      break;
    case 0x28: // ITOF
      {
        union fandi freg2;
        int32_t tgt = ireg[reg2];
        freg2.f = (float)tgt;
        freg[reg1] = freg2.i;
      }
      break;
    case 0x29: // FTOI
      {
        union fandi freg2;
        freg2.i = freg[reg2];
        ireg[reg1] = floor(freg2.f);
      }
      break;
    case 0x2a: // FNEG
      {
        union fandi freg1, freg2;
        freg2.i = freg[reg2];
        freg1.f = -freg2.f;
        freg[reg1] = freg1.i;
      }
      break;
    case 0x2d: // FSQRT
      {
        union fandi freg1, freg2;
        freg2.i = freg[reg2];
        freg1.f = sqrtf(freg2.f);
        freg[reg1] = freg1.i;
      }
      break;
    case 0x2e: // FSIN
      {
        union fandi freg1, freg2;
        freg2.i = freg[reg2];
        freg1.f = sinf(freg2.f);
        freg[reg1] = freg1.i;
      }
      break;
    case 0x2f: // FCOS
      {
        union fandi freg1, freg2;
        freg2.i = freg[reg2];
        freg1.f = cosf(freg2.f);
        freg[reg1] = freg1.i;
      }
      break;
    case 0x30: // FATAN
      {
        union fandi freg1, freg2;
        freg2.i = freg[reg2];
        freg1.f = atanf(freg2.f);
        freg[reg1] = freg1.i;
      }
      break;
    case 0x32: // READF
      {
        union fandi freg1;
        scanf("%f", &freg1.f);
        freg[reg1] = freg1.i;
      }
      break;
    case 0x3a: // READI
      {
        int r;
        scanf("%d", &r);
        ireg[reg1] = r;
      }
      break;
    case 0x33: // WRITEF
      {
        union fandi freg1;
        freg1.i = freg[reg1];
      }
      break;
    case 0x34: // FLOOR
      {
        union fandi freg1, freg2;
        freg2.i = freg[reg2];
        freg1.f = floor(freg2.f);
        freg[reg1] = freg1.i;
      }
      break;
    case 0x35: // MUL
      ireg[reg1] = ireg[reg2] * ireg[reg3];
      break;
    case 0x3b: // MOVEITOF
      freg[reg1] = ireg[reg2];
      break;
    case 0x3c: // MOVEFTOI
      ireg[reg1] = freg[reg2];
      break;
    default:
      assert(false);
      break;
    }
    if ((counter > dstart && counter < dend) ||
        (addr > distart && addr < diend)) {
      printf("%llu: addr: %0lx, %08x: OP[%02x] REG[%d]    r28(%x), r29(%x)\n", counter, addr, inst, opcode, reg1, ireg[28], ireg[29]);
      // if (counter > dstart && counter < dend) {
      //   unsigned mem_a = 0x7ffa8c;
      //   printf("    %x\n", (unsigned)buffer[mem_a]);
      //   printf("    %x\n", (unsigned)buffer[mem_a + 1]);
      //   printf("    %x\n", (unsigned)buffer[mem_a + 2]);
      //   printf("    %x\n", (unsigned)buffer[mem_a + 3]);
      // }
    }
    // printf("   r29[%x]\n", ireg[29]);
    // printf("   r30[%x]\n", ireg[30]);
    addr += 4;
  }
  // printf("\n\n\n\n\nsimulate done.\n");
  fprintf(stderr, "counter: %llu\n", counter);
  fprintf(stderr, "branch: %llu\n", brcount);
  fprintf(stderr, "memaccess: %llu\n", memcount);
  delete [] buffer;
}

// 1374045: addr: 15a1c, 08211801: OP[02] REG[1]    r28(159a4): c
