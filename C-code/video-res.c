// Compilation: gcc video-res.c -o video-res `pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0`
#include<gst/gst.h>
#include<gst/video/video.h>
#include<string.h>
#include<stdio.h>

#define CHUNK_SIZE1 1024 * 768 * 3
#define CHUNK_SIZE2 640 * 480 * 3
/* width * height * num of planes (3 for R, G and B) */
#define FRAMES_PER_SECOND 30

typedef struct _CustomData {
  GstElement *appsrc;
  GstElement *videoconvert;
  GstElement *vp8enc;
  GstElement *matromux;
  GstElement *filesink;
  GstElement *pipeline;
  GMainLoop *main_loop;
  GstClockTime buftimestamp;
  int framecount;
  guint sourceid;
  GstVideoInfo info;
} CustomData;

static gboolean pushdata(CustomData *data){
  GstBuffer *buffer;
  GstVideoFrame frame;
  GstMapInfo map;
  gint16 *raw;
  gfloat freq;
  GstFlowReturn ret;

  // resolution change
  if(data->framecount  ==  100){
    gst_video_info_init(&(data->info));
    GstCaps *video_caps2 = gst_caps_new_simple ("video/x-raw",
         "format", G_TYPE_STRING, "I420",
         "framerate", GST_TYPE_FRACTION, 30, 1,
         "width", G_TYPE_INT, 640,
         "height", G_TYPE_INT, 480,
         "colorimetry", G_TYPE_STRING, GST_VIDEO_COLORIMETRY_BT709,
         NULL);
    if(!gst_video_info_from_caps(&(data->info), video_caps2)){
      g_printerr("Could not parse caps to Gst video info\n");
    }
    g_object_set(data->appsrc, "caps", video_caps2, NULL);

    /*gst_video_info_set_format(&(data->info), GST_VIDEO_FORMAT_GBR, 640, 480);
    GstCaps *video_caps2 = gst_video_info_to_caps(&(data->info));
    g_object_set(data->appsrc, "caps", video_caps2, NULL);*/
  }
  if(data->framecount >= 100){
    buffer = gst_buffer_new_and_alloc(CHUNK_SIZE2);
  } else{
  buffer = gst_buffer_new_and_alloc(CHUNK_SIZE1);
}
  //Timestamp and duration for buffer
  GST_BUFFER_PTS(buffer) = data->buftimestamp;
  GST_BUFFER_DTS(buffer) = data->buftimestamp;
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, FRAMES_PER_SECOND);
  data->buftimestamp  += gst_util_uint64_scale_int(1, GST_SECOND ,FRAMES_PER_SECOND);
  data->framecount += 1;

  // Generating video
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  gst_video_frame_map_id (&frame, &(data->info), buffer, -1, GST_MAP_WRITE);
  for(int p = 0; p < GST_VIDEO_FRAME_N_PLANES(&frame); p++){
    g_print("H %d W %d\n", GST_VIDEO_INFO_HEIGHT(&(data->info)), GST_VIDEO_INFO_WIDTH(&(data->info)));

    if(gst_video_frame_map(&frame, &(data->info), buffer, GST_MAP_READ)){
      guint8 *pixels = GST_VIDEO_FRAME_PLANE_DATA (&frame, p);
      guint stride = GST_VIDEO_FRAME_PLANE_STRIDE (&frame, p);
      guint pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE (&frame, p);
      guint h, w;
       for (h = 0; h < GST_VIDEO_INFO_HEIGHT(&(data->info)); h++) {
         for (w = 0; w < GST_VIDEO_INFO_WIDTH(&(data->info)); w++) {
           guint8 *pixel = pixels + h * stride + w * pixel_stride;
           if(p != 2){
             memset (pixel, 0, pixel_stride);
           }
           else{
             memset(pixel, 255, pixel_stride);
           }
       }
     }
   }
 }
  gst_video_frame_unmap (&frame);
  gst_buffer_unmap (buffer, &map);
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
  memset(&data, 0, sizeof(data)); // Set a region of memory to a certain value? Requires string.h
  data.appsrc = gst_element_factory_make("appsrc", "video_source");
  data.videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  data.vp8enc = gst_element_factory_make("vp8enc", "vp8enc");
  data.matromux = gst_element_factory_make("matroskamux", "matroskamux");
  data.filesink = gst_element_factory_make("filesink", "filesink");
  data.pipeline = gst_pipeline_new("test-pipeline");
  gst_video_info_init(&(data.info));

  if(!data.appsrc || !data.videoconvert || !data.pipeline || !data.vp8enc || !data.matromux || !data.filesink){
    g_printerr("All elements could not be created\n");
    return -1;
  }

  //configure appsrc caps
  //gst_video_info_set_format(&(data.info), GST_VIDEO_FORMAT_GBR, 640, 480);
  //GstCaps *video_caps1 = gst_video_info_to_caps(&(data.info));
  data.buftimestamp = 0; //start time for buffer
  data.framecount = 0;
  GstCaps *video_caps1 = gst_caps_new_simple ("video/x-raw",
       "format", G_TYPE_STRING, "I420",
       "framerate", GST_TYPE_FRACTION, 30, 1,
       "width", G_TYPE_INT, 1024,
       "height", G_TYPE_INT, 768,
       "colorimetry", G_TYPE_STRING, GST_VIDEO_COLORIMETRY_BT709,
       NULL);
  if(!gst_video_info_from_caps(&(data.info), video_caps1)){
    g_printerr("Could not parse caps to Gst video info\n");
  }
g_object_set(data.appsrc, "caps", video_caps1, "format", GST_FORMAT_TIME, NULL);

  // configure appsrc signal handler
  g_signal_connect(data.appsrc, "need-data", G_CALLBACK(start_feed), &data);
  g_signal_connect(data.appsrc, "enough-data", G_CALLBACK(stop_feed), &data);

  //set filesink output location
  g_object_set(data.filesink, "location", "vidoutput", NULL);

  // Add elements to be linked
  gst_bin_add_many(GST_BIN(data.pipeline), data.appsrc, data.videoconvert, data.vp8enc, data.matromux, data.filesink, NULL);
  if(!gst_element_link_many(data.appsrc, data.videoconvert, data.vp8enc, data.matromux, data.filesink, NULL)){
    g_printerr("Not all elements could be linked\n");
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
  gst_caps_unref(video_caps1);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);

  return 0;
}
