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

// BeePCM-uPD7759
// Chip Name: NEC uPD7759
// Chip Used In: Sega System 16, Konami arcade machines, and a bunch of others as well
//
// Interesting Trivia:
//
// The uPD7759 actually came in an array of different varieties.
// Most of those use an (yet-to-be-decapped)
// internal mask ROM, but only the uPD7759 uses external ROM.
//
// BueniaDev's Notes:
//
// This core is pretty much complete, AFAIK.
// Despite this, there are still a few features that could possibly be implemented in the future,
// including VGM player compatibility, as well as a few (possible) others.

#include "upd7759.h"
using namespace beepcm;

namespace beepcm
{
    uPD7759::uPD7759()
    {

    }

    uPD7759::~uPD7759()
    {

    }

    uint32_t uPD7759::get_sample_rate(uint32_t clock_rate)
    {
	return (clock_rate / 4);
    }

    void uPD7759::advance_state()
    {
	switch (state)
	{
	    case DropDRQ:
	    {
		is_drq = false;

		clocks_left = drq_clocks;
		state = drq_state;
	    }
	    break;
	    case Start:
	    {
		uint8_t rom_byte = fifo_in;
		requested_sample = rom_byte;

		clocks_left = 70;
		state = FirstReq;
	    }
	    break;
	    case FirstReq:
	    {
		is_drq = true;

		clocks_left = 44;
		state = LastSample;
	    }
	    break;
	    case LastSample:
	    {
		uint8_t rom_byte = fetchROM(0);
		is_drq = true;

		last_sample = rom_byte;

		clocks_left = 28;

		state = (requested_sample > last_sample) ? Idle : Dummy1;
	    }
	    break;
	    case Dummy1:
	    {
		is_drq = true;

		clocks_left = 32;
		state = AddrMSB;
	    }
	    break;
	    case AddrMSB:
	    {
		uint8_t rom_byte = fetchROM(requested_sample * 2 + 5);
		offset = (rom_byte << 9);
		is_drq = true;

		clocks_left = 44;
		state = AddrLSB;
	    }
	    break;
	    case AddrLSB:
	    {
		uint8_t rom_byte = fetchROM(requested_sample * 2 + 6);
		offset |= (rom_byte << 1);
		is_drq = true;

		clocks_left = 36;
		state = Dummy2;
	    }
	    break;
	    case Dummy2:
	    {
		offset += 1;
		is_valid_header = false;
		is_drq = true;

		clocks_left = 36;
		state = BlockHeader;
	    }
	    break;
	    case BlockHeader:
	    {
		if (repeat_count > 0)
		{
		    repeat_count -= 1;
		    offset = repeat_offset;
		}

		uint8_t rom_byte = fetchROM(offset++);

		block_header = rom_byte;
		is_drq = true;

		switch (block_header & 0xC0)
		{
		    case 0x00:
		    {
			clocks_left = (1024 * ((block_header & 0x3F) + 1));
			state = ((block_header == 0) && is_valid_header) ? Idle : BlockHeader;
			adpcm_sample = 0;
			adpcm_state = 0;
		    }
		    break;
		    case 0x40:
		    {
			sample_rate = ((block_header & 0x3F) + 1);
			nibbles_left = 256;
			clocks_left = 36;
			state = NibbleMSN;
		    }
		    break;
		    case 0x80:
		    {
			sample_rate = ((block_header & 0x3F) + 1);
			clocks_left = 36;
			state = NibbleCount;
		    }
		    break;
		    case 0xC0:
		    {
			repeat_count = ((block_header & 0x7) + 1);
			repeat_offset = offset;
			clocks_left = 36;
			state = BlockHeader;
		    }
		    break;
		    default:
		    {
			cout << "Unrecognized ROM header type of " << hex << int(block_header & 0xC0) << endl;
			exit(0);
		    }
		    break;
		}

		if (block_header != 0)
		{
		    is_valid_header = true;
		}
	    }
	    break;
	    case NibbleCount:
	    {
		uint8_t rom_byte = fetchROM(offset++);
		is_drq = true;
		nibbles_left = (rom_byte + 1);
		clocks_left = 36;
		state = NibbleMSN;
	    }
	    break;
	    case NibbleMSN:
	    {
		uint8_t rom_byte = fetchROM(offset++);
		adpcm_data = rom_byte;
		update_adpcm(adpcm_data >> 4);
		is_drq = true;

		clocks_left = (sample_rate * 4);

		if (--nibbles_left == 0)
		{
		    state = BlockHeader;
		}
		else
		{
		    state = NibbleLSN;
		}
	    }
	    break;
	    case NibbleLSN:
	    {
		update_adpcm(adpcm_data & 0xF);
		clocks_left = (sample_rate * 4);

		if (--nibbles_left == 0)
		{
		    state = BlockHeader;
		}
		else
		{
		    state = NibbleMSN;
		}
	    }
	    break;
	    default:
	    {
		cout << "Unrecognized state of " << dec << int(state) << endl;
		exit(0);
	    }
	    break;
	}

	if (is_drq)
	{
	    drq_state = state;
	    drq_clocks = (clocks_left - 21);
	    state = DropDRQ;
	    clocks_left = 21;
	}
    }

    void uPD7759::update_adpcm(int data)
    {
	adpcm_sample += step_table[adpcm_state][data];
	adpcm_state += state_table[data];

	adpcm_state = clamp<int8_t>(adpcm_state, 0, 15);
    }

    void uPD7759::init()
    {
	is_reset = true;
	is_start = true;
	is_drq = false;
	reset();
    }

    void uPD7759::reset()
    {
	state = Idle;
	position = 0;
	clocks_left = 0;
	sample_rate = 0;
	is_valid_header = 0;
	offset = 0;
	block_header = 0;
	drq_state = Idle;
	drq_clocks = 0;
	requested_sample = 0;
	last_sample = 0;
	adpcm_sample = 0;
	repeat_offset = 0;
	adpcm_state = 0;
	adpcm_data = 0;

	if (is_drq)
	{
	    is_drq = false;
	}
    }

    uint8_t uPD7759::fetchROM(uint32_t addr)
    {
	return (addr < speech_rom.size()) ? speech_rom.at(addr) : 0;
    }

    bool uPD7759::read_busy()
    {
	return (state == Idle);
    }

    void uPD7759::write_start(bool line)
    {
	bool prev_start = is_start;
	is_start = line;

	if ((state == Idle) && (prev_start && !is_start) && is_reset)
	{
	    state = Start;
	}
    }

    void uPD7759::write_reset(bool line)
    {
	bool prev_reset = is_reset;

	is_reset = line;

	if (prev_reset && !is_reset)
	{
	    reset();
	}
    }

    void uPD7759::write_port(uint8_t data)
    {
	fifo_in = data;
    }

    void uPD7759::writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data)
    {
	speech_rom.resize(rom_size, 0xFF);

	uint32_t data_length = data_len;
	uint32_t data_end = (data_start + data_len);

	if (data_start > rom_size)
	{
	    return;
	}

	if (data_end > rom_size)
	{
	    data_length = (rom_size - data_start);
	}

	copy(rom_data.begin(), (rom_data.begin() + data_length), (speech_rom.begin() + data_start));
    }

    void uPD7759::clock_chip()
    {
	uint32_t step = 0x400000;
	if (state != Idle)
	{
	    output_sample = (adpcm_sample << 7);
	    uint32_t current_pos = (position + step);

	    if (current_pos < 0x100000)
	    {
		return;
	    }

	    int clocks_this_time = (current_pos >> 20);

	    if (clocks_this_time > clocks_left)
	    {
		clocks_this_time = clocks_left;
	    }

	    position = (current_pos - (clocks_this_time << 20));
	    clocks_left -= clocks_this_time;

	    if (clocks_left == 0)
	    {
		advance_state();
		if (state != Idle)
		{
		    output_sample = (adpcm_sample << 7);
		}
	    }
	}
    }

    vector<int32_t> uPD7759::get_samples()
    {
	vector<int32_t> samples;
	samples.push_back(output_sample);
	return samples;
    }
};