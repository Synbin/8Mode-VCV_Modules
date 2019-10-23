#include "plugin.hpp"
#include "widgets.hpp"

using namespace widgets;

Knob16::Knob16()
{
	setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/8Mode_Knob1.svg")));
	shadow->box.pos = Vec(0.0, 0);
}

SliderSwitch2State::SliderSwitch2State()
{
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/8Mode_ss_0.svg")));
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/8Mode_ss_1.svg")));
}

Button18::Button18()
{
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/button_18px_0.svg")));
	addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/button_18px_1.svg")));
	momentary = true;
}

Snap_8M_Knob::Snap_8M_Knob()
{
	setSVG(APP->window->loadSvg(asset::plugin(pluginInstance, "res/8Mode_Knob1.svg")));
	shadow->box.pos = Vec(0.0, 0);
	snap = true;
	minAngle = 0.3*M_PI;
	maxAngle = 0.725*M_PI;

}
