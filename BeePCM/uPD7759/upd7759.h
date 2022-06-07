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

#ifndef BEEPCM_UPD7759
#define BEEPCM_UPD7759

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
    class uPD7759
    {
	public:
	    uPD7759();
	    ~uPD7759();

	    uint32_t get_sample_rate(uint32_t clock_rate);
	    void init();

	    bool read_busy();
	    void write_start(bool line);
	    void write_reset(bool line);
	    void write_port(uint8_t data);

	    void writeROM(vector<uint8_t> rom_data)
	    {
		writeROM(rom_data.size(), 0, rom_data.size(), rom_data);
	    }

	    void writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data);

	    void clock_chip();
	    vector<int32_t> get_samples();

	private:
	    template<typename T>
	    bool testbit(T reg, int bit)
	    {
		return ((reg >> bit) & 1);
	    }

	    void reset();

	    ofstream file;

	    bool is_reset = false;
	    bool is_start = false;

	    enum uPD7759State : int
	    {
		Idle = 0,
		DropDRQ,
		Start,
		FirstReq,
		LastSample,
		Dummy1,
		AddrMSB,
		AddrLSB,
		Dummy2,
		BlockHeader,
		NibbleCount,
		NibbleMSN,
		NibbleLSN
	    };

	    uPD7759State state;
	    uPD7759State drq_state;

	    uint32_t position = 0;
	    uint32_t step = 0;
	    int clocks_left = 0;
	    int drq_clocks = 0;

	    uint32_t clock_period = 0;

	    uint8_t requested_sample = 0;
	    uint8_t last_sample = 0;
	    uint32_t offset = 0;

	    int16_t output_sample = 0;

	    bool is_valid_header = false;

	    uint8_t block_header = 0;

	    uint8_t sample_rate = 0;
	    uint16_t nibbles_left = 0;

	    uint8_t repeat_count = 0;

	    uint32_t repeat_offset = 0;

	    uint8_t adpcm_data = 0;

	    uint8_t fifo_in = 0;

	    bool is_drq = false;

	    int8_t adpcm_state = 0;
	    int16_t adpcm_sample = 0;
	    int16_t prev_sample = 0;

	    void update_adpcm(int data);

	    void advance_state();

	    uint8_t fetchROM(uint32_t addr);

	    vector<uint8_t> speech_rom;

	    bool dump = false;

	    #include "upd_tables.inl"
    };
};

#endif // BEEPCM_UPD7759