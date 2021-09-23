// Compilation: gcc video-res.c -o video-res `pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0`
#include<gst/gst.h>
#include<gst/video/video.h>
#include<string.h>
#include<stdio.h>
#include <gst/app/gstappsrc.h>

/* width * height * num of planes (3 for R, G and B) */
#define FRAMES_PER_SECOND 30

typedef struct _CustomData {
  GstElement *appsrc;
  GstElement *vp8enc;
  GstElement *matromux;
  GstElement *filesink;
  GstElement *pipeline;
  GMainLoop *main_loop;
  GstClockTime buftimestamp;
  int framecount;
  guint sourceid;
  int width;
  int height;
  GstVideoInfo info;
} CustomData;

static void setres(CustomData *data, char* format, int width, int height){
    gst_video_info_init(&(data->info));
    GstCaps *video_caps = gst_caps_new_simple ("video/x-raw",
         "format", G_TYPE_STRING, format,
         "framerate", GST_TYPE_FRACTION, 30, 1,
         "width", G_TYPE_INT, width,
         "height", G_TYPE_INT, height,
         "colorimetry", G_TYPE_STRING, GST_VIDEO_COLORIMETRY_BT601,
         NULL);
    if(!gst_video_info_from_caps(&(data->info), video_caps)){
      g_printerr("Could not parse caps to Gst video info\n");
    }
    g_object_set(data->appsrc, "caps", video_caps, "format", GST_FORMAT_TIME, NULL);
}

static gboolean pushdata(CustomData *data){
  GstBuffer *buffer;
  GstVideoFrame frame;
  GstMapInfo map;
  gint16 *raw;
  gfloat freq;
  GstFlowReturn ret;
  // resolution change every 100 frames
  // 0 - 99 1024x768 // 100 - 199 640x480 // 200 - 299 1024x768 .... and so on
 if(data->framecount % 200 >= 100 && data->framecount % 200 <= 199){
    setres(data, "I420", 1024, 768);
    buffer = gst_buffer_new_and_alloc(1024 * 768 * 3);
  } else if (data->framecount % 100 >= 0 && data->framecount % 100 <= 99){
    setres(data, "I420", 640, 480);
    buffer = gst_buffer_new_and_alloc(640 * 480 * 3);
  }
  //GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");

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
    g_print("Framecount %d H %d W %d\n", data->framecount, GST_VIDEO_INFO_HEIGHT(&(data->info)), GST_VIDEO_INFO_WIDTH(&(data->info)));
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(data->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline1");
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

  if(data->framecount == 299){ // this is == 10 seconds of video output
    GstFlowReturn ret2 = gst_app_src_end_of_stream(GST_APP_SRC(data->appsrc));
    if(ret2 == GST_FLOW_OK){
      gst_event_new_eos();
    }
  }
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
  if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR){
    GError *err;
    gchar *debug_info;
    gst_message_parse_error(msg, &err, &debug_info);
    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error (&err);
    g_free (debug_info);
  } else {
  g_print("EOS reached \n");
}
  g_main_loop_quit (data->main_loop);
}

int main(int argc, char *argv[]){
  gst_init(&argc, &argv);
  CustomData data;
  memset(&data, 0, sizeof(data)); // Set a region of memory to a certain value? Requires string.h
  data.appsrc = gst_element_factory_make("appsrc", "video_source");
  //data.videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
  data.vp8enc = gst_element_factory_make("vp8enc", "vp8enc");
  data.matromux = gst_element_factory_make("matroskamux", "matroskamux");
  data.filesink = gst_element_factory_make("filesink", "filesink");
  data.pipeline = gst_pipeline_new("test-pipeline");
  gst_video_info_init(&(data.info));

  if(!data.appsrc || !data.pipeline || !data.vp8enc || !data.matromux || !data.filesink){
    g_printerr("All elements could not be created\n");
    return -1;
  }

  //configure appsrc caps
  data.buftimestamp = 0; //start time for buffer
  data.framecount = 0;
  setres(&data, "I420", 1024, 768);

  // configure appsrc signal handler
  g_signal_connect(data.appsrc, "need-data", G_CALLBACK(start_feed), &data);
  g_signal_connect(data.appsrc, "enough-data", G_CALLBACK(stop_feed), &data);

  //set filesink output location
  g_object_set(data.filesink, "location", "vidoutput", NULL);

  // Add elements to be linked
  gst_bin_add_many(GST_BIN(data.pipeline), data.appsrc, data.vp8enc, data.matromux, data.filesink, NULL);
  if(!gst_element_link_many(data.appsrc, data.vp8enc, data.matromux, data.filesink, NULL)){
    g_printerr("Not all elements could be linked\n");
    return -1;
  }
  //Connect bus to error-handler callback
  //Signals to notify application of error events
  GstBus *bus = gst_element_get_bus(data.pipeline);
  gst_bus_add_signal_watch(bus);
  g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, &data);
  g_signal_connect(G_OBJECT(bus), "message::eos", (GCallback)error_cb, &data);
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
