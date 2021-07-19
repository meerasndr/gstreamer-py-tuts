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

def handle_message(playbin, playing, terminate, seek_enabled, msg):
    if msg.type == Gst.MessageType.ERROR:
        err, debug_info = msg.parse_error()
        logger.error(f"Error received from element {msg.src.get_name()}: {err.message}")
        logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
        terminate = True
    elif msg.type == Gst.MessageType.EOS:
        logger.info("End-Of-Stream reached.")
        terminate = True
    elif msg.type == Gst.MessageType.DURATION_CHANGED:
        # The duration has changed, mark the current one as invalid
        duration = Gst.CLOCK_TIME_NONE

    elif msg.type == Gst.MessageType.STATE_CHANGED:
            if msg.src == playbin:
                old_state, new_state, pending_state = msg.parse_state_changed()
                logger.info(f"Pipeline state changed from {Gst.Element.state_get_name(old_state)} to {Gst.Element.state_get_name(new_state)}")
                # Remember whether we are in the PLAYING state or not
                playing = (new_state == Gst.State.PLAYING)
                if playing: # We just moved to PLAYING. Check if seeking is possible
                    #create new query object
                    seekquery = Gst.Query.new_seeking(Gst.Format.TIME)
                    if playbin.query(seekquery):
                        format, seek_enabled, start, end = seekquery.parse_seeking()
                        if seek_enabled:
                            logger.info(f"Seeking is ENABLED {seek_enabled} from {start} to {end}")
                        else:
                            logger.info("Seeking is disabled for this stream\n") # Live video stream?
                    else:
                        logger.error("Seeking query failed")
    else:
        # This should ideally not be reached
        logger.error("Unexpected message received.")

    return


# Initialize GStreamer
# Setup internal path lists, plugins, GstRegistry
Gst.init(None)

# Creat the single element playbin
global playbin
playbin = Gst.ElementFactory.make("playbin", "playbin")

# Are we in playing state?
global playing
playing = False
# Should we terminate execution?
global terminate
terminate = False
# Is seeking enabled for this media?
global seek_enabled
seek_enabled = False
# Have we performed the seek already?
global seek_done
seek_done = False
# How long does this media last, in nanoseconds?
global duration
duration = Gst.CLOCK_TIME_NONE

if not playbin:
    logger.error("Not all elements could be created")
    sys.exit(1)

# Set URI to play
playbin.props.uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm"

# Grouping data
data = {'playbin': playbin, 'playing': playing, 'terminate': terminate,
'seek_enabled': seek_enabled, 'seek_done': seek_done, 'duration': duration}


# Change state to playing
# Goal: start playing pipeline
# element / pipeline states: NULL -> READY -> PAUSED -> PLAYING.
# When a pipeline is moved to PLAYING state, it goes through all these 4 states internally
# NULL : Default start(and end) state. No resources allocated
# READY : Global resources allocated -> opening devices, buffer allocation. Stream is not opened
# PAUSED: Stream opened, but not being processed
# PLAYING: Stream opened and processing happens -> running clock
ret = playbin.set_state(Gst.State.PLAYING)
if ret == Gst.StateChangeReturn.FAILURE:
    logger.error("Unable to change state of pipeline to playing")
    sys.exit(1)
else:
    logger.info("uri prop added")

#Listening for messages
# Why is a bus required?
# Bus takes care of forwarding messages from pipeline (running in a separate thread) to the application
# Applications can avoid worrying about communicating with streaming threads / the pipeline directly.
bus = playbin.get_bus()

while True:
    msg = bus.timed_pop_filtered(100 * Gst.MSECOND, \
    Gst.MessageType.STATE_CHANGED | Gst.MessageType.ERROR |  \
    Gst.MessageType.EOS | Gst.MessageType.DURATION_CHANGED)
    if msg:
        handle_message(playbin, playing, terminate, seek_enabled, msg)
    else:
        if playing:
            print("Blah playing")
            ret, current = playbin.query_position(Gst.Format.TIME)
            if not ret:
                logger.error("Could not query current position")
            if duration != Gst.CLOCK_TIME_NONE:
                ret, duration = playbin.query_duration(Gst.Format.TIME)
                if not ret:
                    logger.error("Could not query duration")
            # Print current position and total duration
            print(f"Current Position: {current} Total Duration: {duration}")

            if seek_enabled and not seek_done and current > 10 * Gst.SECOND:
                print("Reached 10s, performing seek ...\n")
                playbin.seek_simple(Gst.Format.TIME, \
                Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT,\
                 30 * Gst.SECOND)
                 #Gst.SeekFlags.FLUSH - This discards all data currently in the pipeline before doing the seek.
                 #Might pause a bit while the pipeline is refilled and the new data starts to show up,
                 #but greatly increases the “responsiveness” of the application. If this flag is not provided,
                 #“stale” data might be shown for a while until the new position appears at the end of the pipeline
                 # Gst.SeekFlags.KEY_UNIT - With most encoded video streams, seeking to arbitrary positions is
                 # not possible but only to certain frames called Key Frames. When this flag is used,
                 # the seek will actually move to the closest key frame and start producing data straight away.
                 # If this flag is not used, the pipeline will move internally to the closest key frame (it has no other alternative)
                 # but data will not be shown until it reaches the requested position.
                 # This last alternative is more accurate, but might take longer.
                seek_done = True
    if terminate:
        playbin.set_state(Gst.State.NULL)
        break
