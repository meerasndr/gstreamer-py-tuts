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


class CustomData:
    def __init__(self):
        self.app_source = Gst.ElementFactory.make("appsrc", "audio_source")
        self.tee = Gst.ElementFactory.make("tee", "tee")
        self.audio_queue = Gst.ElementFactory.make("queue", "audio_queue")
        self.audio_convert1 = Gst.ElementFactory.make("audioconvert", "audio_convert1")
        self.audio_resample = Gst.ElementFactory.make(
            "audio_resample", "audio_resample"
        )
        self.audio_sink = Gst.ElementFactory.make("autoaudiosink", "audio_sink")
        self.video_queue = Gst.ElementFactory.make("queue", "video_queue")
        self.audio_convert2 = Gst.ElementFactory.make("audioconvert", "audio_convert2")
        self.visual = Gst.ElementFactory.make("wavescope", "visual")
        self.video_convert = Gst.ElementFactory.make("videoconvert", "video_convert")
        self.video_sink = Gst.ElementFactory.make("autovideosink", "video_sink")
        self.app_queue = Gst.ElementFactory.make("queue", "app_queue")
        self.app_sink = Gst.ElementFactory.make("appsink", "app_sink")
        self.pipeline = Gst.Pipeline.new("test-pipeline")
        self.a, self.b, self.c, self.d = None  # for waveform generation
        self.sourceid = None  # to control GSource
        self.num_samples = (
            None  # number of samples generated so far (for time stamp generation)
        )


def push_data():
    pass


def start_feed():
    pass


def stop_feed():
    pass


def new_sample():
    pass


def main():
    Gst.init(None)
    data = CustomData()
    if (
        not data.pipeline
        or not data.app_source
        or not data.tee
        or not data.audio_queue
        or not data.audio_convert1
        or not data.audio_resample
        or not data.audio_sink
        or not data.video_queue
        or not data.audio_convert2
        or not data.visual
        or not data.video_convert
        or not data.video_sink
        or not data.app_queue
        or not data.app_sink
    ):
        logger.error("Not all elements could be created")
        sys.exit(1)
