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

#ifndef BEEPCM_OKIM6295
#define BEEPCM_OKIM6295

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
using namespace std;

namespace beepcm
{
    class OkiM6295
    {
	public:
	    OkiM6295();
	    ~OkiM6295();

	    uint32_t get_sample_rate(uint32_t clock_rate);
	    void init();
	    bool writeVGM(uint8_t offs, uint8_t data);
	    void writeCmd(uint8_t data);
	    bool setPin7(bool is_set);
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

	    bool is_pin7_set = false;

	    int m6295_cmd = -1;

	    struct okim_voice
	    {
		int number = 0;
		bool is_playing = false;
		uint32_t current_addr = 0;
		uint32_t start_addr = 0;
		uint32_t end_addr = 0;
		uint32_t sample_num = 0;
		uint32_t num_samples = 0;
		int32_t volume = 0;
		uint8_t current_byte = 0;
		uint8_t current_step = 0;
		int16_t prev_signal = 0;
		int16_t current_signal = 0;
		bool is_high_nibble = false;
		int32_t output = 0;
	    };

	    array<okim_voice, 4> voices;

	    void generateSample(okim_voice &voice);

	    vector<uint8_t> m6295_rom;

	    uint8_t readROM(uint32_t addr);

	    array<int32_t, 16> vol_table = 
	    {
		0x20, 0x16, 0x10, 0x0B,
		0x08, 0x06, 0x04, 0x03,
		0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	    };

	    array<uint16_t, 49> step_table = 
	    {
		 16,  17,  19,  21,   23,   25,   28,   31,
		 34,  37,  41,  45,   50,   55,   60,   66,
		 73,  80,  88,  97,  107,  118,  130,  143,
		157, 173, 190, 209,  230,  253,  279,  307,
		337, 371, 408, 449,  494,  544,  598,  658,
		724, 796, 876, 963, 1060, 1166, 1282, 1411,
		1552
	    };

	    array<int8_t, 16> delta_table = 
	    {
		 1,  3,  5,  7,  9,  11,  13,  15,
		-1, -3, -5, -7, -9, -11, -13, -15
	    };

	    array<int8_t, 8> adjust_table =
	    {
		-1, -1, -1, -1,
		 2,  4,  6,  8
	    };
    };
};

#endif // BEEPCM_OKIM6295