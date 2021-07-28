#!/usr/bin/env python3
import sys
import logging
import gi

gi.require_version("GLib", "2.0")
gi.require_version("GObject", "2.0")
gi.require_version("Gst", "1.0")
from gi.repository import GLib, GObject, Gst

logging.basicConfig(
    level=logging.DEBUG, format="[%(name)s] [%(levelname)8s] - %(message)s"
)
logger = logging.getLogger(__name__)


def print_field(field, value, prefix):
    str = Gst.value_serialize(value)
    print("%s %15s: %s\n" % (prefix, field, str))


def print_caps(caps, prefix):
    if caps.is_any():
        print(f"{prefix}ANY\n")
        return
    if caps.is_empty():
        print(f"{prefix}EMPTY\n")
        return

    for i, val in enumerate(caps):
        structure = caps.get_structure(i)
        print(f"{prefix} {Gst.Structure.get_name(structure)}")
        structure.foreach(print_field, prefix)


def print_pad_templates_info(factory):
    print(f"Pad templates for {factory}")  # pending: gst_element_factory_get_longname
    if not Gst.ElementFactory.get_num_pad_templates(factory):
        logger.info("None\n")
        return
    pads = Gst.ElementFactory.get_static_pad_templates(
        factory
    )  # pads is a list of padtemplates
    for pad in pads:
        padtemplate = pad.get()
        if padtemplate.direction == Gst.PadDirection.SRC:
            print(f"SRC template {padtemplate.name_template}\n")
        elif padtemplate.direction == Gst.PadDirection.SINK:
            print(f"SINK template {padtemplate.name_template}\n")
        else:
            print(f"UNKNOWN template {padtemplate.name_template}\n")

        if padtemplate.presence == Gst.PadPresence.ALWAYS:
            print("      Availability: Always\n")
        elif padtemplate.presence == Gst.PadPresence.SOMETIMES:
            print("       Availability: Sometimes\n")
        elif padtemplate.presence == Gst.PadPresence.REQUEST:
            print("       Availability: On Request\n")
        else:
            print("       Availability: Unknown\n")

        if padtemplate.get_caps():
            print("    Capabilities:\n")
            caps = padtemplate.get_caps()
            print_caps(caps, "         ")
            print(" ")


def print_pad_capabilities(element, pad_name):
    # Retrieve pad
    pad = Gst.Element.get_static_pad(element, pad_name)
    if not pad:
        logger.error("Could not retrieve pad %s" % pad_name)
        return

    caps = Gst.Pad.get_current_caps(pad)
    if not caps:
        caps = pad.query_caps(None)

    print("Caps for the %s pad:\n" % pad_name)
    print_caps(caps, "      ")


def main():
    Gst.init(None)
    sourcefactory = Gst.ElementFactory.find("audiotestsrc")
    sinkfactory = Gst.ElementFactory.find("autoaudiosink")
    if not sourcefactory or not sinkfactory:
        logger.error("Not all element factories could be created")
        sys.exit(1)
    # Print information about pad templates of these factories
    print_pad_templates_info(sourcefactory)
    print_pad_templates_info(sinkfactory)

    # Instantiate actual elements using the factories
    source = sourcefactory.create("source")
    sink = sinkfactory.create("sink")
    terminate = False

    # Creat empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")

    if not pipeline or not source or not sink:
        logger.error("Not all elements could be created\n")
        sys.exit(1)

    # Build pipeline
    # Add elements
    pipeline.add(source, sink)
    # Link Pipeline elements
    ret = source.link(sink)
    if not ret:
        logger.error("Elements in the pipeline could not be linked")
        sys.exit(1)

    print("In NULL state: \n")
    print_pad_capabilities(sink, "sink")

    # Start playing
    ret = pipeline.set_state(Gst.State.PLAYING)
    if not ret:
        logger.error("Pipeline state could not be changed to playing")

    # setup bus and messages
    bus = pipeline.get_bus()
    while not terminate:
        msg = bus.timed_pop_filtered(
            Gst.CLOCK_TIME_NONE,
            Gst.MessageType.ERROR | Gst.MessageType.EOS | Gst.MessageType.STATE_CHANGED
        )

        if msg:
            if msg.type == Gst.MessageType.EOS:
                logger.info("End of stream reached\n")
                terminate = True
                break
            elif msg.type == Gst.MessageType.ERROR:
                err, debug_info = msg.parse_error()
                logger.error(
                    f"Error received from element {msg.src.get_name()}: {err.message}"
                )
                logger.error(
                    f"Debug information {debug_info if debug_info else 'none'}"
                )
                terminate = True
                break
            elif msg.type == Gst.MessageType.STATE_CHANGED:
                if msg.src == pipeline:
                    old_state, new_state, pending_state = msg.parse_state_changed()
                    logger.info(
                        f"Pipeline changed from {Gst.Element.state_get_name(old_state)} to {Gst.Element.state_get_name(new_state)}"
                    )
                    # Print the current Capabilities of the sink element
                    print_pad_capabilities(sink, "sink")
            else:
                # Ideally should not reach here
                logger.error("Unknown message")


if __name__ == "__main__":
    main()
