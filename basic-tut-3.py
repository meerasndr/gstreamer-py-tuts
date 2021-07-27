#!/usr/bin/env python3
import sys
import gi
import logging
import signal

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")
from gi.repository import GLib, GObject, Gst

logging.basicConfig(
    level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s"
)
logger = logging.getLogger(__name__)


class CustomData:
    def __init__(self):
        # Create Elements
        # uridecodebin -> Decodes data from a URI into raw media (instantiates source, demuxer, decoder internally)
        self.source = Gst.ElementFactory.make("uridecodebin", "source")
        # audioconvert -> Convert audio to different formats. Ensures platform interoperability
        self.convert = Gst.ElementFactory.make("audioconvert", "convert")
        # audioresample -> Useful for converting between different audio sample rates. Again ensures platform interoperability
        self.resample = Gst.ElementFactory.make("audioresample", "resample")
        # autoaudiosink -> render the audio stream to the audio card
        self.sink = Gst.ElementFactory.make("autoaudiosink", "sink")
        # Create Empty pipeline
        self.pipeline = Gst.Pipeline.new("test-pipeline")


# callback function
# src is the GstElement which triggered the signal.
# new_pad is the GstPad that has just been added to the src element
# This is the pad to which we want to link.


def pad_added_handler(src, new_pad, data):
    sink_pad = data.convert.get_static_pad("sink")
    print(f"Received new pad {new_pad.get_name()} from {src.get_name()}")

    if sink_pad.is_linked():
        print("Already linked")
        return

    new_pad_caps = new_pad.get_current_caps()
    new_pad_struct = new_pad_caps.get_structure(0)
    new_pad_type = new_pad_struct.get_name()

    if not new_pad_type.startswith("audio/x-raw"):
        print("Type not audio.")
        return

    # Linking
    ret = new_pad.link(sink_pad)
    if ret == Gst.PadLinkReturn.OK:
        print("Link success")
    else:
        print("Link failure")

    return


def main():
    # Initialize GStreamer library
    Gst.init(None)
    signal.signal(signal.SIGINT, signal.SIG_DFL)  # signal.signal(signalnum, handler)
    data = CustomData()
    if (
        not data.pipeline
        or not data.source
        or not data.convert
        or not data.resample
        or not data.sink
    ):
        logger.error("Not all elements could be created")
        sys.exit(1)

    # Add elements to pipeline, the elements are not linked yet!
    data.pipeline.add(data.source, data.convert, data.resample, data.sink)

    # Link all elements excluding source (which contains demux, hence dynamic)
    ret = data.convert.link(data.resample)
    ret = ret and data.resample.link(data.sink)
    if not ret:
        logger.error("Elements could not be linked")
        sys.exit(1)

    # Set URI to play
    data.source.props.uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm"

    # The uridecodebin element comes with several element signals including `pad-added`
    # When uridecodebin(source) creates a source pad, and emits `pad-added` signal, the callback is invoked
    # Non-blocking
    data.source.connect("pad-added", pad_added_handler, data)

    ret = data.pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        logger.error("Unable to change state of pipeline to playing")
        sys.exit(1)

    # Listening for messages: Wait for EOS or error
    bus = data.pipeline.get_bus()
    while True:
        msg = bus.timed_pop_filtered(
            Gst.CLOCK_TIME_NONE,
            Gst.MessageType.ERROR | Gst.MessageType.STATE_CHANGED | Gst.MessageType.EOS,
        )
        # Parse message
        if msg:
            if msg.type == Gst.MessageType.ERROR:
                err, debug_info = msg.parse_error()
                logger.error(
                    f"Error received from element {msg.src.get_name()}: {err.message}"
                )
                logger.error(
                    f"Debugging information: {debug_info if debug_info else 'none'}"
                )
                break
            elif msg.type == Gst.MessageType.EOS:
                logger.info("End-Of-Stream reachedata.")
                break

            elif msg.type == Gst.MessageType.STATE_CHANGED:
                if msg.src == data.pipeline:
                    old_state, new_state, pending_state = msg.parse_state_changed()
                    print(
                        f"Pipeline state changed from {Gst.Element.state_get_name(old_state)} to {Gst.Element.state_get_name(new_state)}"
                    )
            else:
                logger.error("Unexpected message")
                break


# Main section
if __name__ == "__main__":
    main()
