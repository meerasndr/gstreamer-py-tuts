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
        # pending:gst_structure_foreach (structure, print_field, (gpointer) pfx);


def print_pad_templates_info(factory):
    print(f"Pad templates for {factory}")  # gst_element_factory_get_longname pending
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


def main():
    Gst.init(None)
    sourcefactory = Gst.ElementFactory.find("audiotestsrc")
    sinkfactory = Gst.ElementFactory.find("autoaudiosink")
    if not sourcefactory or not sinkfactory:
        logger.error("Not all element factories could be created")
        sys.exit(1)
    print_pad_templates_info(sourcefactory)
    print_pad_templates_info(sinkfactory)


if __name__ == "__main__":
    main()
