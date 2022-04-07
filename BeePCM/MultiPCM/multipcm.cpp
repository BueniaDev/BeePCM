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

// BeePCM-MultiPCM (WIP)
// Chip Name: MultiPCM (315-5560)
// Chip Used In: Countless Sega arcade machines
//
// Interesting Trivia:
//
// This chip was famously utilized in Sega's early polygonal-3D arcade machines from the early 1990s, including Model 1 and early Model 2 hardware.
//
//
// BueniaDev's Notes:
//
// TODO: Write up initial notes

#include "multipcm.h"
using namespace beepcm;

namespace beepcm
{
    MultiPCM::MultiPCM()
    {

    }

    MultiPCM::~MultiPCM()
    {

    }

    void MultiPCM::init_tables()
    {
	for (int i = 0; i < 0x400; i++)
	{
	    float fcent = (float(chip_sample_rate) * (1024.f + float(i)) / 1024.f);
	    freq_step_table.at(i) = uint32_t(float(1 << 12) * fcent);
	}

	for (int level = 0; level < 0x80; level++)
	{
	    float vol_db = ((float(level) * -24.f) / 64.f);
	    float total_level = (powf(10.f, (vol_db / 20.f)) / 4.f);

	    for (int pan = 0; pan < 0x10; pan++)
	    {
		float pan_left = 1.0;
		float pan_right = 1.0;

		int pan_table_offs = ((pan << 7) | level);
		m_pan_table[0][pan_table_offs] = int32_t(float(1 << 12) * (pan_left * total_level));
		m_pan_table[1][pan_table_offs] = int32_t(float(1 << 12) * (pan_right * total_level));
	    }
	}

	float numerator_tll = float(0x80 << 12);
	tll_steps[0] = (-numerator_tll / (78.2f * 44100.f / 1000.f)); // Lower TLL
	tll_steps[1] = (numerator_tll / (78.2f * 2 * 44100.f / 1000.f)); // Raise TLL
    }

    int MultiPCM::value_to_channel(int val)
    {
	array<int, 32> ch_map = 
	{
	    0,  1,  2,  3,  4,  5,  6,  -1,
	    7,  8,  9,  10, 11, 12, 13, -1,
	    14, 15, 16, 17, 18, 19, 20, -1,
	    21, 22, 23, 24, 25, 26, 27, -1
	};

	return ch_map[(val & 0x1F)];
    }

    uint8_t MultiPCM::read_rom(uint32_t addr)
    {
	return (addr < multipcm_rom.size()) ? multipcm_rom.at(addr) : 0;
    }

    void MultiPCM::init_sample(multipcm_channel &channel)
    {
	uint32_t address = (channel.sample_index * 12);
	uint32_t start_addr = ((read_rom(address) << 16) | (read_rom(address + 1) << 8) | read_rom(address + 2));
	channel.metadata.format = ((start_addr >> 20) & 0xFE);
	channel.metadata.start_addr = (start_addr & 0x3FFFFF);
	channel.metadata.loop_addr = ((read_rom(address + 3) << 8) | read_rom(address + 4));
	channel.metadata.end_addr = (0xFFFF - ((read_rom(address + 5) << 8) | read_rom(address + 6)));
    }

    void MultiPCM::retrigger_sample(multipcm_channel &channel)
    {
	channel.offset = 0;
	channel.prev_sample = 0;
	channel.tll_val = (channel.total_level << 12);

	if ((chip_bank != 0) && (channel.base_addr >= 0x100000))
	{
	    channel.base_addr &= 0xFFFFF;
	    channel.base_addr |= chip_bank;
	}
    }

    void MultiPCM::writeSlot(int cur_slot, int address, uint8_t data)
    {
	if (cur_slot == -1)
	{
	    return;
	}

	auto &channel = channels[cur_slot];

	switch (address)
	{
	    case 0:
	    {
		cout << "Setting panpot of channel " << dec << cur_slot << endl;
	    }
	    break;
	    case 1:
	    {
		channel.sample_index = ((channel.sample_index & 0x100) | data);
		init_sample(channel);

		channel.format = channel.metadata.format;
		channel.base_addr = channel.metadata.start_addr;

		if (channel.is_playing)
		{
		    retrigger_sample(channel);
		}
	    }
	    break;
	    case 2:
	    {
		channel.sample_index = ((channel.sample_index & 0xFF) | ((data & 0x1) << 8));
		channel.pitch = ((channel.pitch & 0x3C0) | (data >> 2));
		uint32_t pitch = freq_step_table.at(channel.pitch);

		if (testbit(channel.octave, 3))
		{
		    pitch >>= (16 - channel.octave);
		}
		else
		{
		    pitch <<= channel.octave;
		}

		// Basic error checking for valid chip sample rates
		if (chip_sample_rate == 0)
		{
		    throw runtime_error("Chip sample rate can not be 0 (did you forget to call get_sample_rate()?)");
		}

		channel.step = (pitch / chip_sample_rate);
	    }
	    break;
	    case 3:
	    {
		channel.octave = (((data >> 4) - 1) & 0xF);
		channel.pitch = ((channel.pitch & 0x3F) | ((data & 0xF) << 6));
		uint32_t pitch = freq_step_table.at(channel.pitch);

		if (testbit(channel.octave, 3))
		{
		    pitch >>= (16 - channel.octave);
		}
		else
		{
		    pitch <<= channel.octave;
		}

		// Basic error checking for valid chip sample rates
		if (chip_sample_rate == 0)
		{
		    throw runtime_error("Chip sample rate can not be 0 (did you forget to call get_sample_rate()?)");
		}

		channel.step = (pitch / chip_sample_rate);
	    }
	    break;
	    case 4:
	    {
		if (testbit(data, 7))
		{
		    channel.is_playing = true;
		    retrigger_sample(channel);
		}
		else
		{
		    if (channel.is_playing)
		    {
			channel.is_playing = false;
		    }
		}
	    }
	    break;
	    case 5:
	    {
		cout << "Setting total level of channel " << dec << cur_slot << endl;
		channel.total_level = ((data >> 1) & 0x7F);

		if (!testbit(data, 0))
		{
		    if ((channel.tll_val >> 12) > channel.total_level)
		    {
			channel.tll_step = tll_steps[0]; // Decrease TLL
		    }
		    else
		    {
			channel.tll_step = tll_steps[1]; // Increase TLL
		    }
		}
		else
		{
		    channel.tll_val = (channel.total_level << 12);
		}
	    }
	    break;
	    case 6:
	    {
		cout << "Setting LFO frequency/pitch LFO of channel " << dec << cur_slot << endl;
	    }
	    break;
	    case 7:
	    {
		cout << "Setting amplitude LFO of channel " << dec << cur_slot << endl;
	    }
	    break;
	}
    }

    uint32_t MultiPCM::get_sample_rate(uint32_t clock_rate)
    {
	chip_sample_rate = (clock_rate / 224.0);
	init_tables();
	return chip_sample_rate;
    }

    void MultiPCM::init()
    {
	reset();
    }

    void MultiPCM::reset()
    {
	chip_bank = 0;
	left_bank = 0;
	right_bank = 0;
    }

    void MultiPCM::writeBankVGM(uint8_t offset, uint16_t data)
    {
	if ((offset == 3) && !testbit(data, 3))
	{
	    int bank_val = (data >> 4);
	    writeBank1M(bank_val);
	}
	else
	{
	    int bank_val = (data >> 3);

	    if (testbit(offset, 1))
	    {
		writeBank512K(bank_val, true);
	    }

	    if (testbit(offset, 0))
	    {
		writeBank512K(bank_val, false);
	    }
	}
    }

    void MultiPCM::writeBank1M(int bank)
    {
	chip_bank = (bank << 20);
    }

    void MultiPCM::writeBank512K(int bank, bool is_lowbank)
    {
	if (is_lowbank)
	{
	    right_bank = (bank << 19);
	}
	else
	{
	    left_bank = (bank << 19);
	}

	chip_bank = (right_bank != 0) ? right_bank : left_bank;
    }

    void MultiPCM::writeIO(int port, uint8_t data)
    {
	port &= 3;
	switch (port)
	{
	    case 0: writeSlot(ch_num, cur_address, data); break;
	    case 1: ch_num = value_to_channel(data); break;
	    case 2: cur_address = min<uint8_t>(data, 7); break;
	    default: break;
	}
    }

    void MultiPCM::writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data)
    {
	multipcm_rom.resize(rom_size, 0xFF);

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

	copy(rom_data.begin(), (rom_data.begin() + data_length), (multipcm_rom.begin() + data_start));
    }

    void MultiPCM::clockchip()
    {
	for (auto &channel : channels)
	{
	    if (channel.is_playing)
	    {
		uint32_t volume = ((channel.tll_val >> 12));
		uint32_t spos = (channel.offset >> 12);
		uint32_t step = channel.step;
		int32_t csample = 0;
		int32_t fpart = (channel.offset & 0xFFF);

		if (testbit(channel.format, 3))
		{
		    // TODO: 12-bit linear
		    cout << "12-bit linear" << endl;
		    continue;
		}
		else
		{
		    csample = int16_t(read_rom(channel.base_addr + spos) << 8);
		}

		int32_t sample = ((csample * fpart + channel.prev_sample * ((1 << 12) - fpart)) >> 12);
		channel.offset += step;

		if (channel.offset >= (channel.metadata.end_addr << 12))
		{
		    channel.offset = (channel.metadata.loop_addr << 12);
		}

		if (spos ^ (channel.offset >> 12))
		{
		    channel.prev_sample = csample;
		}

		if ((channel.tll_val >> 12) != channel.total_level)
		{
		    channel.tll_val += channel.tll_step;
		}

		sample = ((sample * 4052) >> 10);

		channel.output[0] = ((sample * m_pan_table[0][volume]) >> 12);
		channel.output[1] = ((sample * m_pan_table[1][volume]) >> 12);
	    }
	}
    }

    vector<int32_t> MultiPCM::get_samples()
    {
	array<int32_t, 2> mixed_samples = {0, 0};

	for (auto &channel : channels)
	{
	    for (int i = 0; i < 2; i++)
	    {
		int32_t old_sample = mixed_samples[i];
		int32_t new_sample = clamp(channel.output[i], -32768, 32767);
		mixed_samples[i] = (old_sample + new_sample);
	    }
	}

	vector<int32_t> final_samples;
	final_samples.push_back(mixed_samples[0]);
	final_samples.push_back(mixed_samples[1]);
	return final_samples;
    }
}