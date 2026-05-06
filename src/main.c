// STD
#include <stdio.h>
#include <stdint.h>
// Raylib
#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "../include/raygui.h"
// Own libs
#include "../include/emu-blue/cpu.h"

uint16_t cpu_program[27] = {
// CODE   addr  ASM OP   Cycles  Registers state
  /* Start */
	0x8007, //0 | SRJ 004 | 1     | Running subroutine on 007
	0xA017, //1 | JMP 00x | 19    | Going to other routine
  /* Storage */
	0x0000, //2 |  data   |       | SRJ pointer, 0001 after jump and STA
	0x1111, //3 |  data   |       | const for sub-routine
	0x5555, //4 |  data   |       | const for post sub-routine
	0x0000, //5 |  data   |       | data storage for sub routine (3333)
	0x0000, //6 |  data   |       | data storage for post sub-routine
  /* Sub-Routine */
	0x7002, //7 | STA 002 | 3     | ??? [002] = 0001
	0x6003, //8 | LDA 003 | 5     | A = 1111
	0xF000, //9 | NOP 000 | 6     |
	0x1003, //A | ADD 003 | 8     | A = 1111 + 1111 = 2222
	0x7005, //B | STA 004 | 10    | [005] = 3333
	0xA00F, //C | JMP 00F | 11    | Reconstructing pointer
	0x0000, //D | HLT xxx |       |
	0x0000, //E | HLT xxx |       |
	/* Reconstructor */
	0x6002, //F | LDA 002 | 13    |
	0xA013, //10| JMP 013 | 14    | jump over consts
	0xA000, //11|  data   |       | const OP_JMP
	0x0000, //12|  data   |       | zero const
	0x4011, //13| IOR 011 | 15    | Combining loaded adress with OP_JMP
	0x7015, //14| STA 015 | 17    | Storing it in jmp-bck
	0x0000, //15| jmp-bck | 18    | jumps back to non-sub-routine. Should be A001
	0x0000, //16| HLT xxx |       | trap just in case.
	/* Not Sub-Routine */
	0x6005, //17| LDA 005 | 21    | A = 2222
	0x1004, //1A| ADD 004 | 23    | A = 2222 + 5555 = 7777
	0x0000, //1B| HLT xxx | 24    |
};

Vector2 screen_size = {512, 512};

int main(void) {
	InitWindow(screen_size.x, screen_size.y, "Blue CPU Visualiser");
	SetTargetFPS(60);

	//init cpu
	BlueCpu_t* cpu = initCpu(malloc, free);
	if (!cpu) {
		printf("Error initialisating cpu\n");
		return 0;
	}
	if (loadProgram(cpu, 0x0000, cpu_program,
	                sizeof(cpu_program) / sizeof(uint16_t))) {
		printf("error loading the programm\n");
		return 2;
	}
	enableCpu(cpu);

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(WHITE);

		//draw REGS
		//draw RAM
		Vector2 pos = {10, 10};
		Vector2 size = {4, 4};
		Color color;
		uint16_t cell;
		uint16_t cell_d;
		int dist = 1;
		for (int i = 0; i < 64; i++) {
			for (int j = 0; j < 64; j++) {
				cell = (uint16_t)((i + 1) * (j + 1));
				cell_d = getRamCell(cpu, cell);
				printf("cell = %d\n", cell_d);
				Color color = {
				  (uint8_t)cell_d & 0xF800 >> 11,
				  (uint8_t)cell_d & 0x07C0 >>  6,
				  (uint8_t)cell_d & 0x003F,
				  0xFF,
				};
				if (cell_d)
					DrawRectangleV(pos, size, color);
				pos.x += dist + size.x;
			}
			pos.x = 10;
			pos.y += dist + size.y;
		}

		//draw GUI buttons

		emulateCycle(cpu);

		EndDrawing();
	}

	CloseWindow();
	deinitCpu(cpu, free);

	return 0;
}
