#include<gst/gst.h>
#include<stdio.h>

int main(int argc, char *argv[]){
  GstElement *source, *sink;
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;
  gst_init(&argc, &argv);
  source = gst_element_factory_make("videotestsrc", "source");
  sink = gst_element_factory_make("autovideosink", "sink");
  pipeline = gst_pipeline_new("test-pipeline");

  if(!pipeline || !source || !sink) {
    fprintf(stderr, "Not all elements could be created.\n");
    return -1;
  }
  gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
  if (!gst_element_link(source, sink)){
    fprintf(stderr, "Elements could not be linked \n");
    return -1;
  }
  int ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == 0){
    fprintf(stderr, "Could not change pipeline state to playing\n");
    return -1;
  }

  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS | GST_MESSAGE_ERROR);

  if(msg){
    GError *err = NULL;
    gchar *dbg_info = NULL;

    switch(GST_MESSAGE_TYPE(msg)){
      case GST_MESSAGE_EOS:
        printf("End of stream reached.\n");
        break;
      case GST_MESSAGE_ERROR:

        gst_message_parse_error(msg, &err, &dbg_info);
        printf("Error received: %s \n", err->message);
        printf("Error from: %s\n", GST_OBJECT_NAME(msg->src));
        printf("Debug info: %s\n", dbg_info);
        break;
      default:
        printf("This should not print\n");
    }
  }
  gst_element_set_state(pipeline, GST_STATE_NULL);
}
