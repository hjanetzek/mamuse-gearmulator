#include "device.h"

#include <algorithm>
#include <cstring>

#include "panel.h"
#include "settings.h"

#include "synthLib/deviceException.h"

namespace mameLib
{
	namespace
	{
		// rates the machine can run at directly, others go through the plugin's resampler
		constexpr float g_samplerates[] = { 44100.0f, 48000.0f, 88200.0f, 96000.0f, 176400.0f, 192000.0f };

		// look-ahead on top of the plugin's extra latency: MAME delivers audio in 1 ms steps, plus scheduling slack
		uint32_t latencyMargin(const float _samplerate)
		{
			return static_cast<uint32_t>(_samplerate * 3.0f / 1000.0f);
		}

		uint32_t midiMessageLength(const uint8_t _status)
		{
			switch (_status & 0xf0)
			{
			case 0x80: case 0x90: case 0xa0: case 0xb0: case 0xe0:
				return 3;
			case 0xc0: case 0xd0:
				return 2;
			case 0xf0:
				switch (_status)
				{
				case 0xf1: case 0xf3: return 2;
				case 0xf2: return 3;
				case 0xf0: case 0xf7: return 0;
				default: return 1;
				}
			default:
				return 0;
			}
		}

		// state: "MAME", version, machine name, NVRAM
		constexpr char g_stateMagic[4] = { 'M', 'A', 'M', 'E' };
		constexpr uint8_t g_stateVersion = 1;
	}

	Device::Device(const synthLib::DeviceCreateParams& _params, std::shared_ptr<Panel> _panel)
		: synthLib::Device(_params)
		, m_machine(_params.romName)
		, m_panel(std::move(_panel))
	{
		const auto settings = Settings::load();

		// ROMs may also be put into the plugin's data folder
		m_rompath = settings.rompath;
		if (!_params.homePath.empty())
			m_rompath += ";" + _params.homePath + "/roms";
		m_datapath = settings.datapath + "/" + m_machine;

		const auto host = _params.hostSamplerate;
		const bool hostSupported = std::find(std::begin(g_samplerates), std::end(g_samplerates), host) != std::end(g_samplerates);

		m_outBuffer.resize(65536);

		if (!create(hostSupported ? host : 48000.0f, {}))
			throw synthLib::DeviceException(synthLib::DeviceError::FirmwareMissing, m_error);
	}

	Device::~Device()
	{
		destroy();
	}

	void Device::getSupportedSamplerates(std::vector<float>& _dst) const
	{
		_dst.insert(_dst.end(), std::begin(g_samplerates), std::end(g_samplerates));
	}

	bool Device::setSamplerate(const float _samplerate)
	{
		if (_samplerate == m_samplerate && m_instance)
			return true;
		if (std::find(std::begin(g_samplerates), std::end(g_samplerates), _samplerate) == std::end(g_samplerates))
			return false;

		// the rate is fixed when MAME starts the machine, so restart it, keeping its NVRAM
		const auto nvram = getNvram();
		destroy();
		return create(_samplerate, nvram);
	}

	bool Device::getState(std::vector<uint8_t>& _state, const synthLib::StateType _type)
	{
		if (_type != synthLib::StateTypeGlobal || !m_instance)
			return false;

		const auto nvram = getNvram();

		_state.insert(_state.end(), std::begin(g_stateMagic), std::end(g_stateMagic));
		_state.push_back(g_stateVersion);
		_state.push_back(static_cast<uint8_t>(m_machine.size()));
		_state.insert(_state.end(), m_machine.begin(), m_machine.end());
		_state.insert(_state.end(), nvram.begin(), nvram.end());
		return true;
	}

	bool Device::setState(const std::vector<uint8_t>& _state, const synthLib::StateType _type)
	{
		if (_type != synthLib::StateTypeGlobal)
			return false;

		constexpr size_t headerSize = sizeof(g_stateMagic) + 2;
		if (_state.size() < headerSize || std::memcmp(_state.data(), g_stateMagic, sizeof(g_stateMagic)) != 0 || _state[4] != g_stateVersion)
			return false;

		const size_t nameSize = _state[5];
		if (_state.size() < headerSize + nameSize)
			return false;
		if (std::string(_state.begin() + headerSize, _state.begin() + static_cast<ptrdiff_t>(headerSize + nameSize)) != m_machine)
			return false;

		// NVRAM can only be applied when MAME starts the machine
		const std::vector<uint8_t> nvram(_state.begin() + static_cast<ptrdiff_t>(headerSize + nameSize), _state.end());
		destroy();
		return create(m_samplerate, nvram);
	}

	uint32_t Device::getInternalLatencyMidiToOutput() const
	{
		return latencyMargin(m_samplerate);
	}

	void Device::setOffline(const bool _offline)
	{
		m_offline = _offline;
		if (m_instance)
			mp_set_offline(m_instance, _offline ? 1 : 0);
	}

	void Device::readMidiOut(std::vector<synthLib::SMidiEvent>& _midiOut)
	{
		if (!m_instance)
			return;

		const auto count = mp_midi_output(m_instance, m_outEvents.data(), MaxOutEvents, m_outBuffer.data(), m_outBuffer.size());

		for (uint32_t i = 0; i < count; ++i)
		{
			const auto& e = m_outEvents[i];
			auto& ev = _midiOut.emplace_back(synthLib::MidiEventSource::Device);
			ev.offset = e.frame;
			if (e.size > 3 || e.data[0] == 0xf0)
			{
				ev.sysex.assign(e.data, e.data + e.size);
			}
			else
			{
				ev.a = e.data[0];
				ev.b = e.size > 1 ? e.data[1] : 0;
				ev.c = e.size > 2 ? e.data[2] : 0;
			}
		}
	}

	void Device::processAudio(const synthLib::TAudioInputs& _inputs, const synthLib::TAudioOutputs& _outputs, const size_t _samples)
	{
		if (!m_instance)
		{
			std::fill_n(_outputs[0], _samples, 0.0f);
			std::fill_n(_outputs[1], _samples, 0.0f);
			m_midiIn.clear();
			return;
		}

		// the plugin's extra latency follows the host's block size
		const auto latency = getExtraLatencySamples() + latencyMargin(m_samplerate);
		if (latency != m_latency)
			m_latency = mp_set_latency(m_instance, latency);

		// the events point into m_midiIn, which stays unchanged until mp_process returns
		m_events.clear();
		for (const auto& ev : m_midiIn)
		{
			mp_midi_event& e = m_events.emplace_back();
			e.frame = ev.offset;
			if (!ev.sysex.empty())
			{
				e.data = ev.sysex.data();
				e.size = static_cast<uint32_t>(ev.sysex.size());
			}
			else
			{
				e.data = &ev.a;
				e.size = midiMessageLength(ev.a);
				if (!e.size)
					m_events.pop_back();
			}
		}

		float* const out[2] = { _outputs[0], _outputs[1] };
		mp_process(m_instance, m_events.data(), static_cast<uint32_t>(m_events.size()), out, static_cast<uint32_t>(_samples));
		m_midiIn.clear();
	}

	bool Device::sendMidi(const synthLib::SMidiEvent& _ev, std::vector<synthLib::SMidiEvent>& _response)
	{
		m_midiIn.push_back(_ev);
		return true;
	}

	bool Device::create(const float _samplerate, const std::vector<uint8_t>& _nvram)
	{
		m_samplerate = _samplerate;
		m_latency = getExtraLatencySamples() + latencyMargin(_samplerate);

		mp_config config{};
		config.driver = m_machine.c_str();
		config.rompath = m_rompath.c_str();
		config.datapath = m_datapath.c_str();
		config.sample_rate = static_cast<uint32_t>(_samplerate);
		config.latency_frames = m_latency;
		if (!_nvram.empty())
		{
			config.nvram = _nvram.data();
			config.nvram_size = _nvram.size();
		}

		char error[2048];
		m_instance = mp_create(&config, error, sizeof(error));
		if (!m_instance)
		{
			m_error = "MAME: " + m_machine + " failed to start:\n" + error;
			return false;
		}

		mp_set_offline(m_instance, m_offline ? 1 : 0);
		if (m_panel)
			m_panel->attach(m_instance);
		return true;
	}

	void Device::destroy()
	{
		if (!m_instance)
			return;
		if (m_panel)
			m_panel->detach();
		mp_destroy(m_instance);
		m_instance = nullptr;
	}

	std::vector<uint8_t> Device::getNvram() const
	{
		if (!m_instance)
			return {};
		size_t size = 0;
		const auto* data = static_cast<const uint8_t*>(mp_nvram(m_instance, &size));
		return data ? std::vector<uint8_t>(data, data + size) : std::vector<uint8_t>();
	}
}
