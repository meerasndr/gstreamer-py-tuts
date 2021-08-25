#include<gst/gst.h>

typedef struct CustomData{
  GstElement *playbin;
  gboolean playing;
  gboolean terminate;
  gboolean seek_enabled;
  gboolean seek_done;
  gint64 duration;
} CustomData;

void handle_message(GstMessage *msg, CustomData *data){
  GError *err = NULL;
  gchar *dbg_info = NULL;
  switch(GST_MESSAGE_TYPE(msg)){
    case GST_MESSAGE_EOS:
      g_print("End of stream reached.\n");
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(msg, &err, &dbg_info);
      g_print("Error received: %s \n", err->message);
      g_print("Error from: %s\n", GST_OBJECT_NAME(msg->src));
      g_print("Debug info: %s\n", dbg_info);
      data -> terminate = TRUE;
      break;
    case GST_MESSAGE_DURATION_CHANGED:
      data -> duration = GST_CLOCK_TIME_NONE;
      break;
    case GST_MESSAGE_STATE_CHANGED:{
        GstState old_state, new_state;
        gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(&data -> playbin)){
        g_print("Pipeline state has changed from %s to %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));

        data -> playing = (new_state == GST_STATE_PLAYING);
        if(data -> playing){
          GstQuery *seekquery;
          seekquery = gst_query_new_seeking(GST_FORMAT_TIME);
          if(gst_element_query(data->playbin, seekquery)){
            gint64 start, end;
            gst_query_parse_seeking(seekquery, NULL, &data -> seek_enabled, &start, &end);
            if(data->seek_enabled){
              g_print("Seeking is enabled: from %" GST_TIME_FORMAT " to %u" GST_TIME_FORMAT "\n", GST_TIME_ARGS(start), GST_TIME_ARGS(end));
          } else{
            g_print("Seeking is disabled for this stream \n");
          }
        }
      }
    }
    break;
  }
    default:
      g_printerr("This should not be reached \n");
    }
}


int main(int argc, char *argv[]){
  CustomData data;
  GstBus *bus;
  GstMessage *msg;

  data.playing = FALSE;
  data.terminate = FALSE;
  data.seek_enabled = FALSE;
  data.seek_done = FALSE;
  data.duration = GST_CLOCK_TIME_NONE;
  gst_init(&argc, &argv);
  data.playbin = gst_element_factory_make("playbin", "playbin");

  if(!data.playbin){
    g_printerr("Not all elements could be created");
    return -1;
  }

  g_object_set(data.playbin, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);

  GstStateChangeReturn ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
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
          ret = gst_element_query_position(data.playbin, GST_FORMAT_TIME, &curr);

          if(!ret){
            g_printerr("Could not query current position \n");
          }
          if(data.duration != GST_CLOCK_TIME_NONE){
            ret = gst_element_query_duration(data.playbin, GST_FORMAT_TIME, &data.duration);
            if(!ret){
              g_printerr("Could not query duration \n");
            }
          }
          g_print("Current position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r", GST_TIME_ARGS(curr), GST_TIME_ARGS(data.duration));


        if(data.seek_enabled && !data.seek_done && curr > 10 * GST_SECOND){
          g_print("\nReached 10s, performing seek ... \n");
          if(gst_element_seek_simple(data.playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND)){
            data.seek_done = TRUE;
          }
        }
        }
    }
  } while(!data.terminate);

  gst_object_unref(bus);
  gst_element_set_state(data.playbin, GST_STATE_NULL);
  gst_object_unref(data.playbin);
  return 0;
}
