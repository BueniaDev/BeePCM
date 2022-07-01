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
// Though this core is slowly approaching completion, the following features are still
// completely unimplemented:
//
// LFO/Vibrato/Tremolo
//
// In addition, the 12-bit linear PCM sound generation has not been thoroughly tested as of yet.
// However, work will be done on all of those fronts, so don't lose hope here!

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
		float pan_left = 0.0f;
		float pan_right = 0.0f;

		if (pan == 8)
		{
		    pan_left = 0.0f;
		    pan_right = 0.0f;
		}
		else if (pan == 0)
		{
		    pan_left = 1.0f;
		    pan_right = 1.0f;
		}
		else if (testbit(pan, 3))
		{
		    pan_left = 1.0f;
		    int32_t inverted_pan = (0x10 - pan);
		    float pan_vol_db = (float(inverted_pan) * -12.f / 4.f);

		    pan_right = pow(10.f, (pan_vol_db / 20.f));

		    if ((inverted_pan & 0x7) == 7)
		    {
			pan_right = 0;
		    }
		}
		else
		{
		    pan_right = 1.0f;
		    float pan_vol_db = (float(pan) * -12.f / 4.f);

		    pan_left = pow(10.f, (pan_vol_db / 20.f));

		    if ((pan & 0x7) == 0x7)
		    {
			pan_left = 0;
		    }
		}

		int pan_table_offs = ((pan << 7) | level);
		m_pan_table[0][pan_table_offs] = int32_t(float(1 << 12) * (pan_left * total_level));
		m_pan_table[1][pan_table_offs] = int32_t(float(1 << 12) * (pan_right * total_level));
	    }
	}

	float numerator_tll = float(0x80 << 12);
	tll_steps[0] = (-numerator_tll / (78.2f * 44100.f / 1000.f)); // Lower TLL
	tll_steps[1] = (numerator_tll / (78.2f * 2 * 44100.f / 1000.f)); // Raise TLL

	array<double, 64> base_times = 
	{
	    0,       0,       0,       0,
	    6222.95, 4978.37, 4148.66, 3556.01,
	    3111.47, 2489.21, 2074.33, 1778.00,
	    1555.74, 1244.63, 1037.19, 889.02,
	    777.87,  622.31,  518.59,  444.54,
	    388.93,  311.16,  259.32,  222.27,
	    194.47,  155.60,  129.66,  111.16,
	    97.23,   77.82,   64.85,   55.60,
	    48.62,   38.91,   32.43,   27.80,
	    24.31,   19.46,   16.24,   13.92,
	    12.15,   9.75,    8.12,    6.98,
	    6.08,    4.90,    4.08,    3.49,
	    3.04,    2.49,    2.13,    1.90,
	    1.72,    1.41,    1.18,    1.04,
	    0.91,    0.73,    0.59,    0.50,
	    0.45,    0.45,    0.45,    0.45
	};

	for (int i = 4 ; i < 0x40; i++)
	{
	    attack_steps[i] = float(0x400 << 16) / float(base_times[i] * 44100.f / 1000.f);
	    decay_steps[i] = float(0x400 << 16) / float(base_times[i] * 14.32833 * 44100.f / 1000.f);
	}

	for (int i = 0; i < 4; i++)
	{
	    attack_steps[i] = 0;
	    decay_steps[i] = 0;
	}

	attack_steps[0x3F] = (0x400 << 16);

	for (int i = 0; i < 0x400; i++)
	{
	    float db = -(96.f - (96.f * float(i) / float(0x400)));
	    float exp_volume = powf(10.f, (db / 20.f));
	    volume_table[i] = uint32_t(float(1 << 12) * exp_volume);
	}
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

	channel.metadata.attack_rate = (read_rom(address + 8) >> 4);
	channel.metadata.decay_rate = (read_rom(address + 8) & 0xF);
	channel.metadata.decay2_rate = (read_rom(address + 9) & 0xF);
	channel.metadata.decay_level = (read_rom(address + 9) >> 4);
	channel.metadata.release_rate = (read_rom(address + 10) & 0xF);
	channel.metadata.key_rate_scale = (read_rom(address + 10) >> 4);
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

	calc_env_rate(channel);
	channel.env_state = multipcm_env_state::Attack;
	channel.env_volume = 0;
    }

    void MultiPCM::env_update(multipcm_channel &channel)
    {
	switch (channel.env_state)
	{
	    case multipcm_env_state::Attack:
	    {
		channel.env_volume += channel.attack_rate;

		if (channel.env_volume >= (0x3FF << 16))
		{
		    channel.env_state = multipcm_env_state::Decay1;

		    if (channel.decay_rate >= (0x400 << 16))
		    {
			channel.env_state = multipcm_env_state::Decay2;
		    }

		    channel.env_volume = (0x3FF << 16);
		}

		channel.final_volume = volume_table[channel.env_volume >> 16];
	    }
	    break;
	    case multipcm_env_state::Decay1:
	    {
		channel.env_volume -= channel.decay_rate;

		if (channel.env_volume <= 0)
		{
		    channel.env_volume = 0;
		}

		if ((channel.env_volume >> 16) <= (channel.decay_level << 6))
		{
		    channel.env_state = multipcm_env_state::Decay2;
		}

		channel.final_volume = volume_table[channel.env_volume >> 16];
	    }
	    break;
	    case multipcm_env_state::Decay2:
	    {
		channel.env_volume -= channel.decay2_rate;

		if (channel.env_volume <= 0)
		{
		    channel.env_volume = 0;
		}

		channel.final_volume = volume_table[channel.env_volume >> 16];
	    }
	    break;
	    case multipcm_env_state::Release:
	    {
		channel.env_volume -= channel.release_rate;

		if (channel.env_volume <= 0)
		{
		    channel.env_volume = 0;
		    channel.is_playing = false;
		}

		channel.final_volume = volume_table[channel.env_volume >> 16];
	    }
	    break;
	    default:
	    {
		channel.final_volume = (1 << 12);
	    }
	    break;
	}
    }

    void MultiPCM::calc_env_rate(multipcm_channel &channel)
    {
	int32_t octave = channel.octave;

	if (testbit(octave, 3))
	{
	    octave -= 16;
	}

	int32_t rate = 0;

	if (channel.metadata.key_rate_scale != 0xF)
	{
	    rate = (((octave + channel.metadata.key_rate_scale) * 2 + (testbit(channel.octave, 3))));
	}

	auto get_rate = [&](array<uint32_t, 0x40> steps, uint32_t rate, uint32_t val) -> uint32_t
	{
	    switch (val)
	    {
		case 0x0: return steps[0]; break;
		case 0xF: return steps[0x3F]; break;
		default:
		{
		    int p_rate = (4 * val) + rate;
		    int env_rate = min(0x3F, p_rate);
		    return steps[env_rate];
		}
		break;
	    }
	};

	channel.attack_rate = get_rate(attack_steps, rate, channel.metadata.attack_rate);
	channel.decay_rate = get_rate(decay_steps, rate, channel.metadata.decay_rate);
	channel.decay2_rate = get_rate(decay_steps, rate, channel.metadata.decay2_rate);
	channel.release_rate = get_rate(decay_steps, rate, channel.metadata.release_rate);
	channel.decay_level = (0xF - channel.metadata.decay_level);
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
		channel.panpot = (data >> 4);
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
			if (channel.metadata.release_rate != 0xF)
			{
			    channel.env_state = multipcm_env_state::Release;
			}
			else
			{
			    channel.is_playing = false;
			}
		    }
		}
	    }
	    break;
	    case 5:
	    {
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

	for (auto &channel : channels)
	{
	    channel.env_volume = 0;
	    channel.env_state = multipcm_env_state::Attack;
	    channel.is_playing = false;
	}
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
	for (int i = 0; i < 28; i++)
	{
	    auto &channel = channels[i];
	    if (channel.is_playing)
	    {
		uint32_t volume = ((channel.tll_val >> 12) | (channel.panpot << 7));
		uint32_t spos = (channel.offset >> 12);
		uint32_t step = channel.step;
		int32_t csample = 0;
		int32_t fpart = (channel.offset & 0xFFF);

		if (testbit(channel.format, 3))
		{
		    uint32_t addr = (channel.base_addr + (spos >> 2) * 6);

		    switch (spos & 3)
		    {
			case 0:
			{
			    // ab.c .... ....
			    int16_t w0 = ((read_rom(addr) << 8) | ((read_rom(addr + 1) & 0xF) << 4));
			    csample = w0;
			}
			break;
			case 1:
			{
			    // ..C. AB.. ....
			    int16_t w0 = ((read_rom(addr + 2) << 8) | (read_rom(addr + 1) & 0xF0));
			    csample = w0;
			}
			break;
			case 2:
			{
			    // .... ..ab .c..
			    int16_t w0 = ((read_rom(addr + 3) << 8) | ((read_rom(addr + 4) & 0xF) << 4));
			    csample = w0;
			}
			break;
			case 3:
			{
			    // .... .... C.AB
			    int16_t w0 = ((read_rom(addr + 5) << 8) | (read_rom(addr + 4) & 0xF0));
			    csample = w0;
			}
			break;
		    }
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

		env_update(channel);
		sample = ((sample * channel.final_volume) >> 10);

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