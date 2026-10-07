/* capture.cpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "capture.hpp"

#include <wayfire/core.hpp>
#include <wayfire/output.hpp>
#include <wayfire/seat.hpp>

namespace unity_spatial_gestures
{
namespace
{
constexpr auto TAKEN = wf::input_event_processing_mode_t::IGNORE;

bool inhibited()
{
    auto *output = wf::get_core().seat->get_active_output();
    return !output || output->is_inhibited();
}
}

capture::capture(protocol& sink) : wayland(sink)
{
    on_swipe_begin  = [this] (auto *ev) { begin(swipe, ev); };
    on_swipe_update = [this] (auto *ev) { update(swipe, ev, 1.0, 0.0); };
    on_swipe_end    = [this] (auto *ev) { end(swipe, ev); };
    on_pinch_begin  = [this] (auto *ev) { begin(pinch, ev); };
    on_pinch_update = [this] (auto *ev) { update(pinch, ev, ev->event->scale, ev->event->rotation); };
    on_pinch_end    = [this] (auto *ev) { end(pinch, ev); };

    wf::get_core().connect(&on_swipe_begin);
    wf::get_core().connect(&on_swipe_update);
    wf::get_core().connect(&on_swipe_end);
    wf::get_core().connect(&on_pinch_begin);
    wf::get_core().connect(&on_pinch_update);
    wf::get_core().connect(&on_pinch_end);
}

template<class event_t>
void capture::begin(tracked& state, signal<event_t> *ev)
{
    if (inhibited() || !wayland.wants(state.gesture, ev->event->fingers))
    {
        state.fingers = 0;
        return;
    }

    state.fingers = ev->event->fingers;
    ev->mode = TAKEN;
    wayland.begin(state.gesture, state.fingers, ev->event->time_msec);
}

template<class event_t>
void capture::update(tracked& state, signal<event_t> *ev, double scale, double rotation)
{
    if (state.fingers)
    {
        ev->mode = TAKEN;
        wayland.update(state.gesture, state.fingers, ev->event->time_msec, ev->event->dx, ev->event->dy,
            scale, rotation);
    }
}

template<class event_t>
void capture::end(tracked& state, signal<event_t> *ev)
{
    if (state.fingers)
    {
        ev->mode = TAKEN;
        wayland.end(state.gesture, state.fingers, ev->event->time_msec, ev->event->cancelled);
        state.fingers = 0;
    }
}
}
