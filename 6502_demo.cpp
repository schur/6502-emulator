/*
	olc6502 - An emulation of the 6502/2A03 processor

	License (OLC-3)
	~~~~~~~~~~~~~~~

	Copyright 2018-2019 OneLoneCoder.com

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions or derivations of source code must retain the above
	copyright notice, this list of conditions and the following disclaimer.

	2. Redistributions or derivative works in binary form must reproduce
	the above copyright notice. This list of conditions and the following
	disclaimer must be reproduced in the documentation and/or other
	materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Datasheet: http://archive.6502.org/datasheets/rockwell_r650x_r651x.pdf

	Files: olc6502.h, olc6502.cpp

	Author
	~~~~~~
	David Barr, aka javidx9, �OneLoneCoder 2019
	Reinhard Schu, 2024
*/

#include <iostream>
#include <sstream>
#include <cstdint>
#include <iterator>

#include "Bus.h"
#include "olc6502.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"



class Demo_olc6502 : public olc::PixelGameEngine
{
public:
	Demo_olc6502() { sAppName = "olc6502 Demonstration"; }

	float KeyDownTime;	// for continuous stepping on holding space key

	struct LoopData
	{
		uint16_t LoopEnd;	// for loop stepping
		bool on;			// on/off toogle
	};
	LoopData Loop;

	Bus nes;
	std::map<uint16_t, std::string> mapAsm;
	std::vector<uint8_t> prog_buf;		// buffer to hold the program (before being loaded into nes.ram)

	std::string hex(uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	void DrawRam(int x, int y, uint16_t nAddr, int nRows, int nColumns)
	{
		int nRamX = x, nRamY = y;
		for (int row = 0; row < nRows; row++)
		{
			std::string sOffset = "$" + hex(nAddr, 4) + ":";
			for (int col = 0; col < nColumns; col++)
			{
				sOffset += " " + hex(nes.read(nAddr, true), 2);
				nAddr += 1;
			}
			DrawString(nRamX, nRamY, sOffset);
			nRamY += 10;
		}
	}

	void DrawCpu(int x, int y)
	{
		std::string status = "STATUS: ";
		DrawString(x , y , "STATUS:", olc::WHITE);
		DrawString(x  + 64, y, "N", nes.cpu.status & olc6502::N ? olc::GREEN : olc::RED);
		DrawString(x  + 80, y , "V", nes.cpu.status & olc6502::V ? olc::GREEN : olc::RED);
		DrawString(x  + 96, y , "-", nes.cpu.status & olc6502::U ? olc::GREEN : olc::RED);
		DrawString(x  + 112, y , "B", nes.cpu.status & olc6502::B ? olc::GREEN : olc::RED);
		DrawString(x  + 128, y , "D", nes.cpu.status & olc6502::D ? olc::GREEN : olc::RED);
		DrawString(x  + 144, y , "I", nes.cpu.status & olc6502::I ? olc::GREEN : olc::RED);
		DrawString(x  + 160, y , "Z", nes.cpu.status & olc6502::Z ? olc::GREEN : olc::RED);
		DrawString(x  + 178, y , "C", nes.cpu.status & olc6502::C ? olc::GREEN : olc::RED);
		DrawString(x , y + 10, "PC: $" + hex(nes.cpu.pc, 4));
		DrawString(x , y + 20, "A: $" +  hex(nes.cpu.a, 2) + "  [" + std::to_string(nes.cpu.a) + "]");
		DrawString(x , y + 30, "X: $" +  hex(nes.cpu.x, 2) + "  [" + std::to_string(nes.cpu.x) + "]");
		DrawString(x , y + 40, "Y: $" +  hex(nes.cpu.y, 2) + "  [" + std::to_string(nes.cpu.y) + "]");
		DrawString(x , y + 50, "Stack P: $" + hex(nes.cpu.stkp, 4));
	}

	void DrawCode(int x, int y, int nLines)
	{
		auto it_a = mapAsm.find(nes.cpu.pc);
		int nLineY = (nLines >> 1) * 10 + y;
		
		if (it_a != mapAsm.end())
		{
			DrawString(x, nLineY, (*it_a).second, olc::CYAN);
			while (nLineY < (nLines * 10) + y)
			{
				nLineY += 10;
				if (++it_a != mapAsm.end())
				{
					DrawString(x, nLineY, (*it_a).second);
				}
			}
		}

		it_a = mapAsm.find(nes.cpu.pc);
		nLineY = (nLines >> 1) * 10 + y;
		if (it_a != mapAsm.end())
		{
			while (nLineY > y)
			{
				nLineY -= 10;
				if (--it_a != mapAsm.end())
				{
					DrawString(x, nLineY, (*it_a).second);
				}
			}
		}		
	}

	void LoadDefaultProgram()
	{
		// Load Program (assembled at https://www.masswerk.at/6502/assembler.html)
		/*
			*=$8000
			LDA #3
			STA $01
			LDA #0
			LDY #11			 
			CLC
			loop
			ADC $01
			DEY
			BNE loop
			STA $02
			NOP
			NOP
			NOP
		*/
		
		std::stringstream ss;
		ss << "A9 03 85 01 A9 00 A0 0B 18 65 01 88 D0 FB 85 02 EA EA EA";

		while (!ss.eof())
		{	//Convert hex string into bytes and store in prog_buf
			std::string b;
			ss >> b;
			uint8_t bb = (uint8_t)std::stoul(b, nullptr, 16);
				// debug msg std::cerr << "b: " << b << ", bb: " << std::to_string(bb) << std::endl;
			prog_buf.push_back(bb); 
		}
	}

    bool loadProgramFromFile(const char *fileName) noexcept
        {
        // Try to open a file specified by its name
        std::ifstream file(fileName, std::ios::in | std::ios::binary);
        if (!file.is_open() || file.bad())
            return false;

        // Clear whitespace removal flag
        file.unsetf(std::ios::skipws);

        // Determine size of the file
        file.seekg(0, std::ios_base::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios_base::beg);

        // Discard previous vector content
        /* prog_buf.resize(0);
        prog_buf.reserve(0);
        prog_buf.shrink_to_fit(); */

        // Order to prealocate memory to avoid unnecessary reallocations due to vector growth
        prog_buf.reserve(fileSize);

        // Read entire file content into prealocated vector memory
        prog_buf.insert(begin(prog_buf),
            std::istream_iterator<uint8_t>(file),
            std::istream_iterator<uint8_t>());

        // Make sure entire content is loaded
        return size(prog_buf) == fileSize;
        }

	// Reset CPU
	void ResetCPU()
	{
		nes.cpu.reset();
		Loop.on = false;
		StepCPU(1);			// step CPU once to fix no response to space first time in OnUserUpdate
	}

	// Step CPU [numStep] times
	void StepCPU(uint16_t numStep)
	{
		for (uint16_t i = 0; i < numStep; i++) {
			do
			{
				nes.cpu.clock();
			} 
			while (!nes.cpu.complete());
		} 
	}


	bool OnUserCreate()
	{
		// load program from prog_buf into emulated ram
		uint16_t nOffset = 0x8000;
		for (int i=0; i<size(prog_buf); i++)
		{
			nes.ram[nOffset+i] = prog_buf[i];
		}

		// Set Reset Vector
		nes.ram[0xFFFC] = 0x00;
		nes.ram[0xFFFD] = 0x80;

		// Dont forget to set IRQ and NMI vectors if you want to play with those
				
		// Extract dissassembly
		mapAsm = nes.cpu.disassemble(0x0000, 0xFFFF);

		// Reset
		ResetCPU();

		// clear variables for key functions
		KeyDownTime = 0;

		return true;

	}

	bool OnUserUpdate(float fElapsedTime)
	{
		Clear(olc::DARK_BLUE);

		if (GetKey(olc::Key::SPACE).bPressed)
		{
			KeyDownTime = 0;
			Loop.on = false;		// space press stops any looping
			StepCPU(1);
		}

		if (GetKey(olc::Key::SPACE).bHeld)
		{
			KeyDownTime = KeyDownTime + fElapsedTime;
			if (KeyDownTime > 0.5)	// delay 500msec before continuous stepping when <Space> is held down 
			StepCPU(1);
		}

		if (GetKey(olc::Key::L).bPressed)	// loop once 
		{
			Loop.LoopEnd = nes.cpu.pc;
			Loop.on = true;
			StepCPU(1);
		}

		if (GetKey(olc::Key::C).bPressed)	// continuous looping
		{
			Loop.LoopEnd = nes.cpu.pc + 1;
			Loop.on = true;
			StepCPU(1);
		}

		if (Loop.on)
		{
			if (nes.cpu.pc < Loop.LoopEnd)	// continue stepping until loop complete
				StepCPU(1);
			else
				Loop.on = false;			// reset loop flag
		}


		if (GetKey(olc::Key::R).bPressed)
			ResetCPU();

		if (GetKey(olc::Key::I).bPressed)
			nes.cpu.irq();

		if (GetKey(olc::Key::N).bPressed)
			nes.cpu.nmi();

		// Draw Ram Page 0x00		
		DrawRam(2, 2, 0x0000, 16, 16);
		DrawRam(2, 182, 0x8000, 16, 16);
		DrawCpu(448, 2);
		DrawCode(448, 72, 26);


		DrawString(10, 370, "SPACE = Step Instruction    C = Loop Once    C = Loop Continuously");
		DrawString(10, 380, "R = RESET    I = IRQ    N = NMI");

		return true;
	}
};


int main(int argc, char* argv[])
{
	if (argc>2) {
		std::cerr << "Usage: " << argv[0] << " [OPTION] [FILE]" << std::endl;
		return 1;
	}

	Demo_olc6502 demo;

	if (argc<2)	{
		demo.LoadDefaultProgram();		// if no filename given, load a short default demo program
	}

	if (argc==2) {
		if (!demo.loadProgramFromFile(argv[1])) {
			std::cerr << "Error reading file " << argv[1] << std::endl;
			return 1;
		}
	}

	demo.Construct(680, 480, 2, 2);
	demo.Start();

	return 0;
}