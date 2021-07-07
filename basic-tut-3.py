#!/uusr/bin/env python3
import sys
import gi
import logger

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")
from gi.repository import GLib, GObject, GSt

logging.basicConfig(level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

class CustomData:
    def __init__(self):

        #Initialize GStreamer
        Gst.init(None)

        #Create Elements
        self.source = Gst.ElementFactory.make("uridecodebin", "source")
        self.convert = Gst.ElementFactory.make("audioconvert", "convert")
        self.resample = Gst.ElementFactory.make("audioresample", "resample")
        self.sink = Gst.ElementFactory.make("autoaudiosink", "sink")

        #Create Empty pipeline
        self.pipeline = Gst.Pipeline.new("test-pipeline")
        if not self.pipeline or not self.source or not self.convert or not self.resample or not self.sink:
            logger.error("Not all elements could be created")
            sys.exit(1)

        #Add elements to pipeline
        self.pipeline.add(self.source, self.convert, self.resample, self.sink)

        #Link all elements excluding source (which contains demux)
        ret = self.convert.link(self.resample)
        ret = ret and self.resample.link(self.sink)
        if not ret:
            logger.error("Elements could not be linked")
            sys.exit(1)

        #Set URI to play
        self.source.props.uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm"

        #Connect to pad-added signal and handle this in function pad_added_handler
        #Is `pad-added` a built-in signal name?
        self.source.connect("pad-added", self.pad_added_handler)

        #Start PLAYING
        ret = self.pipeline.set_state(Gst.State.PLAYING)
        if ret == Gst.StateChangeReturn.FAILURE:
            logger.error("Unable to change state of pipeline to playing")
            sys.exit(1)

        #Listening for messages:
        bus = self.pipeline.get_bus()
        msg = self.bus.timed_pop_filtered(Gst.CLOCK_TIME_NONE, Gst.MessageType.ERROR | Gst.MessageType.STATE_CHANGED | Gst.MessageType.EOS)

        #Parse message
        if msg:
            if msg.type == Gst.MessageType.ERROR:
                err, debug_info = msg.parse_error()
                logger.error(f"Error received from element {msg.src.get_name()}: {err.message}")
                logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
            elif msg.type == Gst.MessageType.EOS:
                logger.info("End-Of-Stream reached.")
            else:
                # This should ideally not be reached
                logger.error("Unexpected message received.")
