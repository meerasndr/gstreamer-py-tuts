#include<gst/gst.h>


int main(int argc, char * argv[]){
  gst_init(&argc, &argv);
  GstElementFactory *sourcefactory, *sinkfactory;
  GstElement *source, *sink, *pipeline;
  gboolean terminate = FALSE;
  GstBus* bus;
  GstMessage* msg;

  sourcefactory = gst_element_factory_find("audiotestsrc");
  sinkfactory = gst_element_factory_find("autoaudiosink");

  if(!sourcefactory || !sinkfactory){
    g_printerr("Not all element factories could be created\n");
    return -1;
  }
  //print_pad_templates_info(&sourcefactory);
  //print_pad_templates_info(&sinkfactory);

  source = sourcefactory.create("source");
  sink = sinkfactory.create("sink");
  pipeline = gst_pipeline_new("test-pipeline");

  if(!pipeline || !source || !sink){
    g_printerr("Not all elements could be created\n");
    return -1;
  }

  gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
  if(!gst_element_link(source, sink)){
    g_printerr("Elements could not be linked\n");
    return -1;
  }

  //print_pad_capabilities(sink, "sink");

  if(gst_element_set_state(pipeline, GST_STATE_PLAYING) != 0 ){
    g_printerr("Pipeline could not be set to playing state\n");
  }

  bus = gst_element_get_bus(pipeline);
  do {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS | GST_MESSAGE_ERROR | GST_MESSAGE_STATE_CHANGED);

    if(msg != NULL){
      GError *err = NULL;
      gchar *dbg_info = NULL;
      switch(GST_MESSAGE_TYPE(msg)){
        case GST_MESSAGE_EOS:
          g_print("End of stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_ERROR:
          gst_message_parse_error(msg, &err, &dbg_info);
          g_printerr("Error received: %s \n", err->message);
          g_printerr("Error from: %s\n", GST_OBJECT_NAME(msg->src));
          g_printerr("Debug info: %s\n", dbg_info);
          g_clear_error (&err);
          g_free (debug_info);
          terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)){
              GstState old_state, new_state, pending_state;
              gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
              g_print("\nPipeline state has changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
              //print_pad_capabilities(sink, "sink");
            }
            break;
        default:
          g_printerr("Unexpected msg received \n");
          break;
    }
    gst_message_unref(msg);
  }
}(while !terminate);

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  gst_object_unref(sourcefactory);
  gst_object_unref(sinkfactory);
  return 0;
}
