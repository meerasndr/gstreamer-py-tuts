#!/usr/bin/env python3
import sys
import logging
import gi
import signal

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")
gi.require_version("GstBase", "1.0")
gi.require_version("GstAudio", "1.0")
from gi.repository import GLib, GObject, Gst, GstAudio

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
        self.audio_resample = Gst.ElementFactory.make("audioresample", "audio_resample")
        self.audio_sink = Gst.ElementFactory.make("autoaudiosink", "audio_sink")
        self.video_queue = Gst.ElementFactory.make("queue", "video_queue")
        self.audio_convert2 = Gst.ElementFactory.make("audioconvert", "audio_convert2")
        self.visual = Gst.ElementFactory.make("wavescope", "visual")
        self.video_convert = Gst.ElementFactory.make("videoconvert", "video_convert")
        self.video_sink = Gst.ElementFactory.make("autovideosink", "video_sink")
        self.app_queue = Gst.ElementFactory.make("queue", "app_queue")
        self.app_sink = Gst.ElementFactory.make("appsink", "app_sink")
        self.pipeline = Gst.Pipeline.new("test-pipeline")
        self.a, self.b, self.c, self.d = (
            0,
            0,
            0,
            0,
        )  # for waveform generation
        self.sourceid = 0  # to control GSource
        self.num_samples = (
            0  # number of samples generated so far (for time stamp generation)
        )
        self.main_loop = None


def push_data(data):
    chunk_size = 1024
    num_samples = chunk_size // 2  # because each sample is 16 bits
    # Create a new empty buffer
    buffer = Gst.Buffer.new_allocate(None, chunk_size, None)

    # To-Do: set buffer timestamp and duration
    
    # Generate some psychedelic waveforms aka make buffer data
    ret, map_info = buffer.map(Gst.MapFlags.WRITE)
    raw = map_info.data
    #print("Raw:", raw)
    print("Raw Type", type(raw))
    raw = list(raw)
    #print("Raw:", raw)
    print("Raw Type", type(raw))
    data.c += data.d
    data.d -= data.c // 1000
    freq = 1100 + 1000 * data.d
    for i in range(num_samples):
        data.a += data.b
        data.b -= data.a // freq
        raw[i] = int(500 * data.a)

    data.num_samples += num_samples

    # Push the buffer into the appsrc
    ret = data.app_source.emit("push-buffer", buffer)
    if ret != Gst.FlowReturn.OK:
        return False

    return True


def start_feed(object, arg0, data):
    if data.sourceid == 0:
        print("Start feeding\n")
        # data.sourceid =
        GLib.timeout_add(10, push_data, data)
        data.sourceid = 1


def stop_feed(object, data):
    if data.sourceid != 0:
        print("Stop feeding\n")
        GObject.source_remove(data.sourceid)
        data.sourceid = 0


def new_sample(object, data):
    sample = data.app_sink.emit("pull-sample")
    if sample:
        print("*")
        return Gst.FlowReturn.OK

    return Gst.FlowReturn.ERROR


def error_cb(bus, msg, data):
    err, debug_info = msg.parse_error()
    logger.error(f"Error received from element {msg.src.get_name()}: {err.message}")
    logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
    data.main_loop.quit()


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

    # Configure wavescope
    data.visual.set_property("shader", 0)
    data.visual.set_property("style", 0)

    # Configure appsrc
    sample_rate = 44100
    info = GstAudio.AudioInfo()
    info.set_format(GstAudio.AudioFormat.S16, sample_rate, 1, None)
    audio_caps = info.to_caps()
    data.app_source.set_property("caps", audio_caps)
    data.app_source.set_property("format", Gst.Format.TIME)
    data.app_source.connect("need-data", start_feed, data)
    data.app_source.connect("enough-data", stop_feed, data)

    # configure appsink
    data.app_sink.set_property("emit-signals", True)
    data.app_sink.set_property("caps", audio_caps)
    data.app_sink.connect("new-sample", new_sample, data)

    # add elements to pipeline
    data.pipeline.add(
        data.app_source,
        data.tee,
        data.audio_queue,
        data.audio_convert1,
        data.audio_resample,
        data.audio_sink,
        data.video_queue,
        data.audio_convert2,
        data.visual,
        data.video_convert,
        data.video_sink,
        data.app_queue,
        data.app_sink,
    )
    # Link elements with "Always" pads
    ret1 = data.app_source.link(data.tee)
    ret2 = (
        data.audio_queue.link(data.audio_convert1)
        and data.audio_convert1.link(data.audio_resample)
        and data.audio_resample.link(data.audio_sink)
    )
    ret3 = (
        data.video_queue.link(data.audio_convert2)
        and data.audio_convert2.link(data.visual)
        and data.visual.link(data.video_convert)
        and data.video_convert.link(data.video_sink)
    )
    ret4 = data.app_queue.link(data.app_sink)

    if not ret1 or not ret2 or not ret3 or not ret4:
        logger.error("Elements could not be linked")
        sys.exit(1)

    # Manually link tee, which has request pads
    tee_audio_pad = data.tee.get_request_pad("src_%u")
    logger.info("Obtained request pad for audio branch")  # To do: add gst_pad_get_name
    queue_audio_pad = data.audio_queue.get_static_pad("sink")

    tee_video_pad = data.tee.get_request_pad("src_%u")
    logger.info("Obtained request pad for video branch")  # To do: add gst_pad_get_name
    queue_video_pad = data.video_queue.get_static_pad("sink")

    tee_app_pad = data.tee.get_request_pad("src_%u")
    logger.info("Obtained request pad for app branch")
    queue_app_pad = data.app_queue.get_static_pad("sink")

    ret_audio = tee_audio_pad.link(queue_audio_pad)
    ret_video = tee_video_pad.link(queue_video_pad)
    ret_app = tee_app_pad.link(queue_app_pad)
    if ret_audio != 0 or ret_video != 0 or ret_app != 0:
        logger.error("Tee could not be linked")
        sys.exit(1)

    # Setup bus and message handlers
    bus = data.pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message::error", error_cb, data)

    # Play pipeline
    data.pipeline.set_state(Gst.State.PLAYING)
    print("Pipeline play")

    # Create a GLib Main loop and set it to return
    data.main_loop = GLib.MainLoop()
    data.main_loop.run()


if __name__ == "__main__":
    main()
