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

// BeePCM-YMZ280B
// Chip Name: YMZ280B
// Chip Used In: Cave68k and various (Data East and Psikyo) arcade machines
//
// Interesting Trivia:
//
// The YMZ280B chip was used most famously by Cave, a Japanese video game company formed
// by a group of ex-Toaplan developers after that company went bankrupt in the
// mid-90's. The company, which became famous for their legendary "bullet hell"
// shoot-em-up games, mostly used this chip as the main sound chip in their
// Cave68k arcade hardware. 
// 
//
// BueniaDev's Notes:
//
// Though this core is slowly approaching completion, the following features are still
// completely unimplemented:
//
// IRQ generation
// External memory handlers
//
// In addition, the 16-bit PCM sound generation has not been thoroughly tested as of yet.
// However, work will be done on all of those fronts, so don't lose hope here!


#include "ymz280b.h"
using namespace beepcm;

namespace beepcm
{
    YMZ280B::YMZ280B()
    {

    }

    YMZ280B::~YMZ280B()
    {

    }

    void YMZ280B::update_step(ymcreative_voice &voice)
    {
	int freq_num = 0;

	if (voice.mode == 1)
	{
	    freq_num = (voice.freq_num & 0xFF);
	}
	else
	{
	    freq_num = (voice.freq_num & 0x1FF);
	}

	voice.output_step = (freq_num + 1);
    }

    void YMZ280B::update_volumes(ymcreative_voice &voice)
    {
	if (voice.pan == 8)
	{
	    voice.output_left = voice.level;
	    voice.output_right = voice.level;
	}
	else if (voice.pan < 8)
	{
	    voice.output_left = voice.level;
	    voice.output_right = (voice.pan == 0) ? 0 : voice.level * (voice.pan - 1) / 7;
	}
	else
	{
	    voice.output_left = voice.level * (15 - voice.pan) / 7;
	    voice.output_right = voice.level;
	}
    }

    void YMZ280B::key_on(ymcreative_voice &voice)
    {
	voice.is_playing = true;
	voice.current_addr = voice.start_addr;
	voice.is_high_nibble = false;
	voice.current_byte = 0;
	voice.output_pos = 0;
	voice.prev_signal = 0;
	voice.voice_signal = 0;
	voice.voice_step = 127;
	voice.adpcm_loop = false;
    }

    void YMZ280B::key_off(ymcreative_voice &voice)
    {
	voice.is_playing = false;
    }

    uint8_t YMZ280B::fetch_rom(uint32_t addr)
    {
	addr &= 0xFFFFFF;
	return (addr < ymz280b_rom.size()) ? ymz280b_rom.at(addr) : 0;
    }

    void YMZ280B::generate_adpcm_sample(ymcreative_voice &voice)
    {
	if (!voice.is_playing)
	{
	    return;
	}

	array<int, 8> adpcm_step_scale =
	{
	    230, 230, 230, 230,
	    307, 409, 512, 614
	};

	if (voice.is_looping)
	{
	    if ((voice.current_addr == voice.loop_start_addr) && !voice.adpcm_loop)
	    {
		voice.loop_signal = voice.voice_signal;
		voice.loop_step = voice.voice_step;
	    }

	    if (voice.current_addr == voice.loop_end_addr)
	    {
		if (voice.is_keyon)
		{
		    voice.current_addr = voice.loop_start_addr;
		    voice.voice_signal = voice.loop_signal;
		    voice.voice_step = voice.loop_step;
		    voice.is_high_nibble = false;
		    voice.adpcm_loop = true;
		}
	    }
	}

	if (voice.current_addr == voice.stop_addr)
	{
	    voice.prev_signal = 0;
	    voice.voice_signal = 0;
	    return;
	}

	if (!voice.is_high_nibble)
	{
	    voice.current_byte = fetch_rom(voice.current_addr++);
	    voice.current_addr &= 0xFFFFFF;
	}

	uint8_t data = (uint8_t(voice.current_byte << (4 * voice.is_high_nibble)) >> 4);
	voice.is_high_nibble = !voice.is_high_nibble;

	voice.prev_signal = voice.voice_signal;

	int32_t delta = (2 * (data & 0x7) + 1) * voice.voice_step / 8;

	if (testbit(data, 3))
	{
	    delta = -delta;
	}

	voice.voice_signal = ((voice.voice_signal * 254) / 256);
	voice.voice_signal = clamp((voice.voice_signal + delta), -32768, 32767);

	int step_scale = adpcm_step_scale[(data & 0x7)];

	voice.voice_step = clamp(((voice.voice_step * step_scale) >> 8), 127, 24576);
    }

    void YMZ280B::generate_pcm8(ymcreative_voice &voice)
    {
	if (!voice.is_playing)
	{
	    return;
	}

	if (voice.is_looping)
	{
	    if (voice.current_addr == voice.loop_end_addr)
	    {
		if (voice.is_keyon)
		{
		    voice.current_addr = voice.loop_start_addr;
		}
	    }
	}

	if (voice.current_addr == voice.stop_addr)
	{
	    voice.prev_signal = 0;
	    voice.voice_signal = 0;
	    return;
	}

	voice.current_byte = fetch_rom(voice.current_addr++);

	voice.prev_signal = voice.voice_signal;
	voice.voice_signal = (int8_t(voice.current_byte) << 8);
    }

    void YMZ280B::generate_pcm16(ymcreative_voice &voice)
    {
	if (!voice.is_playing)
	{
	    return;
	}

	if (voice.is_looping)
	{
	    if (voice.current_addr == voice.loop_end_addr)
	    {
		if (voice.is_keyon)
		{
		    voice.current_addr = voice.loop_start_addr;
		}
	    }
	}

	if (voice.current_addr == voice.stop_addr)
	{
	    voice.prev_signal = 0;
	    voice.voice_signal = 0;
	    return;
	}

	uint8_t low_byte = fetch_rom(voice.current_addr++);
	uint8_t high_byte = fetch_rom(voice.current_addr++);

	int16_t result = int16_t((high_byte << 8) | low_byte);
	voice.prev_signal = voice.voice_signal;
	voice.voice_signal = result;
    }

    void YMZ280B::channel_output(ymcreative_voice &voice)
    {
	auto m_prev_accum = voice.prev_signal;
	auto m_position = voice.output_pos;
	auto m_accum = voice.voice_signal;

	int32_t result = (((m_prev_accum * int32_t((m_position ^ 0x1FF) + 1)) + (m_accum * int32_t(m_position))) >> 9);

	voice.output[0] = ((result * int32_t(voice.output_left)) >> 9);
	voice.output[1] = ((result * int32_t(voice.output_right)) >> 9);
    }

    void YMZ280B::writereg(uint8_t reg, uint8_t data)
    {
	if (reg < 0x80)
	{
	    int voice_num = ((reg >> 2) & 0x7);

	    auto &voice = voices[voice_num];

	    switch (reg & 0xE3)
	    {
		case 0x00:
		{
		    voice.freq_num = ((voice.freq_num & 0x100) | data);
		    update_step(voice);
		}
		break;
		case 0x01:
		{
		    voice.freq_num = ((voice.freq_num & 0xFF) | ((data & 0x1) << 8));
		    voice.is_looping = testbit(data, 4);
		    voice.mode = ((data & 0x60) >> 5);

		    bool is_keyon_val = false;

		    if (voice.mode != 0)
		    {
			is_keyon_val = testbit(data, 7);
		    }

		    if (!voice.is_keyon && is_keyon_val && master_keyon)
		    {
			key_on(voice);
		    }
		    else if (voice.is_keyon && !is_keyon_val)
		    {
			key_off(voice);
		    }

		    voice.is_keyon = is_keyon_val;
		    update_step(voice);
		}
		break;
		case 0x02:
		{
		    voice.level = data;
		    update_volumes(voice);
		}
		break;
		case 0x03:
		{
		    voice.pan = (data & 0xF);
		    update_volumes(voice);
		}
		break;
		case 0x20:
		{
		    voice.start_addr = ((voice.start_addr & 0x00FFFF) | (data << 16));
		}
		break;
		case 0x21:
		{
		    voice.loop_start_addr = ((voice.loop_start_addr & 0x00FFFF) | (data << 16));
		}
		break;
		case 0x22:
		{
		    voice.loop_end_addr = ((voice.loop_end_addr & 0x00FFFF) | (data << 16));
		}
		break;
		case 0x23:
		{
		    voice.stop_addr = ((voice.stop_addr & 0x00FFFF) | (data << 16));
		}
		break;
		case 0x40:
		{
		    voice.start_addr = ((voice.start_addr & 0xFF00FF) | (data << 8));
		}
		break;
		case 0x41:
		{
		    voice.loop_start_addr = ((voice.loop_start_addr & 0xFF00FF) | (data << 8));
		}
		break;
		case 0x42:
		{
		    voice.loop_end_addr = ((voice.loop_end_addr & 0xFF00FF) | (data << 8));
		}
		break;
		case 0x43:
		{
		    voice.stop_addr = ((voice.stop_addr & 0xFF00FF) | (data << 8));
		}
		break;
		case 0x60:
		{
		    voice.start_addr = ((voice.start_addr & 0xFFFF00) | data);
		}
		break;
		case 0x61:
		{
		    voice.loop_start_addr = ((voice.loop_start_addr & 0xFFFF00) | data);
		}
		break;
		case 0x62:
		{
		    voice.loop_end_addr = ((voice.loop_end_addr & 0xFFFF00) | data);
		}
		break;
		case 0x63:
		{
		    voice.stop_addr = ((voice.stop_addr & 0xFFFF00) | data);
		}
		break;
		default:
		{
		    if (data != 0)
		    {
			cout << "Unrecognized YMZ280B write of " << hex << int(reg) << " = " << hex << int(data) << endl;
		    }
		}
		break;
	    }
	}
	else
	{
	    switch (reg)
	    {
		// DSP writes are unimplemented
		case 0x80:
		case 0x81:
		case 0x82: break;
		case 0x84:
		{
		    cout << "Writing to ROM readback / RAM write bits 23-16" << endl;
		}
		break;
		case 0x85:
		{
		    cout << "Writing to ROM readback / RAM write bits 15-8" << endl;
		}
		break;
		case 0x86:
		{
		    cout << "Writing to ROM readback / RAM write bits 7-0" << endl;
		}
		break;
		case 0x87:
		{
		    cout << "Writing to ROM readback / RAM write data port" << endl;
		}
		break;
		case 0xFE:
		{
		    cout << "Updating IRQ state" << endl;
		}
		break;
		case 0xFF:
		{
		    cout << "Writing to IRQ enable/memory enable/test register" << endl;

		    if (master_keyon && !testbit(data, 7))
		    {
			for (int i = 0; i < 8; i++)
			{
			    voices[i].is_playing = false;
			}
		    }
		    else if (!master_keyon && testbit(data, 7))
		    {
			for (int i = 0; i < 8; i++)
			{
			    if (voices[i].is_keyon)
			    {
				voices[i].is_playing = true;
			    }
			}
		    }

		    master_keyon = testbit(data, 7);
		}
		break;
		default:
		{
		    if (data != 0)
		    {
			cout << "Unrecognized YMZ280B write of " << hex << int(reg) << " = " << hex << int(data) << endl;
		    }
		}
		break;
	    }
	}
    }

    uint32_t YMZ280B::get_sample_rate(uint32_t clock_rate)
    {
	// Internal clock rate is (clock / 384) * 2
	return (clock_rate / 192.0);
    }

    void YMZ280B::init()
    {
	reset();
    }

    void YMZ280B::reset()
    {
	for (auto &voice : voices)
	{
	    voice.output.fill(0);
	}
    }

    void YMZ280B::writeIO(int port, uint8_t data)
    {
	if ((port & 1) == 0)
	{
	    chip_address = data;
	}
	else
	{
	    writereg(chip_address, data);
	}
    }

    void YMZ280B::writeROM(uint32_t rom_size, uint32_t data_start, uint32_t data_len, vector<uint8_t> rom_data)
    {
	uint32_t vec_size = ymz280b_rom.size();

	if (vec_size != rom_size)
	{
	    ymz280b_rom.resize(rom_size, 0xFF);
	}

	if (data_start > rom_size)
	{
	    return;
	}

	uint32_t data_length = data_len;

	if ((data_start + data_length) > rom_size)
	{
	    data_length = (rom_size - data_start);
	}

	copy(rom_data.begin(), (rom_data.begin() + data_length), (ymz280b_rom.begin() + data_start));
    }

    void YMZ280B::clockchip()
    {
	for (int i = 0; i < 8; i++)
	{
	    auto &voice = voices[i];

	    if (!voice.is_playing)
	    {
		continue;
	    }

	    uint32_t position = (voice.output_pos + voice.output_step);
	    voice.output_pos = (position & 0x1FF);

	    if (position < 0x200)
	    {
		continue;
	    }

	    switch (voice.mode)
	    {
		case 1: generate_adpcm_sample(voice); break;
		case 2: generate_pcm8(voice); break;
		case 3: generate_pcm16(voice); break;
	    }

	    channel_output(voice);
	}
    }

    vector<int32_t> YMZ280B::get_samples()
    {
	vector<int32_t> final_samples;
	array<int32_t, 2> mixed_samples = {0, 0};

	for (auto &voice : voices)
	{
	    for (int i = 0; i < 2; i++)
	    {
		int32_t old_sample = mixed_samples[i];
		int32_t new_sample = clamp<int16_t>(voice.output[i], -32768, 32767);

		int32_t result = (old_sample + new_sample);
		mixed_samples[i] = clamp(result, -32768, 32767);
	    }
	}

	final_samples.push_back(mixed_samples[0]);
	final_samples.push_back(mixed_samples[1]);

	return final_samples;
    }
};