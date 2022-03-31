/*
    This file is part of the BeePCM engine.
    Copyright (C) 2021 BueniaDev.

    BeePCM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BeePCM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BeePCM.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef BEEPCM_SEGAPCM
#define BEEPCM_SEGAPCM

#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <array>
using namespace std;

namespace beepcm
{
    class SegaPCM
    {
	public:
	    SegaPCM();
	    ~SegaPCM();

	    uint32_t get_sample_rate(uint32_t clock_rate);
	    void set_bank(uint32_t bank);
	    void init();
	    void writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data);
	    uint8_t readRAM(uint16_t addr);
	    void writeRAM(uint16_t addr, uint8_t data);
	    void clockchip();
	    vector<int32_t> get_samples();

	    void writeROM(vector<uint8_t> rom_data)
	    {
		writeROM(rom_data.size(), 0, rom_data.size(), rom_data);
	    }

	private:
	    template<typename T>
	    bool testbit(T reg, int bit)
	    {
		return ((reg >> bit) & 1);
	    }

	    template<typename T>
	    T setbit(T reg, int bit)
	    {
		return (reg | (1 << bit));
	    }

	    void reset();

	    int bank_shift = 0;
	    int bank_mask = 0;

	    vector<uint8_t> pcm_rom;
	    array<uint8_t, 0x800> pcm_ram;

	    array<uint8_t, 16> low_addr;

	    array<array<int16_t, 2>, 16> ch_outputs;

	    uint8_t get_reg(int ch, uint8_t offs);
	    void set_reg(int ch, uint8_t offs, uint8_t data);

	    uint8_t fetch_rom(uint32_t addr);
    };
};

#endif // BEEPCM_SEGAPCM

