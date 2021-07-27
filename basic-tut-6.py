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


def print_pad_templates_info(factory):
    print(f"Pad templates for {factory}")  # gst_element_factory_get_longname pending
    if not Gst.ElementFactory.get_num_pad_templates(factory):
        logger.info("None\n")
        return
    pads = Gst.ElementFactory.get_static_pad_templates(
        factory
    )  # pads is a list of padtemplates
    print("Pads", pads)
    for pad in pads:
        padtemplate = pad.get()
        # pad = GLib.List.next(pad) #GLib.List is a struct used to represent each element in the doubly linked list

        if padtemplate.direction == Gst.PadDirection.SRC:
            logger.info(f"SRC template {padtemplate.name_template}\n")
        elif padtemplate.direction == Gst.PadDirection.SINK:
            logger.info(f"SINK template {padtemplate.name_template}\n")
        else:
            logger.info(f"UNKNOWN template {padtemplate.name_template}\n")

        if padtemplate.presence == Gst.PadPresence.ALWAYS:
            logger.info("      Availability: Always\n")
        elif padtemplate.presence == Gst.PadPresence.SOMETIMES:
            logger.info("       Availability: Sometimes\n")
        elif padtemplate.presence == Gst.PadPresence.REQUEST:
            logger.info("       Availability: On Request\n")
        else:
            logger.info("       Availability: Unknown\n")

        # Pending:
        """
            if (padtemplate->static_caps.string) {
          GstCaps *caps;
          g_print ("    Capabilities:\n");
          caps = gst_static_caps_get (&padtemplate->static_caps);
          print_caps (caps, "      ");
          gst_caps_unref (caps);

         }"""


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
