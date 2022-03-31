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

// BeePCM-SegaPCM
// Chip Name: SegaPCM (315-5218)
// Chip Used In: Countless Sega arcade machines
//
// Interesting Trivia:
//
// The SegaPCM chip is particularly infamous amongst arcade machine repair
// enthusiasts for both the chip's absurdly high failure rate and the fact that its
// precise internal workings are completely undocumented (other than what's already known
// in MAME's own software emulator for the chip), forcing people repairing arcade boards that
// use this chip to swipe a replacement chip from a different arcade PCB.
//
//
// BueniaDev's Notes:
//
// While all the major features of this chip (as far as what is already known about it) are
// fully implemented, this implementation may possibly be rewritten once
// furrtek finishes up the (reverse-engineered) schematics for this chip,
// you know, in the name of accuracy.

#include "segapcm.h"
using namespace beepcm;

namespace beepcm
{
    SegaPCM::SegaPCM()
    {

    }

    SegaPCM::~SegaPCM()
    {

    }

    uint32_t SegaPCM::get_sample_rate(uint32_t clock_rate)
    {
	return (clock_rate / 128);
    }

    void SegaPCM::set_bank(uint32_t bank)
    {
	bank_shift = (bank & 0xF);
	bank_mask = (0x70 | ((bank >> 16) & 0xFC));
    }

    void SegaPCM::init()
    {
	reset();
    }

    void SegaPCM::reset()
    {
	bank_shift = 12;
	bank_mask = 0x70;

	low_addr.fill(0);
	ch_outputs.fill({0, 0});
	pcm_ram.fill(0xFF);
    }

    void SegaPCM::writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data)
    {
	pcm_rom.resize(rom_size, 0x80);

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

	copy(rom_data.begin(), (rom_data.begin() + data_length), (pcm_rom.begin() + data_start));
    }

    uint8_t SegaPCM::readRAM(uint16_t addr)
    {
	return pcm_ram[(addr & 0x7FF)];
    }

    void SegaPCM::writeRAM(uint16_t addr, uint8_t data)
    {
	pcm_ram[(addr & 0x7FF)] = data;
    }

    uint8_t SegaPCM::get_reg(int ch, uint8_t offs)
    {
	uint32_t addr = ((ch * 8) + offs);
	return pcm_ram[addr];
    }

    void SegaPCM::set_reg(int ch, uint8_t offs, uint8_t data)
    {
	uint32_t addr = ((ch * 8) + offs);
	pcm_ram[addr] = data;
    }

    uint8_t SegaPCM::fetch_rom(uint32_t addr)
    {
	return (addr < pcm_rom.size()) ? pcm_rom.at(addr) : 0x80;
    }

    void SegaPCM::clockchip()
    {
	for (int ch = 0; ch < 16; ch++)
	{
	    if (!testbit(get_reg(ch, 0x86), 0))
	    {
		uint32_t offset = ((get_reg(ch, 0x86) & bank_mask) << bank_shift);
		uint8_t addr_msb = get_reg(ch, 0x85);
		uint8_t addr_lsb = get_reg(ch, 0x84);

		uint8_t end_val = (get_reg(ch, 0x06) + 1);
		uint32_t addr_inc = get_reg(ch, 0x07);

		uint32_t vol_left = (get_reg(ch, 0x02) & 0x7F);
		uint32_t vol_right = (get_reg(ch, 0x03) & 0x7F);

		uint32_t addr = ((addr_msb << 16) | (addr_lsb << 8) | low_addr[ch]);

		bool is_ch_disabled = false;

		if ((addr >> 16) == end_val)
		{
		    if (testbit(get_reg(ch, 0x86), 1))
		    {
			set_reg(ch, 0x86, setbit(get_reg(ch, 0x86), 0));
			is_ch_disabled = true;
		    }
		}

		int16_t left = 0;
		int16_t right = 0;

		if (!is_ch_disabled)
		{
		    int8_t value = (fetch_rom(offset + (addr >> 8)) - 0x80);
		    left = (value * vol_left);
		    right = (value * vol_right);
		    addr = ((addr + addr_inc) & 0xFFFFFF);
		}

		ch_outputs[ch][0] = left;
		ch_outputs[ch][1] = right;

		set_reg(ch, 0x84, (addr >> 8));
		set_reg(ch, 0x85, (addr >> 16));
		low_addr[ch] = testbit(get_reg(ch, 0x86), 0) ? 0 : addr;
	    }
	}
    }

    vector<int32_t> SegaPCM::get_samples()
    {
	vector<int32_t> final_samples;
	array<int32_t, 2> mixed_samples = {0, 0};

	for (auto &sample : ch_outputs)
	{
	    for (int i = 0; i < 2; i++)
	    {
		int32_t old_sample = mixed_samples[i];
		int32_t new_sample = clamp<int16_t>(sample[i], -32768, 32767);

		int32_t result = (old_sample + new_sample);
		mixed_samples[i] = clamp(result, -32768, 32767);
	    }
	}

	final_samples.push_back(mixed_samples[0]);
	final_samples.push_back(mixed_samples[1]);

	return final_samples;
    }
};