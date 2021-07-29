#!/usr/bin/env python3
import sys
import logging
import gi
import signal

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")
from gi.repository import GLib, GObject, Gst

logging.basicConfig(
    level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s"
)
logger = logging.getLogger(__name__)


def main():
    Gst.init(None)
    signal.Signal(signal.SIGINT, signal.SIG_DFL)
    audio_source = Gst.ElementFactory.make("audiotestsrc", "audio_source")
    tee = Gst.ElementFactory.make("tee", "tee")
    audio_queue = Gst.ElementFactory.make("queue", "audio_queue")
    audio_convert = Gst.ElementFactory.make("audioconvert", "audio_convert")
    audio_resample = Gst.ElementFactory.make("audioresample", "audio_resample")
    audio_sink = Gst.ElementFactory.make("autoaudiosink", "audio_sink")
    video_queue = Gst.ElementFactory.make("queue", "video_queue")
    visual = Gst.ElementFactory.make("wavescope", "visual")
    video_convert = Gst.ElementFactory.make("videoconvert", "video_sonvert")
    video_sink = Gst.ElementFactory.make("autovideosink", "video_sink")

    # Create empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")
    if (
        not pipeline
        or not audio_source
        or not tee
        or not audio_queue
        or not audio_convert
        or not audio_resample
        or not audio_sink
        or not video_queue
        or not visual
        or not video_convert
        or not video_sink
    ):
        logger.error("Not all elements could be created \n")
        sys.exit(1)

    
