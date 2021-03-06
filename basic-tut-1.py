#!/usr/bin/env python3
import sys
import gi

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")

from gi.repository import Gst, GObject, GLib

pipeline = None
bus = None
message = None

# initialize GStreamer library-> set up internal path lists, register built-in elements, load standard plugins
Gst.init(sys.argv[1:])

# build the pipeline
# parse_launch() builds a pipeline and abstracts away the details
pipeline = Gst.parse_launch(
    "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm"
)

# start playing pipeline
pipeline.set_state(Gst.State.PLAYING)

# wait until EOS or error
# Why is a bus required? Bus takes care of forwarding messages from pipeline (running in a separate thread) to the application
# Applications can avoid worrying about communicating with streaming threads / the pipeling directly.
# Applications only need to set a message-handler on a bus
# Bus is periodically checked for messages, and callback is called when a message is available
bus = pipeline.get_bus()
msg = bus.timed_pop_filtered(
    Gst.CLOCK_TIME_NONE, Gst.MessageType.ERROR | Gst.MessageType.EOS
)

# free resources
pipeline.set_state(Gst.State.NULL)
