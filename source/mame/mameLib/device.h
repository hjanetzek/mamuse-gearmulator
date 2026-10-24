#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "synthLib/device.h"

#include "mame_plugin.h"

namespace mameLib
{
	// A machine emulated by MAME. DeviceCreateParams::romName is the MAME machine name, e.g. "tx81z".
	// The emulation runs on a thread of its own inside MAME, ahead of the host by the extra latency the plugin asks for
	// plus a small margin, so process() never waits for it unless rendering offline
	class Device : public synthLib::Device
	{
	public:
		explicit Device(const synthLib::DeviceCreateParams& _params);
		~Device() override;

		float getSamplerate() const override { return m_samplerate; }
		void getSupportedSamplerates(std::vector<float>& _dst) const override;
		bool setSamplerate(float _samplerate) override;

		bool isValid() const override { return m_instance != nullptr; }

		// the state is the machine's NVRAM, e.g. a synthesizer's patch memory
		bool getState(std::vector<uint8_t>& _state, synthLib::StateType _type) override;
		bool setState(const std::vector<uint8_t>& _state, synthLib::StateType _type) override;

		uint32_t getChannelCountIn() override { return 0; }
		uint32_t getChannelCountOut() override { return 2; }

		uint32_t getInternalLatencyMidiToOutput() const override;

		// offline rendering waits for the emulation and is deterministic
		void setOffline(bool _offline);

		const std::string& getMachine() const { return m_machine; }

		// why the machine didn't start
		const std::string& getError() const { return m_error; }

	protected:
		void readMidiOut(std::vector<synthLib::SMidiEvent>& _midiOut) override;
		void processAudio(const synthLib::TAudioInputs& _inputs, const synthLib::TAudioOutputs& _outputs, size_t _samples) override;
		bool sendMidi(const synthLib::SMidiEvent& _ev, std::vector<synthLib::SMidiEvent>& _response) override;

	private:
		bool create(float _samplerate, const std::vector<uint8_t>& _nvram);
		void destroy();
		std::vector<uint8_t> getNvram() const;

		std::string m_machine;
		std::string m_rompath;
		std::string m_datapath;
		float m_samplerate = 48000.0f;
		bool m_offline = false;
		mp_instance* m_instance = nullptr;
		std::string m_error;
		uint32_t m_latency = 0;		// what the instance was last set to

		// MIDI received for the current block; SysEx is kept in m_midiIn
		std::vector<synthLib::SMidiEvent> m_midiIn;
		std::vector<mp_midi_event> m_events;

		// MIDI the machine sent
		static constexpr uint32_t MaxOutEvents = 1024;
		std::array<mp_midi_event, MaxOutEvents> m_outEvents;
		std::vector<uint8_t> m_outBuffer;
	};
}
