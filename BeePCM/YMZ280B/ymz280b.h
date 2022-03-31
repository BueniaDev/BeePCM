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

#ifndef BEEPCM_YMZ280B
#define BEEPCM_YMZ280B

#include <iostream>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <array>
using namespace std;

namespace beepcm
{
    struct ymcreative_voice
    {
	bool is_keyon = false;
	bool is_playing = false;
	bool is_looping = false;
	bool adpcm_loop = false;

	uint16_t freq_num = 0;
	int mode = 0;

	uint8_t level = 0;

	int pan = 0;
	int output_left = 0;
	int output_right = 0;

	uint32_t start_addr = 0;
	uint32_t loop_start_addr = 0;
	uint32_t loop_end_addr = 0;
	uint32_t stop_addr = 0;

	uint32_t current_addr = 0;
	uint32_t output_step = 0;

	bool is_high_nibble = false;
	uint8_t current_byte = 0;
	uint32_t output_pos = 0;
	int32_t prev_signal = 0;
	int32_t voice_step = 0;
	int32_t voice_signal = 0;
	int32_t loop_signal = 0;
	int32_t loop_step = 0;
	array<int32_t, 2> output;
    };

    struct ymcreative_debug
    {
	array<ymcreative_voice, 8> voices;
	bool master_keyon = false;

	ymcreative_voice get_voice(int channel)
	{
	    channel &= 7;
	    return voices.at(channel);
	}
    };

    class YMZ280B
    {
	public:
	    YMZ280B();
	    ~YMZ280B();

	    uint32_t get_sample_rate(uint32_t clock_rate);
	    void init();
	    void writeIO(int port, uint8_t data);
	    void writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data);
	    void clockchip();
	    vector<int32_t> get_samples();

	    void writeROM(vector<uint8_t> rom_data)
	    {
		writeROM(rom_data.size(), 0, rom_data.size(), rom_data);
	    }

	    void fillROM(uint8_t byte)
	    {
		vector<uint8_t> rom_data(0x1000000, byte);
		writeROM(rom_data);
	    }

	    ymcreative_debug get_debug()
	    {
		ymcreative_debug debug;
		debug.voices = voices;
		debug.master_keyon = master_keyon;
		return debug;
	    }

	private:
	    template<typename T>
	    bool testbit(T reg, int bit)
	    {
		return ((reg >> bit) & 1);
	    }

	    void reset();

	    uint8_t chip_address = 0;

	    void writereg(uint8_t reg, uint8_t data);

	    bool master_keyon = false;

	    array<ymcreative_voice, 8> voices;

	    void update_step(ymcreative_voice &voice);
	    void update_volumes(ymcreative_voice &voice);

	    void key_on(ymcreative_voice &voice);
	    void key_off(ymcreative_voice &voice);

	    void channel_output(ymcreative_voice &voice);
	    void generate_adpcm_sample(ymcreative_voice &voice);
	    void generate_pcm8(ymcreative_voice &voice);
	    void generate_pcm16(ymcreative_voice &voice);

	    uint8_t fetch_rom(uint32_t addr);

	    vector<uint8_t> ymz280b_rom;
    };
};



#endif // BEEPCM_YMZ280B