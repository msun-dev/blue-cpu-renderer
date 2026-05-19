// STD
#include <time.h>
// Raylib
#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "../include/raygui.h"
// Own libs
#include "../include/prng.h"
#include "../include/blue-cpu/src/cpu.h"
#include "../include/blue-cpu/src/types.h"

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

typedef struct prng_t {
	uint32_t state;
	uint32_t inc;
} prng_t;

typedef struct context_t {
	Vector2    screen_size;
	int        target_fps;
	BlueCpu_t* cpu;
	prng_t*    rng;
	bool       draw_pc; // Draws PC register
	bool       process_enabled; // Enables process
	bool       step_req;        // Single step
	bool       clear_req;       // Clears CPU memory and regs
	bool       corrupt_req;     // Sets to random uint values besides 0
	bool       restart_req;     // Clears cpu and loads starting program
} context_t;

void Setup(context_t* ctx);
void Process(context_t* ctx);
void Draw(context_t* ctx);
void Shutdown(context_t* ctx);

Color GetColorFromCell(uint16_t val);

int main(void) {
	context_t ctx = {{512, 512}, 60, NULL, NULL,
	                 false, false, false, false, false, true};
	
	Setup(&ctx);
	
	while (!WindowShouldClose()) {
		Process(&ctx);
		Draw(&ctx);
	}
	
	Shutdown(&ctx);
	
	return 0;
}

void Setup(context_t* ctx) {
	InitWindow(ctx->screen_size.x, ctx->screen_size.y, "Blue CPU Visualiser");
	SetTargetFPS(ctx->target_fps);
	
	ctx->rng = malloc(sizeof(pcg32_random_t));
	ctx->rng->state = time(NULL);
	ctx->rng->inc = 0;
	
	ctx->cpu = initCpu(malloc, free);
	if (!ctx->cpu) {
		return 0;
	}
	if (loadProgram(
			  ctx->cpu, 0x0000, cpu_program, sizeof(cpu_program) / sizeof(uint16_t))) {
		return 2;
	}
	enableCpu(ctx->cpu);
}

void Process(context_t* ctx) {
	if (ctx->process_enabled) {
		emulateCycle(ctx->cpu);
	}
	if (ctx->step_req) {
		ctx->process_enabled = false;
		emulateCycle(ctx->cpu);
		ctx->step_req = false;
	}
	if (ctx->clear_req) {
		ctx->process_enabled = false;
		clearRam(ctx->cpu);
		ctx->clear_req = false;
	}
	if (ctx->corrupt_req) {
		ctx->process_enabled = false;
		for (uint16_t i = 0; i < RAM_LEN; i++) {
			uint16_t cell = (uint16_t)pcg32_random_r(&ctx->rng);
			setRamCell(ctx->cpu, i, cell);
		}
		ctx->corrupt_req = false;
	}
	if (ctx->restart_req) {
		ctx->process_enabled = false;
		free(ctx->cpu);
		ctx->cpu = initCpu(malloc, free);
		if (!ctx->cpu) {
			return 0;
		}
		if (loadProgram(
					ctx->cpu, 0x0000, cpu_program,
					sizeof(cpu_program) / sizeof(uint16_t))) {
			return 2;
		}
		enableCpu(ctx->cpu);
		ctx->restart_req = false;
	}
}

void Draw(context_t* ctx) {
	BeginDrawing();
	ClearBackground(WHITE);
	
	// UI
	if (GuiButton((Rectangle) { 340, 10, 25, 25}, "S"))
		ctx->step_req = true;
	if (GuiToggle((Rectangle) { 366, 10, 25, 25}, "P", &ctx->process_enabled))
		ctx->process_enabled = !ctx->process_enabled;
	if (GuiButton((Rectangle) { 392, 10, 25, 25}, "CL"))
		ctx->clear_req = true;
	if (GuiButton((Rectangle) { 418, 10, 25, 25}, "CO"))
		ctx->corrupt_req = true;
	if (GuiButton((Rectangle) { 444, 10, 25, 25}, "R"))
		ctx->restart_req = true;
	if (GuiToggle((Rectangle) { 470, 10, 25, 25}, "P", &ctx->draw_pc))
		ctx->draw_pc = true;
	
	Vector2 size = {4, 4};
	// CPU
	// RAM
	Vector2 pos = {10, 10};
	Color color;
	uint16_t cell = 0;
	uint16_t cell_d;
	int dist = 1;
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			cell_d = getRamCell(ctx->cpu, cell);
			Color color = GetColorFromCell(cell_d);
			if (cell_d)
				DrawRectangleV(pos, size, color);
			pos.x += dist + size.x;
			cell++;
		}
		pos.x = 10;
		pos.y += dist + size.y;
	}
	
	// Registers
	// PC
	if (ctx->draw_pc) {
		uint16_t reg = getRegister(ctx->cpu, REG_PC);
		Vector2 ir_pos = {10 + reg % 64 * (size.x + 1), 10 + reg / 64 * (size.y + 1)};
		if (ctx->cpu->status_switches[SW_POWER])
			DrawRectangleLines(ir_pos.x, ir_pos.y, size.x, size.y,
			                   (Color){0xFF, 0x00, 0x00, 0xFF});
	}
	// registers[]
	Vector2 reg_pos = {10, 335};
	Color reg_clr;
	for (size_t i = 0; i < REGS_LEN; i++) {
		reg_clr = GetColorFromCell(ctx->cpu->registers[i]);
		DrawRectangleV(reg_pos, size, reg_clr);
		reg_pos.x += size.x + 1;
	}
	
	// Switches
	Vector2 sw_pos = {10, 330};
	for (size_t i = 0; i < SWITCHES_LEN; i++) {
		DrawRectangleV(sw_pos,
		               size,
		               ctx->cpu->status_switches[i] == true ?
		                 (Color){0xFF, 0x00, 0x00, 0xFF} :
		                 (Color){0x00, 0x00, 0x00, 0xFF}
		);
		sw_pos.x += 5;
	}
	
	EndDrawing();
}

void Shutdown(context_t* ctx) {
	CloseWindow();
	deinitCpu(ctx->cpu, free);
}

Color GetColorFromCell(uint16_t value) {
	Color c = {
		0,
		(uint8_t)((value >> 8) & 0xFF),
		(uint8_t)((value & 0xFF)),
		0xFF
	};
	return c;
}

uint32_t RandGenU32(prng_t* rng) {
	uint64_t oldstate = rng->state;
	rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
	uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	uint32_t rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
