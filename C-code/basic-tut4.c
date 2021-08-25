#include<gst/gst.h>

typedef struct CustomData{
  GstElement playbin;
  gboolean playing;
  gboolean terminate;
  gboolean seek_enabled;
  gboolean seek_done;
  gint duration;
} CustomData;

void handle_message(){
  
}

int main(int argc, char *argv[]){
  CustomData data;
  GstBus *bus;
  GstMessage *msg;

  data.playbin = gst_element_factory_make("playbin", "playbin");
  data.playing = FALSE;
  data.terminate = FALSE;
  data.seek_enabled = FALSE;
  data.seek_done = FALSE;
  data.duration = GST_CLOCK_TIME_NONE;

  gst_init(&argc, &argv);

  if(!data.playbin){
    g_printerr("Not all elements could be created");
    return -1;
  }

  g_object_set(data.playbin, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm");

  gint ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
  if(ret == 0) {
    g_printerr("Could not change state of pipeline to playing\n");
    return -1;
  } else {
    g_print("URI property added\n");
  }

  bus = gst_element_get_bus(data.playbin);

  do {
    msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND, GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION_CHANGED);
    if(msg){
      handle_message(msg, &data);
    } else {
        if(data.playing){
          gboolean ret;
          gint64 curr;
          ret = gst_element_query_position(data.playbin, GST_FORMAT_TIME, curr);

          if(!ret){
            g_printerr("Could not query current position \n");
          }
          if(data.duration != GST_CLOCK_TIME_NONE){
            ret = gst_element_query_duration(data.playbin, GST_FORMAT_TIME, data.duration);
            if(!ret){
              g_printerr("Could not query duration \n");
            }
          }
          g_print("Current position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r", GST_TIME_ARGS(curr), GST_TIME_ARGS(data.duration));
        }

        if(data.seek_enabled && !data.seek_done && curr > 10 * GST_SECOND){
          g_print("\nReached 10s, performing seek ... \n");
          if(gst_element_seek_simple(data.playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND)){
            data.seek_done = TRUE;
          }

        }
    }
  } while(!data.terminate);

  gst_object_unref(bus);
  gst_element_set_state(data.playbin, GST_STATE_NULL);
  gst_object_unref(data.playbin);
  return 0;
}
