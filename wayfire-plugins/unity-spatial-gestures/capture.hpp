/* capture.hpp
 *
 * Copyright 2026 Muqtadir
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <wayfire/signal-definitions.hpp>
#include <wayfire/nonstd/wlroots-full.hpp>

#include "protocol.hpp"

namespace unity_spatial_gestures
{
class capture
{
  public:
    explicit capture(protocol& sink);

  private:
    template<class event_t> using signal = wf::input_event_signal<event_t>;

    struct tracked
    {
        kind gesture;
        uint32_t fingers = 0;
    };

    template<class event_t> void begin(tracked& state, signal<event_t> *ev);
    template<class event_t> void update(tracked& state, signal<event_t> *ev, double scale, double rotation);
    template<class event_t> void end(tracked& state, signal<event_t> *ev);

    protocol& wayland;
    tracked swipe{kind::swipe};
    tracked pinch{kind::pinch};

    wf::signal::connection_t<signal<wlr_pointer_swipe_begin_event>> on_swipe_begin;
    wf::signal::connection_t<signal<wlr_pointer_swipe_update_event>> on_swipe_update;
    wf::signal::connection_t<signal<wlr_pointer_swipe_end_event>> on_swipe_end;
    wf::signal::connection_t<signal<wlr_pointer_pinch_begin_event>> on_pinch_begin;
    wf::signal::connection_t<signal<wlr_pointer_pinch_update_event>> on_pinch_update;
    wf::signal::connection_t<signal<wlr_pointer_pinch_end_event>> on_pinch_end;
};
}
