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
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    audio_source = Gst.ElementFactory.make("audiotestsrc", "audio_source")
    tee = Gst.ElementFactory.make("tee", "tee")
    audio_queue = Gst.ElementFactory.make("queue", "audio_queue")
    audio_convert = Gst.ElementFactory.make("audioconvert", "audio_convert")
    audio_resample = Gst.ElementFactory.make("audioresample", "audio_resample")
    audio_sink = Gst.ElementFactory.make("autoaudiosink", "audio_sink")
    video_queue = Gst.ElementFactory.make("queue", "video_queue")
    visual = Gst.ElementFactory.make("wavescope", "visual")
    video_convert = Gst.ElementFactory.make("videoconvert", "video_convert")
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

    audio_source.set_property("freq", 215.0)
    visual.set_property("shader", 0)
    visual.set_property("style", 1)

    pipeline.add(
        audio_source,
        tee,
        audio_queue,
        audio_convert,
        audio_resample,
        audio_sink,
        video_queue,
        visual,
        video_convert,
        video_sink,
    )
    ret_first = audio_source.link(tee)
    ret_second = audio_queue.link(audio_convert)
    ret_second = ret_second and audio_convert.link(audio_resample)
    ret_second = ret_second and audio_resample.link(audio_sink)
    ret_third = video_queue.link(visual)
    ret_third = ret_third and visual.link(video_convert)
    ret_third = ret_third and video_convert(video_sink)

    if not ret_first or not ret_second or not ret_third:
        logger.error("Elements could not be linked\n")
        sys.exit(1)


if __name__ == "__main__":
    main()
