#include <gst/gst.h>
#include <stdio.h>

typedef struct CustomData{
  GstElement *source;
  GstElement *convert;
  GstElement *resample;
  GstElement *sink;
  GstElement *pipeline;
} CustomData;

void pad_added_handler(GstElement *src, GstPad *newpad, CustomData *data){
  GstPad *sink_pad = gst_element_get_static_pad(data -> convert, "sink");
  printf("Received new pad \n");
  if(gst_pad_is_linked(sink_pad)){
    fprintf(stderr, "Already linked");
  }
  GstCaps *newpad_caps = gst_pad_get_current_caps(sink_pad);
  GstStructure *newpad_struct = gst_caps_get_structure(newpad_caps, 0);
  const gchar *newpad_type = gst_structure_get_name(newpad_struct);
  if (!g_str_has_prefix (newpad_type, "audio/x-raw")) {
    g_print ("Ignoring. Expected type: Raw Audio. Actual type: '%s' \n", newpad_type);
  }

  if (gst_pad_link(newpad, sink_pad) != 0 ){
    fprintf(stderr, "Linking failed \n");
  }
  else {
    fprintf(stderr, "Link success \n");
  }
}

int main(int argc, char * argv[]){
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  gboolean terminate = FALSE;
  gst_init(&argc, &argv);
  data.source = gst_element_factory_make("uridecodebin", "source");
  data.convert = gst_element_factory_make("audioconvert", "convert");
  data.resample = gst_element_factory_make("audioresample", "resample");
  data.sink = gst_element_factory_make("autoaudiosink", "sink");
  data.pipeline = gst_pipeline_new("test-pipeline");

  if(!data.source || !data.convert || !data.resample || !data.sink || !data.pipeline){
    fprintf(stderr, "Not all elements could be created.\n");
    return -1;
  }

  gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.convert, data.resample, data.sink, NULL);

  if (!gst_element_link_many(data.convert, data.resample, data.sink, NULL)){
    fprintf(stderr, "Not all elements could be linked.\n");
    return -1;
  }
  // Set URI to play
  g_object_set (data.source, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

  // Connect to the pad-added handler
  g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

// move tpipeline to playing state
  int ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  if (ret == 0){
    fprintf(stderr, "Could not change pipeline state to playing\n");
    return -1;
  }
  bus = gst_element_get_bus(data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS | GST_MESSAGE_ERROR | GST_MESSAGE_STATE_CHANGED);
    GError *err = NULL;
    gchar *dbg_info = NULL;
    switch(GST_MESSAGE_TYPE(msg)){
      case GST_MESSAGE_EOS:
        printf("End of stream reached.\n");
        terminate = TRUE;
        break;
      case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &dbg_info);
        g_print("Error received: %s \n", err->message);
        g_print("Error from: %s\n", GST_OBJECT_NAME(msg->src));
        g_print("Debug info: %s\n", dbg_info);
        terminate = TRUE;
        break;

      case GST_MESSAGE_STATE_CHANGED:
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline)){
          GstState old_state, new_state;
          gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
          g_print("Pipeline state has changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
        }
        break;
      default:
        g_print("Unexpected message received \n");
        break;
  }
} while(!terminate);

  gst_message_unref(msg);
  gst_object_unref(bus);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);

  return 0;
}
