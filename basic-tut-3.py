#!/usr/bin/env python3
import sys
import gi
import logging

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")
from gi.repository import GLib, GObject, Gst

logging.basicConfig(level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

class CustomData():
    def __init__(self):
        #Create Elements
        #uridecodebin -> Decodes data from a URI into raw media (instantiates source, demuxer, decoder internally)
        self.source = Gst.ElementFactory.make("uridecodebin", "source")
        #audioconvert -> Convert audio to different formats. Ensures platform interoperability
        self.convert = Gst.ElementFactory.make("audioconvert", "convert")
        #audioresample -> Useful for converting between different audio sample rates. Again ensures platform interoperability
        self.resample = Gst.ElementFactory.make("audioresample", "resample")
        #autoaudiosink -> render the audio stream to the audio card
        self.sink = Gst.ElementFactory.make("autoaudiosink", "sink")
        #Create Empty pipeline
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

    #Linking
    ret = new_pad.link(sink_pad)
    if ret == Gst.PadLinkReturn.OK:
        print("Link success")
    else:
        print("Link failure")

    return


def main():
    # Initialize GStreamer library
    # set up internal path lists, register built-in elements, load standard plugins (GstRegistry)
    Gst.init(None)
    data = CustomData()

    if not data.pipeline or not data.source or not data.convert or not data.resample or not data.sink:
        logger.error("Not all elements could be created")
        sys.exit(1)

    #Add elements to pipeline, the elements are not linked yet!
    data.pipeline.add(data.source, data.convert, data.resample, data.sink)

    #Link all elements excluding source (which contains demux, hence dynamic)
    ret = data.convert.link(data.resample)
    ret = ret and data.resample.link(data.sink)
    if not ret:
        logger.error("Elements could not be linked")
        sys.exit(1)

    #Set URI to play
    data.source.props.uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm"
    #data.source.props.uri = "https://file-examples-com.github.io/uploads/2020/03/file_example_WEBM_480_900KB.webm"
    #data['source'].props.uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer_gr.srt"
    #The uridecodebin element comes with several element signals including `pad-added`
    #When uridecodebin(source) creates a source pad, and emits `pad-added` signal, the callback is invoked
    #Non-blocking
    data.source.connect("pad-added", pad_added_handler, data)

    # Grouping all the pipeline data for ease of use in the pad_added_handler callback
    #data = {'source': source, 'convert': convert, 'resample': resample, 'sink': sink, 'pipeline': pipeline,}

    # Goal: start playing pipeline
    # element / pipeline states: NULL -> READY -> PAUSED -> PLAYING.
    # When a pipeline is moved to PLAYING state, it goes through all these 4 states internally
    # NULL : Default start(and end) state. No resources allocated
    # READY : Global resources allocated -> opening devices, buffer allocation. Stream is not opened
    # PAUSED: Stream opened, but not being processed
    # PLAYING: Stream opened and processing happens -> running clock

    # In this case, the pipeline first moves from NULL to READY
    # pad-added signals are triggered, callback is executed
    # Once there is a link success, pipeline moves to PAUSED, then to PLAY

    ret = data.pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        logger.error("Unable to change state of pipeline to playing")
        sys.exit(1)

    # Listening for messages: Wait for EOS or error
    # Why is a bus required? Bus takes care of forwarding messages from pipeline (running in a separate thread) to the application
    # Applications can avoid worrying about communicating with streaming threads / the pipeling directly.
    # Applications only need to set a message-handler on a bus
    # Bus is periodically checked for messages, and callback is called when a message is available
    bus = data.pipeline.get_bus()
    while True:
        msg = bus.timed_pop_filtered(100 * Gst.MSECOND, Gst.MessageType.ERROR | Gst.MessageType.STATE_CHANGED | Gst.MessageType.EOS)
        #Parse message
        if msg:
            if msg.type == Gst.MessageType.ERROR:
                err, debug_info = msg.parse_error()
                logger.error(f"Error received from element {msg.src.get_name()}: {err.message}")
                logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
                break
            elif msg.type == Gst.MessageType.EOS:
                logger.info("End-Of-Stream reachedata.")
                break

            elif msg.type == Gst.MessageType.STATE_CHANGED:
                if msg.src == data.pipeline:
                    old_state, new_state, pending_state = msg.parse_state_changed()
                    print(f"Pipeline state changed from {Gst.Element.state_get_name(old_state)} to {Gst.Element.state_get_name(new_state)}")
            else:
                logger.error("Unexpected message")
                break


# Main section
if __name__ == '__main__':
    main()
