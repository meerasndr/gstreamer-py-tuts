#include<gst/gst.h>
#include<gst/video/video.h>
#include<string.h>

#define CHUNK_SIZE 2048
#define SAMPLE_RATE 44100

typedef struct _CustomData{
  GstElement *appsrc;
  GstElement *videoconvert;
  GstElement *autovideosink;
  GstElement *pipeline;
  GMainLoop *main_loop;
  guint64 num_samples;
  gfloat a, b, c, d;
  guint sourceid;
} CustomData;

static gboolean pushdata(CustomData *data){
  GstBuffer *buffer;
  GstVideoFrame *frame;
  GstMapInfo map;
  gint16 *raw;
  gint num_samples = CHUNK_SIZE / 2; // Each sample is 16 bits = 2 bytes
  gfloat freq;

  buffer = gst_buffer_new_and_alloc(CHUNK_SIZE);
  //Timestamp and duration for buffer
  GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(data->num_samples, GST_SECOND, SAMPLE_RATE);
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(num_samples, GST_SECOND, SAMPLE_RATE);

  // Generating waveforms
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  //gst_video_frame_map(frame, data->info, buffer, GST_MAP_WRITE);
  raw = (gint16*)map.data;
  data->c += data->d;
  data->d -= data->c / 1000;
  freq = 1100 + 1000 * data->d;
  for(int i = 0; i < num_samples; i++){
    data->a += data->b;
    data->b -= data->a / freq;
    raw[i] = (gint16)(5000 * data->a);
  }
  gst_buffer_unmap (buffer, &map);
  data->num_samples += num_samples;

  GstFlowReturn ret;
  g_signal_emit_by_name(data->appsrc, "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);

  if (ret != GST_FLOW_OK){
    return FALSE;
  }
  return TRUE;
}

static void start_feed(GstElement *source, guint size, CustomData *data){
  if(data->sourceid == 0){
    g_print("Start data feed\n");
    data->sourceid = g_idle_add((GSourceFunc)pushdata, data); //g_idle_add returns a guint? The ID of the event source
  }
}

static void stop_feed(GstElement *source, CustomData *data){
  if(data->sourceid != 0){
    g_print("Stop data feed\n");
    g_source_remove(data->sourceid); //g_source_remove removed the source with given ID from the default main context
    data->sourceid = 0;
  }
}

static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data){
  GError *err;
  gchar *debug_info;

  gst_message_parse_error(msg, &err, &debug_info);
  g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
  g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error (&err);
  g_free (debug_info);

  g_main_loop_quit (data->main_loop);
}

int main(int argc, char *argv[]){
  gst_init(&argc, &argv);
  CustomData data;
  GstVideoInfo info;
  memset(&data, 0, sizeof(data)); // Set a region of memory to a certain value? Requires string.h
  data.b = 1;
  data.d = 1;
  data.appsrc = gst_element_factory_make("appsrc", "video_source");
  data.videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  data.autovideosink = gst_element_factory_make("autovideosink", "autovideosink");
  data.pipeline = gst_pipeline_new("test-pipeline");

  //gst_video_info_init (data.info);

if(!data.appsrc || !data.videoconvert || !data.autovideosink || !data.pipeline){
  g_printerr("All elements could not be created\n");
  return -1;
}

//configure appsrc caps
gst_video_info_set_format(&info, GST_VIDEO_FORMAT_RGB, 1024, 768);
GstCaps *video_caps = gst_video_info_to_caps(&info);
g_object_set(data.appsrc, "caps", video_caps, "format", GST_FORMAT_TIME, NULL);

// configure appsrc signal handler
g_signal_connect(data.appsrc, "need-data", G_CALLBACK(start_feed), &data);
g_signal_connect(data.appsrc, "enough-data", G_CALLBACK(stop_feed), &data);

// Add elements to be linked
gst_bin_add_many(GST_BIN(data.pipeline), data.appsrc, data.videoconvert, data.autovideosink, NULL);
if(!gst_element_link_many(data.appsrc, data.videoconvert, data.autovideosink, NULL)){
  g_printerr("Not all elements could be linked");
  return -1;
}

//Connect bus to error-handler callback
//Signals to notify application of error events
GstBus *bus = gst_element_get_bus(data.pipeline);
gst_bus_add_signal_watch(bus);
g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, &data);
gst_object_unref(bus);

//Start playing pipeline
gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

//GLib main loop
data.main_loop = g_main_loop_new(NULL, FALSE);
g_main_loop_run(data.main_loop);

// free resources
gst_element_set_state(data.pipeline, GST_STATE_NULL);
gst_object_unref(data.pipeline);

return 0;
}
