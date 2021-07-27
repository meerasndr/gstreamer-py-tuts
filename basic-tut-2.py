#!/usr/bin/ienv python3

import sys
import gi
import logging

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")

from gi.repository import Gst, GObject, GLib

logging.basicConfig(
    level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s"
)
logger = logging.getLogger(__name__)

# Initialize GStreamer library-> set up internal path lists, register built-in elements, load standard plugins
# GstRegistry setup??
Gst.init(sys.argv[1:])

# Create elements
# GstElementFactory -> required to create a GstElement object.
# Element factories -> are the basic types in a Gst Registry. Describe all plugins and elements GStreamer can create
# Gst.ElemenFactory.make() = gst_element_factory_find(<factory_name>) and gst_element_factory_create(<factory>, <element_name>)
# `gst-inspect-1.0 videotestsrc` gives Factory details, Plugin details, Pad templates (source only) and more
# videotestsrc creates a test video pattern
# autovideosink displays the video on a window

source = Gst.ElementFactory.make("videotestsrc", "source")
sink = Gst.ElementFactory.make("autovideosink", "sink")
# Create empty pipeline
pipeline = Gst.Pipeline.new("test-pipeline")
print("Pipeline: ", pipeline)

if not pipeline or not source or not sink:
    logger.error("Not all elements could be created")
    sys.exit(1)

# Add elements to pipeline
# The elements are not linked yet
pipeline.add(source, sink)
# Link elements. Order matters.
if not source.link(sink):
    logger.error("Elements not linked")
    sys.exit(1)


# set source's properties
source.props.pattern = 1

# Goal: start playing pipeline
# element / pipeline states: NULL -> READY -> PAUSED -> PLAYING. When a pipeline is moved to PLAYING state, it goes through all these 4 states internally
# NULL : Default start(and end) state. No resources allocated
# READY : Global resources allocated -> opening devices, buffer allocation. Stream is not opened
# PAUSED: Stream opened, but not being processed
# PLAYING: Stream opened and processing happens -> running clock
# Below line is non-blocking, and does not need iteration. A separate thread is created, and processing happens on this thread

ret = pipeline.set_state(Gst.State.PLAYING)

if ret == Gst.StateChangeReturn.FAILURE:
    logger.error("Not able to play the pipeline")
    sys.exit(1)

# Wait for EOS or error
# Why is a bus required? Bus takes care of forwarding messages from pipeline (running in a separate thread) to the application
# Applications can avoid worrying about communicating with streaming threads / the pipeling directly.
# Applications only need to set a message-handler on a bus
# Bus is periodically checked for messages, and callback is called when a message is available
bus = pipeline.get_bus()
msg = bus.timed_pop_filtered(
    Gst.CLOCK_TIME_NONE, Gst.MessageType.ERROR | Gst.MessageType.EOS
)  # Blocking code

# Parse message

if msg:
    if msg.type == Gst.MessageType.ERROR:
        err, debug_info = msg.parse_error()
        logger.error(f"Error received from element {msg.src.get_name()}: {err.message}")
    elif msg.type == Gst.MessageType.EOS:
        logger.info("End-Of-Stream reached.")
    else:
        # This should not happen as we only asked for ERRORs and EOS
        logger.error("Unexpected message received.")

# Free stuff
pipeline.set_state(Gst.State.NULL)
