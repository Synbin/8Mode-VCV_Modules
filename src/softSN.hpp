#pragma once

#include "plugin.hpp"

enum ParamIds
{
	m_noise_clock_res,
	m_noise_filter_res,
	m_decay_res,
	m_attack_res,
	m_vco_res,
	m_pitch_voltage,
	m_slf_res,
	M_MIXER_A_PARAM,
	M_MIXER_B_PARAM,
	M_MIXER_C_PARAM,
	M_ENV_KNOB,
	VCO_SELECT_PARAM,
	ONE_SHOT_PARAM,
	ONE_SHOT_CAP_PARAM,
	m_envelope_1,
	m_envelope_2,
	NUM_PARAMS
};
enum InputIds
{
	EXT_VCO,
	SLF_EXT,
	ONE_SHOT_GATE_PARAM,
	ATTACK_MOD_PARAM,
	DECAY_MOD_PARAM,
	NOISE_FREQ_MOD_PARAM,
	NOISE_FILTER_MOD_PARAM,
	ONE_SHOT_LENGTH_MOD_PARAM,
	DUTY_MOD_PARAM,
	NUM_INPUTS
};
enum OutputIds
{
	SINE_OUTPUT,
	TRI_OUTPUT,
	RESOUT,
	CAPOUT,
	NUM_OUTPUTS
};
enum LightIds
{
	PIN1,
	NUM_LIGHTS
};
