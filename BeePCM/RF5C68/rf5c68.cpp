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

// BeePCM-RF5C68 (WIP)
// Chip Name: Ricoh RF5C68 / Ricoh RF5C164
// Chip Used In: Sega System 18, Sega System 32, Mega CD, and FM Towns (Marty)
//
// Interesting Trivia:
//
// The RF5C68 chip, considered a core aspect of both the Mega CD addon and
// the FM Towns computer, was mostly used for sound effects and voices in
// arcade machines. In addition, the chip's current VGMPlay implementation
// relies on a crude hack to get certain VGMs working.
//
// BueniaDev's Notes:
//
// TODO: Write initial notes here

#include "rf5c68.h"
using namespace beepcm;

namespace beepcm
{
    RF5C68::RF5C68()
    {

    }

    RF5C68::~RF5C68()
    {

    }

    uint32_t RF5C68::get_sample_rate(uint32_t clock_rate)
    {
	return (clock_rate / 384.0);
    }

    void RF5C68::check_vgm_samples(uint32_t addr, uint16_t speed)
    {
	if (!is_vgm_hack)
	{
	    return;
	}

	uint32_t sample_speed = (speed >= 0x800) ? (speed >> 11) : 1;

	if (addr >= vgm_cur_addr)
	{
	    // Is the stream running too fast?
	    if ((addr - vgm_cur_addr) <= (sample_speed * 5))
	    {
		// If so, then delay the stream
		vgm_cur_addr -= (sample_speed * 4);
		if (vgm_cur_addr < vgm_base_addr)
		{
		    vgm_cur_addr = vgm_base_addr;
		}
	    }
	}
	else
	{
	    // Is the stream running too slow?
	    if ((vgm_cur_addr - addr) <= sample_speed * 5)
	    {
		// If so, then add more data
		if ((vgm_cur_addr + sample_speed * 4) >= vgm_end_addr)
		{
		    flush_vgm();
		}
		else
		{
		    auto begin = (vgm_data.begin() + (vgm_cur_addr - vgm_base_addr));
		    auto end = (begin + (sample_speed * 4));
		    copy(begin, end, (rf5c68_ram.begin() + vgm_cur_addr));
		    vgm_cur_addr += (sample_speed * 4);
		}
	    }
	}
    }

    void RF5C68::flush_vgm()
    {
	if (!is_vgm_hack)
	{
	    return;
	}

	if (vgm_cur_addr >= vgm_end_addr)
	{
	    return;
	}

	auto begin = (vgm_data.begin() + (vgm_cur_addr - vgm_base_addr));
	auto end = (begin + (vgm_end_addr - vgm_cur_addr));

	copy(begin, end, (rf5c68_ram.begin() + vgm_cur_addr));

	vgm_cur_addr = vgm_end_addr;
    }

    void RF5C68::init()
    {
	reset();
    }

    void RF5C68::enable_vgm_hack(bool is_enabled)
    {
	is_vgm_hack = is_enabled;
    }

    void RF5C68::reset()
    {
	rf5c68_ram.fill(0);
	mem_bank = 0;
	ch_bank = 0;
    }

    void RF5C68::writeRAM(int data_start, int data_len, vector<uint8_t> ram_data)
    {
	uint32_t data_offs = (data_start | (mem_bank << 12));
	uint32_t data_length = data_len;
	uint32_t data_end = (data_offs + data_len);

	if (data_offs >= rf5c68_ram.size())
	{
	    return;
	}

	if (data_end >= rf5c68_ram.size())
	{
	    data_length = (rf5c68_ram.size() - data_offs);
	}

	if (is_vgm_hack)
	{
	    flush_vgm();

	    vgm_base_addr = data_offs;
	    vgm_cur_addr = vgm_base_addr;
	    vgm_end_addr = (vgm_base_addr + data_length);
	    vgm_cur_step = 0;
	    vgm_data = ram_data;

	    uint32_t byte_count = 0x40; // Certain VGMs need such a high value

	    if ((vgm_cur_addr + byte_count) > vgm_end_addr)
	    {
		byte_count = (vgm_end_addr - vgm_cur_addr);
	    }

	    auto begin = (vgm_data.begin() + (vgm_cur_addr - vgm_base_addr));
	    auto end = (begin + byte_count);
	    copy(begin, end, (rf5c68_ram.begin() + vgm_cur_addr));
	    vgm_cur_addr += byte_count;
	}
	else
	{
	    copy(ram_data.begin(), (ram_data.begin() + data_length), (rf5c68_ram.begin() + data_offs));
	}
    }

    void RF5C68::writemem(uint16_t addr, uint8_t data)
    {
	uint32_t ram_addr = ((mem_bank << 12) | (addr & 0xFFF));
	rf5c68_ram.at(ram_addr) = data;
    }

    void RF5C68::writereg(uint8_t reg, uint8_t data)
    {
	auto &channel = rf5c68_channels[ch_bank];

	switch (reg)
	{
	    case 0x00:
	    {
		channel.envelope = data;
	    }
	    break;
	    case 0x01:
	    {
		channel.pan = data;
	    }
	    break;
	    case 0x02:
	    {
		channel.step = ((channel.step & 0xFF00) | data);
	    }
	    break;
	    case 0x03:
	    {
		channel.step = ((channel.step & 0xFF) | (data << 8));
	    }
	    break;
	    case 0x04:
	    {
		channel.loop_start = ((channel.loop_start & 0xFF00) | data);
	    }
	    break;
	    case 0x05:
	    {
		channel.loop_start = ((channel.loop_start & 0xFF) | (data << 8));
	    }
	    break;
	    case 0x06:
	    {
		channel.start_byte = data;

		if (!channel.is_enable)
		{
		    channel.current_addr = (channel.start_byte << (8 + 11));
		}
	    }
	    break;
	    case 0x07:
	    {
		rf5c68_enable = testbit(data, 7);

		if (testbit(data, 6))
		{
		    ch_bank = (data & 0x7);
		}
		else
		{
		    mem_bank = (data & 0xF);
		}
	    }
	    break;
	    case 0x08:
	    {
		for (int i = 0; i < 8; i++)
		{
		    auto &chan_sel = rf5c68_channels[i];
		    chan_sel.is_enable = !testbit(data, i);

		    if (!chan_sel.is_enable)
		    {
			chan_sel.current_addr = (chan_sel.start_byte << (8 + 11));
		    }
		}
	    }
	    break;
	}
    }

    void RF5C68::clockchip()
    {
	if (!rf5c68_enable)
	{
	    return;
	}

	for (auto &channel : rf5c68_channels)
	{
	    if (channel.is_enable)
	    {
		int left_vol = ((channel.pan & 0xF) * channel.envelope);
		int right_vol = ((channel.pan >> 4) * channel.envelope);

		check_vgm_samples(((channel.current_addr >> 11) & 0xFFFF), channel.step);
		uint8_t sample_val = rf5c68_ram.at(((channel.current_addr >> 11) & 0xFFFF));

		if (sample_val == 0xFF)
		{
		    channel.current_addr = (channel.loop_start << 11);
		    sample_val = rf5c68_ram.at(((channel.current_addr >> 11) & 0xFFFF));

		    if (sample_val == 0xFF)
		    {
			continue;
		    }
		}

		channel.current_addr = ((channel.current_addr + channel.step) & 0x7FFFFFF);

		int8_t sample = 0;

		if (testbit(sample_val, 7))
		{
		    sample = (sample_val & 0x7F);
		}
		else
		{
		    sample = -(sample_val & 0x7F);
		}

		channel.output[0] = ((sample * left_vol) >> 5);
		channel.output[1] = ((sample * right_vol) >> 5);
	    }
	}

	if (is_vgm_hack)
	{
	    if (vgm_cur_addr < vgm_end_addr)
	    {
		vgm_cur_step += 0x800;

		if (vgm_cur_step >= 0x800)
		{
		    uint32_t vgm_pos = (vgm_cur_step >> 11);
		    vgm_cur_step &= 0x7FF;

		    if ((vgm_cur_addr + vgm_pos) > vgm_end_addr)
		    {
			vgm_pos = (vgm_end_addr - vgm_cur_addr);
		    }

		    auto begin = (vgm_data.begin() + (vgm_cur_addr - vgm_base_addr));
		    auto end = (begin + vgm_pos);
		    copy(begin, end, (rf5c68_ram.begin() + vgm_cur_addr));
		    vgm_cur_addr += vgm_pos;
		}
	    }
	}
    }

    vector<int32_t> RF5C68::get_samples()
    {
	array<int32_t, 2> mixed_samples = {0, 0};

	for (auto &channel : rf5c68_channels)
	{
	    for (int ch = 0; ch < 2; ch++)
	    {
		int32_t old_sample = mixed_samples[ch];
		int32_t new_sample = clamp<int16_t>(channel.output[ch], -32768, 32767);

		int32_t result = (old_sample + new_sample);
		mixed_samples[ch] = result;
	    }
	}

	// Output is only 10 bits on the RF5C68,
	// but 16 bits on the RF5C164
	if (true)
	// if (chip_type == RF5CType::RF5C68_Chip)
	{
	    mixed_samples[0] &= ~0x3F;
	    mixed_samples[1] &= ~0x3F;
	}

	vector<int32_t> final_samples;
	final_samples.push_back(mixed_samples[0]);
	final_samples.push_back(mixed_samples[1]);
	return final_samples;
    }
};