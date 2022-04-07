/*
    This file is part of the BeePCM engine.
    Copyright (C) 2022 BueniaDev.

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

#ifndef BEEPCM_RF5C68
#define BEEPCM_RF5C68

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
using namespace std;

namespace beepcm
{
    class RF5C68
    {
	public:
	    RF5C68();
	    ~RF5C68();

	    uint32_t get_sample_rate(uint32_t clock_rate);
	    void init();
	    void enable_vgm_hack(bool is_enabled = true);
	    void writeRAM(int data_start, int data_len, vector<uint8_t> ram_data);
	    void writemem(uint16_t addr, uint8_t data);
	    void writereg(uint8_t reg, uint8_t data);

	    void clockchip();
	    vector<int32_t> get_samples();

	private:
	    template<typename T>
	    bool testbit(T reg, int bit)
	    {
		return ((reg >> bit) & 1);
	    }

	    void reset();

	    struct rf5c68_channel
	    {
		uint8_t envelope = 0;
		uint8_t pan = 0;
		uint16_t step = 0;
		uint16_t loop_start = 0;
		uint8_t start_byte = 0;
		uint32_t current_addr = 0;
		array<int32_t, 2> output = {0, 0};
		bool is_enable = false;
	    };

	    array<rf5c68_channel, 8> rf5c68_channels;
	    array<uint8_t, 0x10000> rf5c68_ram;
	    bool rf5c68_enable = false;

	    int mem_bank = 0;
	    int ch_bank = 0;

	    bool is_vgm_hack = false;

	    uint32_t vgm_base_addr = 0;
	    uint32_t vgm_cur_addr = 0;
	    uint32_t vgm_end_addr = 0;
	    uint16_t vgm_cur_step = 0;
	    vector<uint8_t> vgm_data;

	    void check_vgm_samples(uint32_t addr, uint16_t speed);
	    void flush_vgm();
    };
};

#endif // BEEPCM_RF5C68