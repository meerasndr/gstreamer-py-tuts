#!/usr/bin/env python3

import sys
import gi
import logging

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")

from gi.repository import Gst, GObject, GLib

logging.basicConfig(level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

#Initialize GStreamer
Gst.init(sys.argv[1:])

#Create elements
source = Gst.ElementFactory.make("videotestsrc", "source")
sink = Gst.ElementFactory.make("autovideosink", "sink")

#Create empty pipeline
pipeline = Gst.Pipeline.new("test-pipeline")
print("Pipeline: ", pipeline)

if not pipeline or not source or not sink:
    logger.error("Not all elements could be created")
    sys.exit(1)
#Add elements to pipeline
pipeline.add(source, sink)
if not source.link(sink):
    logger.error("Elements not linked")
    sys.exit(1)


#set source's properties
source.props.pattern = 1

#Start playing

ret = pipeline.set_state(Gst.State.PLAYING)
if ret == Gst.StateChangeReturn.FAILURE:
    logger.error("Not able to play the pipeline")
    sys.exit(1)

# Wait for EOS or error

bus = pipeline.get_bus()
msg = bus.timed_pop_filtered(Gst.CLOCK_TIME_NONE, Gst.MessageType.ERROR | Gst.MessageType.EOS)

#Parse message

if msg:
    if msg.type == Gst.MessageType.ERROR:
        err, debug_info = msg.parse_error()
        logger.error(f"Error received from element {msg.src.get_name()}: {err.message}")
        logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
    elif msg.type == Gst.MessageType.EOS:
        logger.info("End-Of-Stream reached.")
    else:
        # This should not happen as we only asked for ERRORs and EOS
        logger.error("Unexpected message received.")

#Free stuff ?
pipeline.set_state(Gst.State.NULL)
