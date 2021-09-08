#include<gst/gst.h>
#include<gst/video/video.h>
#include<string.h>
#include<stdio.h>

#define CHUNK_SIZE 1024 * 768 * 3
/* width * height * num of planes (3 for R, G and B) */
//#define SAMPLE_RATE 44100
#define FRAMES_PER_SECOND 30

typedef struct _CustomData{
  GstElement *appsrc;
  GstElement *videoconvert;
  GstElement *autovideosink;
  GstElement *pipeline;
  GMainLoop *main_loop;
  guint64 num_samples;
  int height, width;
  guint sourceid;
  GstVideoInfo info;
} CustomData;

static gboolean pushdata(CustomData *data){
  GstBuffer *buffer;
  GstVideoFrame frame;
  GstMapInfo map;
  gint16 *raw;
  gint num_samples = CHUNK_SIZE / 2; // Each sample is 16 bits = 2 bytes
  gfloat freq;

  buffer = gst_buffer_new_and_alloc(CHUNK_SIZE); // 1024 * 768 * 3
  //Timestamp and duration for buffer
  //GST_BUFFER_TIMESTAMP(buffer) = gst_util_uint64_scale(data->num_samples, GST_SECOND, FRAMES_PER_SECOND);
  GST_BUFFER_DURATION(buffer) = GST_CLOCK_TIME_NONE;

  // Generating waveforms
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  gst_video_frame_map_id (&frame, &data -> info, buffer, -1, GST_MAP_WRITE);
  int p;

  for(p = 0; p < 2; p++){
    if(gst_video_frame_map(&frame, &data->info, buffer, GST_MAP_READ)){
      guint8 *pixels = GST_VIDEO_FRAME_PLANE_DATA (&frame, p);
       guint stride = GST_VIDEO_FRAME_PLANE_STRIDE (&frame, p);
       guint pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE (&frame, p);
       guint h, w;
       for (h = 0; h < data->height; h++) {
         for (w = 0; w < data->width; w++) {
           guint8 *pixel = pixels + h * stride + w * pixel_stride;
           if(p == 1){
             memset (pixel, 255, pixel_stride);
           } else{
             memset(pixel, 0, pixel_stride);
           }

       }
     }

   }
 }
 gst_video_frame_unmap (&frame);
  gst_buffer_unmap (buffer, &map);


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
  //GstVideoInfo info;
  memset(&data, 0, sizeof(data)); // Set a region of memory to a certain value? Requires string.h

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
gst_video_info_set_format(&data.info, GST_VIDEO_FORMAT_I420, 1024, 768);
GstCaps *video_caps = gst_video_info_to_caps(&data.info);
data.height = 768;
data.width = 1024;
/*GstCaps *video_caps = gst_caps_new_simple ("video/x-raw",
     "format", G_TYPE_STRING, "RGB",
     "framerate", GST_TYPE_FRACTION, 25, 1,
     "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
     "width", G_TYPE_INT, 1024,
     "height", G_TYPE_INT, 768,
     NULL);
if(!gst_video_info_from_caps(data.info, video_caps)){
  g_printerr("Could not parse caps to Gst video info\n");
}*/

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
