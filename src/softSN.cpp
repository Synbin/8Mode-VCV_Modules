#include "plugin.hpp"
#include "widgets.hpp"
#include "softSN.hpp"
#include "sn76477.h"
#include "rescap.h"
#include "dsp/digital.hpp"

using namespace dsp;
using namespace widgets;

struct SN_VCO : Module
{
	int acc = 0;
	double triout = 0;
	double Power = 0;
	float energy = 0;
	float sample = 0;
	float output_power_normal = 0;
	float K = 0;
	float sampleRate;

	SchmittTrigger OneShotTrigger;
	sn76477_device sn;

	SN_VCO()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(m_noise_clock_res, 10000, 3300000, 0.0);
		configParam(m_noise_filter_res, 1, 100000000, 0.0);
		configParam(m_decay_res, 1, 20000000, 10000000);
		configParam(m_attack_res, 1, 5000000, 10);
		configParam(m_vco_res, 0, 8, 4);
		configParam(m_pitch_voltage, 0, 4.55, 2.30);
		configParam(m_slf_res, 0, 16, 8);
		configParam(M_MIXER_A_PARAM, 0.0, 1.0, 1.0);
		configParam(M_MIXER_B_PARAM, 0.0, 1.0, 0.0);
		configParam(M_MIXER_C_PARAM, 0.0, 1.0, 0.0);
		configParam(M_ENV_KNOB, 0, 3, 0);
		configParam(VCO_SELECT_PARAM, 0.0, 1.0, 0.0);
		configParam(ONE_SHOT_PARAM, 0.0, 1.0, 0.0);
		configParam(ONE_SHOT_CAP_PARAM, 10, 2000, 500);
		configParam(m_envelope_1, 0.0, 0.0, 0.0);
		configParam(m_envelope_2, 0.0, 0.0, 0.0);

		sampleRate = APP->engine->getSampleRate();
		sn.set_amp_res(100);
		sn.set_feedback_res(100);
		sn.set_m_our_sample_rate(sampleRate);
		sn.device_start();
	}

	void process(const ProcessArgs &args) override
	{
		float newSampleRate = args.sampleRate;
		if (newSampleRate != sampleRate)
		{
			sampleRate = newSampleRate;
			sn.set_m_our_sample_rate(sampleRate);
		}

		params[M_MIXER_A_PARAM].value = round(params[M_MIXER_A_PARAM].value);
		params[M_MIXER_B_PARAM].value = round(params[M_MIXER_B_PARAM].value);
		params[M_MIXER_C_PARAM].value = round(params[M_MIXER_C_PARAM].value);
		params[M_ENV_KNOB].value = round(params[M_ENV_KNOB].value);
		params[VCO_SELECT_PARAM].value = round(params[VCO_SELECT_PARAM].value);

		// Calculate VCO and SLF Oscillator Voltages
		double volts = 1.752 * powf(2.0f, -1 * (inputs[EXT_VCO].value + (params[m_vco_res].value - 4 + (params[VCO_SELECT_PARAM].value * 6.223494))));
		double slf_volts = 1.283184 * powf(2.0f, -1 * (inputs[SLF_EXT].value + (params[m_slf_res].value - 8 + (params[VCO_SELECT_PARAM].value * 6.223494))));

		// Applies parameters to SN76447 emulator
		float attack_res = params[m_attack_res].value + (((inputs[ATTACK_MOD_PARAM].value * 20) / 100) * 5000000);
		float decay_res = params[m_decay_res].value + (((inputs[DECAY_MOD_PARAM].value * 20) / 100) * 20000000);
		float noise_clock_res = params[m_noise_clock_res].value + (((inputs[NOISE_FREQ_MOD_PARAM].value * 20) / 100) * 3300000);
		float noise_filter_res = params[m_noise_filter_res].value + (((inputs[NOISE_FILTER_MOD_PARAM].value * 20) / 100) * 100000000);
		float one_shot_length = ((params[ONE_SHOT_CAP_PARAM].value) + (((inputs[ONE_SHOT_LENGTH_MOD_PARAM].value * 20) / 100) * 2000)) / 1000000000;
		float duty_cycle = params[m_pitch_voltage].value + (((inputs[DUTY_MOD_PARAM].value * 20) / 100) * 4.55);

		sn.set_vco_params(2.30, 0, volts);
		sn.set_slf_params(CAP_U(.047), slf_volts);
		sn.set_noise_params(noise_clock_res, noise_filter_res, CAP_P(470));
		sn.set_decay_res(decay_res);
		sn.set_attack_params(0.00000005, attack_res);
		sn.set_pitch_voltage(duty_cycle);
		sn.set_mixer_params(params[M_MIXER_A_PARAM].value, params[M_MIXER_B_PARAM].value, params[M_MIXER_C_PARAM].value);
		sn.set_envelope(params[M_ENV_KNOB].value);
		sn.set_vco_mode(params[VCO_SELECT_PARAM].value);
		sn.set_oneshot_params(one_shot_length, 5000000);

		// One Shot Trigger
		if (params[ONE_SHOT_PARAM].value)
			sn.shot_trigger();
		if (OneShotTrigger.process(inputs[ONE_SHOT_GATE_PARAM].value))
			sn.shot_trigger();

		// Attempt at AGC for TRI output.

		Rsamples sam = sn.sound_stream_update(1);

		double sine = (5.0 * (double)sam.s1 / 25000) + 1.3;
		outputs[SINE_OUTPUT].value = sine;

		triout = sam.s2;

		if (params[VCO_SELECT_PARAM].value)
		{
			sample = (float)triout - 1.5;
		}
		else
		{
			sample = (float)triout;
		}
		energy += sample * sample;
		acc = acc + 1;

		if (acc == 16000)
		{
			output_power_normal = (float)pow((double)10, (double)(-30 / 10));
			//Power = 10 * log10(energy / 16000);
			K = (float)sqrt((output_power_normal * 16000) / energy);
			energy = 0;
			acc = 0;
		}

		//outputs[RESOUT].value = noise_filter_res;
		//outputs[CAPOUT].value = 1;

		if (!params[VCO_SELECT_PARAM].value)
		{
			outputs[TRI_OUTPUT].value = (sample * K * 6000.5) - 190;
		}
		else
		{
			outputs[TRI_OUTPUT].value = sample * K * 100.5;
		}
	}
};

struct MyModuleWidget : ModuleWidget
{
	MyModuleWidget(SN_VCO *module)
	{
		setModule(module);
		auto panel = APP->window->loadSvg(asset::plugin(pluginInstance, "res/SNsoft_Panel.svg"));
		setPanel(panel);

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		auto M_VCO_RES_POSITION = mm2px(Vec(39.488, 20.007));
		auto M_DECAY_RES_POSITION = mm2px(Vec(73.136, 20.008));
		auto M_ATTACK_RES_POSITION = mm2px(Vec(57.452, 20.008));
		auto M_NOISE_FILTER_RES_POSITION = mm2px(Vec(73.176, 42.12));
		auto M_NOISE_CLOCK_RES_POSITION = mm2px(Vec(57.53, 42.12));
		auto M_SLF_RES_POSITION = mm2px(Vec(39.613, 42.214));
		auto M_PITCH_VOLTAGE_POSITION = mm2px(Vec(73.605, 64.111));
		auto ONE_SHOT_CAP_POSITION = mm2px(Vec(55.125, 64.488));
		auto ONE_SHOT_POSITION = mm2px(Vec(41.777, 66.387));
		auto M_ENV_KNOB_POSITION = mm2px(Vec(39.816, 87.654));
		auto VCO_SELECT_POSITION = mm2px(Vec(74.262, 90.294));
		auto M_MIXER_A_POSITION = mm2px(Vec(7.664, 90.451));
		auto M_MIXER_B_POSITION = mm2px(Vec(17.704, 90.451));
		auto M_MIXER_C_POSITION = mm2px(Vec(27.743, 90.451));
		auto EXT_VCO_POSITION = mm2px(Vec(4.035, 40.629));
		auto ATTACK_MOD_POSITION = mm2px(Vec(14.932, 40.629));
		auto DECAY_MOD_POSITION = mm2px(Vec(25.828, 40.629));
		auto SLF_EXT_POSITION = mm2px(Vec(3.935, 54.281));
		auto NOISE_FREQ_MOD_POSITION = mm2px(Vec(14.922, 54.281));
		auto NOISE_FILTER_MOD_POSITION = mm2px(Vec(25.803, 54.281));
		auto ONE_SHOT_GATE_POSITION = mm2px(Vec(3.841, 67.775));
		auto ONE_SHOT_LENGTH_MOD_POSITION = mm2px(Vec(14.993, 67.775));
		auto DUTY_MOD_POSITION = mm2px(Vec(25.885, 67.775));
		auto TRI_OUT_POSITION = mm2px(Vec(57.544, 107.234));
		auto SINE_POSITION = mm2px(Vec(75.766, 107.274));
		auto CAPOUT_POSITION = mm2px(Vec(12.941, 119.732));
		auto RESOUT_POSITION = mm2px(Vec(21.296, 119.732));

		addParam(createParam<Knob16>(M_NOISE_CLOCK_RES_POSITION, module, m_noise_clock_res));
		addParam(createParam<Knob16>(M_NOISE_FILTER_RES_POSITION, module, m_noise_filter_res));
		addParam(createParam<Knob16>(M_DECAY_RES_POSITION, module, m_decay_res));
		addParam(createParam<Knob16>(M_ATTACK_RES_POSITION, module, m_attack_res));
		addParam(createParam<Knob16>(M_VCO_RES_POSITION, module, m_vco_res));
		addParam(createParam<Knob16>(M_SLF_RES_POSITION, module, m_slf_res));
		addParam(createParam<SliderSwitch2State>(M_MIXER_A_POSITION, module, M_MIXER_A_PARAM));
		addParam(createParam<SliderSwitch2State>(M_MIXER_B_POSITION, module, M_MIXER_B_PARAM));
		addParam(createParam<SliderSwitch2State>(M_MIXER_C_POSITION, module, M_MIXER_C_PARAM));
		addParam(createParam<SliderSwitch2State>(VCO_SELECT_POSITION, module, VCO_SELECT_PARAM));
		addParam(createParam<Snap_8M_Knob>(M_ENV_KNOB_POSITION, module, M_ENV_KNOB));
		addParam(createParam<CKD6>(ONE_SHOT_POSITION, module, ONE_SHOT_PARAM));
		addParam(createParam<Knob16>(ONE_SHOT_CAP_POSITION, module, ONE_SHOT_CAP_PARAM));
		addParam(createParam<Knob16>(M_PITCH_VOLTAGE_POSITION, module, m_pitch_voltage));

		addInput(createInput<PJ301MPort>(SLF_EXT_POSITION, module, SLF_EXT));
		addInput(createInput<PJ301MPort>(EXT_VCO_POSITION, module, EXT_VCO));
		addInput(createInput<PJ301MPort>(ONE_SHOT_GATE_POSITION, module, ONE_SHOT_GATE_PARAM));
		addInput(createInput<PJ301MPort>(ATTACK_MOD_POSITION, module, ATTACK_MOD_PARAM));
		addInput(createInput<PJ301MPort>(DECAY_MOD_POSITION, module, DECAY_MOD_PARAM));
		addInput(createInput<PJ301MPort>(NOISE_FREQ_MOD_POSITION, module, NOISE_FREQ_MOD_PARAM));
		addInput(createInput<PJ301MPort>(NOISE_FILTER_MOD_POSITION, module, NOISE_FILTER_MOD_PARAM));
		addInput(createInput<PJ301MPort>(ONE_SHOT_LENGTH_MOD_POSITION, module, ONE_SHOT_LENGTH_MOD_PARAM));
		addInput(createInput<PJ301MPort>(DUTY_MOD_POSITION, module, DUTY_MOD_PARAM));

		addOutput(createOutput<PJ301MPort>(SINE_POSITION, module, SINE_OUTPUT));
		addOutput(createOutput<PJ301MPort>(TRI_OUT_POSITION, module, TRI_OUTPUT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelsoftSN = createModel<SN_VCO, MyModuleWidget>("softSN");
