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

class CustomData:
    def __init__(data):
        # Initialize GStreamer
        # Setup internal path lists, plugins, GstRegistry
        Gst.init(None)

        # Creat the single element playbin
        data.playbin = Gst.ElementFactory.make("playbin", "playbin")

        # Are we in playing state?
        data.playing = False
        # Should we terminate execution?
        data.terminate = False
        # Is seeking enabled for this media?
        data.seek_enabled = False
        # Have we performed the seek already?
        data.seek_done = False
        # How long does this media last, in nanoseconds?
        data.duration = Gst.CLOCK_TIME_NONE

        if not data.playbin:
            logger.error("Not all elements could be created")
            sys.exit(1)

        # Set URI to play
        data.playbin.props.uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm"

        # Change state to playing
        ret = data.playbin.set_state(Gst.State.PLAYING)
        if ret == Gst.StateChangeReturn.FAILURE:
            logger.error("Unable to change state of pipeline to playing")
            sys.exit(1)
        else:
            logger.info("uri prop added")

        #Listening for messages
        # Why is a bus required? Bus takes care of forwarding messages from pipeline (running in a separate thread) to the application
        # Applications can avoid worrying about communicating with streaming threads / the pipeling directly.
        # Applications only need to set a message-handler on a bus
        # Bus is periodically checked for messages, and callback is called when a message is available
        data.bus = data.playbin.get_bus()
        while True:
            msg = data.bus.timed_pop_filtered(100 * Gst.MSECOND, \
            Gst.MessageType.STATE_CHANGED | Gst.MessageType.ERROR |  \
            Gst.MessageType.EOS | Gst.MessageType.DURATION_CHANGED)
            if msg:
                data.handle_message(msg)
            else:
                if data.playing :
                    #current = -1
                    ret, current = data.playbin.query_position(Gst.Format.TIME)
                    if not ret:
                        logger.error("Could not query current position")
                    if data.duration != Gst.CLOCK_TIME_NONE:
                        ret, data.duration = data.query_duration(Gst.Format.TIME)
                        if not ret:
                            logger.error("Could not query duration")
                    # Print current position and total duration
                    print(f"Current Position: {current} Total Duration: {data.duration}")

                    if data.seek_enabled and not data.seek_done and current > 10 * Gst.SECOND:
                        print("Reached 10s, performing seek ...\n")
                        data.playbin.seek_simple(Gst.Format.TIME, Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT,\
                         30 * Gst.SECOND)
                        data.seek_done = True
            if data.terminate:
                data.playbin.set_state(Gst.State.NULL)
                break

    def handle_message(data, msg):
        if msg.type == Gst.MessageType.ERROR:
            err, debug_info = msg.parse_error()
            logger.error(f"Error received from element {msg.src.get_name()}: {err.message}")
            logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
            data.terminate = True
        elif msg.type == Gst.MessageType.EOS:
            logger.info("End-Of-Stream reached.")
            data.terminate = True
        elif msg.type == Gst.MessageType.DURATION_CHANGED:
            # The duration has changed, mark the current one as invalid
            data.duration = Gst.CLOCK_TIME_NONE

        elif msg.type == Gst.MessageType.STATE_CHANGED:
                if msg.src == data.playbin:
                    old_state, new_state, pending_state = msg.parse_state_changed()
                    logger.info(f"Pipeline state changed from {Gst.Element.state_get_name(old_state)} to {Gst.Element.state_get_name(new_state)}")
                    # Remember whether we are in the PLAYING state or not
                    data.playing = (new_state == Gst.State.PLAYING)
                    if data.playing:
                        seekquery = Gst.Query.new_seeking(Gst.Format.TIME)
                        if data.playbin.query(seekquery):
                            format, data.seek_enabled, start, end = seekquery.parse_seeking()
                            if data.seek_enabled:
                                logger.info(f"Seeking is ENABLED {data.seek_enabled} from {start} to {end}")
                            else:
                                logger.info("Seeking is disabled for this stream\n")
                        else:
                            logger.error("Seeking query failed")
        else:
            # This should ideally not be reached
            logger.error("Unexpected message received.")

        return

if __name__ == '__main__':
    c = CustomData()
