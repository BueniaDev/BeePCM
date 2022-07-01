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

// BeePCM-OKIM6295
// Chip Name: OKIM6295
// Chip Used In: Capcom CPS1 hardware (mainly), and possibly others
//
// Interesting Trivia:
//
// This chip formed one-half of the iconic sound chip duo that provided the sound for
// some of Capcom's most legendary titles, including Street Fighter II, and countless others!
//
//
// BueniaDev's Notes:
// 
// Although basic M6295 support is fully implemented,
// the following features are still completely unimplemented:
// Banking support
// Chip reads
// NMK112 support
//
// However, work is being done on all of those fronts, so don't lose hope here!

#include "okim6295.h"
using namespace beepcm;

namespace beepcm
{
    OkiM6295::OkiM6295()
    {

    }

    OkiM6295::~OkiM6295()
    {

    }

    uint8_t OkiM6295::readROM(uint32_t addr)
    {
	// TODO: Implement ROM banking and NMK112 support
	uint32_t rom_addr = 0;
	rom_addr = addr;

	return (rom_addr < m6295_rom.size()) ? m6295_rom.at(rom_addr) : 0;
    }

    void OkiM6295::generateSample(okim_voice &voice)
    {
	if (voice.current_addr == voice.end_addr)
	{
	    voice.is_playing = false;
	    return;
	}

	if (voice.is_high_nibble)
	{
	    voice.current_byte = readROM(voice.current_addr++);
	}

	uint8_t data = 0;

	if (voice.is_high_nibble)
	{
	    data = (voice.current_byte >> 4);
	}
	else
	{
	    data = (voice.current_byte & 0xF);
	}

	voice.is_high_nibble = !voice.is_high_nibble;

	uint16_t step_size = step_table.at(voice.current_step);

	int16_t delta = (step_size >> 3);

	if (testbit(data, 0))
	{
	    delta += (step_size >> 2);
	}

	if (testbit(data, 1))
	{
	    delta += (step_size >> 1);
	}

	if (testbit(data, 2))
	{
	    delta += step_size;
	}

	if (testbit(data, 3))
	{
	    delta = -delta;
	}

	int32_t sample = (voice.current_signal + delta);

	voice.current_signal = clamp(sample, -2048, 2047);

	int8_t adjusted_step = (voice.current_step + adjust_table.at(data & 0x7));

	voice.current_step = clamp<int8_t>(adjusted_step, 0, 48);

	voice.output = ((voice.current_signal * voice.volume) / 2);
    }

    uint32_t OkiM6295::get_sample_rate(uint32_t clock_rate)
    {
	int divisor = (is_pin7_set) ? 132 : 165;
	return (clock_rate / divisor);
    }

    void OkiM6295::init()
    {
	reset();
    }

    void OkiM6295::reset()
    {
	m6295_cmd = -1;

	for (int i = 0; i < 4; i++)
	{
	    auto &voice = voices[i];
	    voice.number = i;
	    voice.is_playing = false;
	    voice.current_addr = 0;
	    voice.sample_num = 0;
	    voice.num_samples = 0;
	}
    }

    void OkiM6295::writeCmd(uint8_t data)
    {
	if (m6295_cmd != -1)
	{
	    int voice_mask = ((data >> 4) & 0xF);

	    for (int i = 0; i < 4; i++)
	    {
		if (testbit(voice_mask, i))
		{
		    auto &voice = voices[i];

		    if (!voice.is_playing)
		    {
			uint32_t start_addr = 0;
			uint32_t stop_addr = 0;
			uint32_t base = (m6295_cmd * 8);

			start_addr = (readROM(base) << 16);
			start_addr |= (readROM(base + 1) << 8);
			start_addr |= readROM(base + 2);
			start_addr &= 0x3FFFF;

			stop_addr = (readROM(base + 3) << 16);
			stop_addr |= (readROM(base + 4) << 8);
			stop_addr |= readROM(base + 5);
			stop_addr &= 0x3FFFF;

			if (start_addr < stop_addr)
			{
			    voice.is_playing = true;
			    voice.current_addr = start_addr;
			    voice.start_addr = start_addr;
			    voice.end_addr = stop_addr;
			    voice.volume = vol_table.at(data & 0xF);
			    voice.current_byte = 0;
			    voice.is_high_nibble = true;
			    voice.current_step = 0;
			    voice.current_signal = 0;
			}
			else
			{
			    voice.is_playing = false;
			}
		    }
		}
	    }

	    m6295_cmd = -1;
	}
	else if (testbit(data, 7))
	{
	    m6295_cmd = (data & 0x7F);
	}
	else
	{
	    int voice_mask = ((data >> 3) & 0xF);

	    for (int i = 0; i < 4; i++)
	    {
		if (testbit(voice_mask, i))
		{
		    auto &voice = voices[i];
		    voice.is_playing = false;
		}
	    }
	}
    }

    bool OkiM6295::setPin7(bool is_set)
    {
	string pin7_line = (is_set) ? "Asserting" : "Clearing";
	cout << pin7_line << " OKIM6295 pin 7" << endl;
	bool has_changed = (is_pin7_set != is_set);
	is_pin7_set = is_set;
	return has_changed;
    }

    bool OkiM6295::writeVGM(uint8_t offs, uint8_t data)
    {
	switch (offs)
	{
	    case 0x00:
	    {
		writeCmd(data);
		return false;
	    }
	    break;
	    case 0x0C:
	    {
		return setPin7(data != 0);
	    }
	    break;
	    default:
	    {
		cout << "Writing value of " << hex << int(data) << " to OKIM6295 port of " << hex << int(offs) << endl;
	    }
	    break;
	}

	return false;
    }

    void OkiM6295::writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data)
    {
	m6295_rom.resize(rom_size, 0xFF);

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

	copy(rom_data.begin(), (rom_data.begin() + data_length), (m6295_rom.begin() + data_start));
    }

    void OkiM6295::clockchip()
    {
	for (auto &voice : voices)
	{
	    if (voice.is_playing)
	    {
		generateSample(voice);
	    }
	}
    }

    vector<int32_t> OkiM6295::get_samples()
    {
	int32_t sample = 0;

	for (auto &voice : voices)
	{
	    sample += voice.output;
	}

	vector<int32_t> samples;
	samples.push_back(sample);
	return samples;
    }
}