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

#ifndef BEEPCM_MULTIPCM
#define BEEPCM_MULTIPCM

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
    class MultiPCM
    {
	public:
	    MultiPCM();
	    ~MultiPCM();

	    uint32_t get_sample_rate(uint32_t clock_rate);
	    void init();
	    void configure_clock(uint32_t clock_rate);
	    void writeBankVGM(uint8_t offset, uint16_t data);
	    void writeBank1M(int bank);
	    void writeBank512K(int bank, bool is_lowbank);
	    void writeIO(int port, uint8_t data);
	    void writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data);

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

	    void reset();
	    void init_tables();

	    int ch_num = 0;
	    int cur_address = 0;

	    array<uint32_t, 0x400> freq_step_table;
	    array<array<int32_t, 0x800>, 2> m_pan_table;
	    array<int32_t, 2> tll_steps;

	    int chip_bank = 0;
	    int left_bank = 0;
	    int right_bank = 0;

	    int value_to_channel(int val);
	    void writeSlot(int cur_slot, int address, uint8_t data);

	    struct multipcm_data
	    {
		uint32_t start_addr = 0;
		uint32_t format = 0;
		uint16_t loop_addr = 0;
		uint16_t end_addr = 0;
	    };

	    struct multipcm_channel
	    {
		uint32_t pitch = 0;
		uint8_t octave = 0;
		uint32_t step = 0;
		int32_t prev_sample = 0;
		uint32_t total_level = 0;
		int32_t tll_step = 0;
		uint32_t tll_val = 0;
		uint32_t base_addr = 0;
		uint16_t sample_index = 0;
		uint32_t offset = 0;
		uint32_t format = 0;
		bool is_playing = false;
		array<int32_t, 2> output = {0, 0};
		multipcm_data metadata;
	    };

	    array<multipcm_channel, 28> channels;

	    void init_sample(multipcm_channel &channel);
	    void retrigger_sample(multipcm_channel &channel);
	    uint8_t read_rom(uint32_t addr);

	    vector<uint8_t> multipcm_rom;

	    uint32_t chip_sample_rate = 0;
    };
};

#endif // BEEPCM_MULTIPCM