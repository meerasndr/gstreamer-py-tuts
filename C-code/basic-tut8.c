#include<gst/gst.h>
#include<gst/audio/audio.h>
#include <string.h>

#define CHUNK_SIZE 1024
#define SAMPLE_RATE 44100

typedef struct _CustomData{
  GstElement *app_source;
  GstElement *tee;
  GstElement *audio_queue;
  GstElement *audio_convert1;
  GstElement *audio_resample;
  GstElement *audio_sink;
  GstElement *video_queue;
  GstElement *audio_convert2;
  GstElement *visual;
  GstElement *video_convert;
  GstElement *video_sink;
  GstElement *app_queue;
  GstElement *app_sink;
  GstElement *pipeline;
  GMainLoop *main_loop;
  guint64 num_samples;
  gfloat a, b, c, d;
  guint sourceid;
} CustomData;

// Super-basic square wave
static gboolean pushdata(CustomData *data){
  GstBuffer *buffer;
  GstFlowReturn ret;
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
  raw = (gint16*)map.data;
  data->c += data->d;
  data->d -= data->c / 1000;
  freq = 1100 + 1000 * data->d;
  for(int i = 0; i < num_samples; i++){
    data->a += data->b;
    data->b -= data->a / freq;
    raw[i] = (gint16)(data->b);
  }
  gst_buffer_unmap (buffer, &map);
  data->num_samples += num_samples * 2;

  g_signal_emit_by_name(data->app_source, "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);

  if (ret != GST_FLOW_OK){
    return FALSE;
  }
  return TRUE;
}
// Sine wave
/*static gboolean pushdata(CustomData *data){
  GstBuffer *buffer;
  GstFlowReturn ret;
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
  raw = (gint16*)map.data;
  data->c += data->d;
  data->d -= data->c / 1000;
  freq = 1100 + 1000 * data->d;
  for(int i = 0; i < num_samples; i++){
    data->a += data->b;
    data->b -= data->a / freq;
    raw[i] = (gint16)(500 * data->a);
  }
  gst_buffer_unmap (buffer, &map);
  data->num_samples += num_samples;

  g_signal_emit_by_name(data->app_source, "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);

  if (ret != GST_FLOW_OK){
    return FALSE;
  }
  return TRUE;
}*/

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

static GstFlowReturn new_sample(GstElement *sink, CustomData *data){
  GstSample *sample;
  g_signal_emit_by_name(sink, "pull-sample", &sample);
  if(sample){
    g_print("*");
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }
  return GST_FLOW_ERROR;
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
  GstAudioInfo info;
  memset(&data, 0, sizeof(data));
  data.b = 1;
  data.d = 1;
  data.app_source = gst_element_factory_make("appsrc", "audio_source");
  data.tee = gst_element_factory_make("tee", "tee");
  data.audio_queue = gst_element_factory_make("queue", "audio_queue");
  data.audio_convert1 = gst_element_factory_make("audioconvert", "audio_convert1");
  data.audio_resample = gst_element_factory_make("audioresample", "audio_resample");
  data.audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");
  data.video_queue = gst_element_factory_make("queue", "video_queue");
  data.audio_convert2 = gst_element_factory_make("audioconvert", "audio_convert2");
  data.visual = gst_element_factory_make("wavescope", "visual");
  data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
  data.video_sink = gst_element_factory_make("autovideosink", "video_sink");
  data.app_queue = gst_element_factory_make("queue", "app_queue");
  data.app_sink = gst_element_factory_make("appsink", "app_sink");
  data.pipeline = gst_pipeline_new("test-pipeline");

if(!data.app_source || !data.tee || !data.audio_queue || !data.audio_convert1 || !data.audio_resample || !data.audio_sink || !data.video_queue || !data.audio_convert2 || !data.visual || !data.video_convert || !data.video_sink || !data.app_queue || !data.app_sink || !data.pipeline){
  g_printerr("All Elements could not be created \n");
  return -1;
}

  // configure wavescope visual
  g_object_set (data.visual, "shader", 0, "style", 0, NULL);

 //configure app_source caps
 gst_audio_info_set_format(&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
GstCaps *audio_caps = gst_audio_info_to_caps(&info);
g_object_set(data.app_source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
// configure app_source signal handler
g_signal_connect(data.app_source, "need-data", G_CALLBACK(start_feed), &data);
g_signal_connect(data.app_source, "enough-data", G_CALLBACK(stop_feed), &data);

 //configure app_sink
 //emit-signals needs to be set to true first
 g_object_set(data.app_sink, "emit-signals", TRUE, "caps", audio_caps, NULL);
 //configure app_sink signal handler
 g_signal_connect(data.app_sink, "new-sample", G_CALLBACK(new_sample), &data);
 gst_caps_unref(audio_caps);

// Link elements with Always pads
gst_bin_add_many(GST_BIN(data.pipeline), data.app_source, data.tee, data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink, data.video_queue, data.audio_convert2, data.visual, data.video_convert, data.video_sink, data.app_queue, data.app_sink, NULL);
gboolean link1 = gst_element_link_many(data.app_source, data.tee, NULL);
gboolean link2 = gst_element_link_many(data.audio_queue, data.audio_convert1, data.audio_resample, data.audio_sink, NULL);
gboolean link3 = gst_element_link_many(data.video_queue, data.audio_convert2, data.visual, data.video_convert, data.video_sink, NULL);
gboolean link4 = gst_element_link_many(data.app_queue, data.app_sink, NULL);

if(!link1 || !link2 || !link3 || !link4){
  g_printerr("Elements could not be linked \n");
  return -1;
}

//Manual linking with tee pads
GstPad *tee_audio_pad;
tee_audio_pad = gst_element_get_request_pad(data.tee, "src_%u");
GstPad *queue_audio_pad = gst_element_get_static_pad(data.audio_queue, "sink");

GstPad *tee_video_pad = gst_element_get_request_pad(data.tee, "src_%u");
GstPad *queue_video_pad = gst_element_get_static_pad(data.video_queue, "sink");

GstPad *tee_app_pad = gst_element_get_request_pad(data.tee, "src_%u");
GstPad *queue_app_pad = gst_element_get_static_pad(data.app_queue, "sink");
gst_pad_link(tee_audio_pad, queue_audio_pad);
gst_pad_link(tee_video_pad, queue_video_pad);
gst_pad_link(tee_app_pad, queue_app_pad);

gst_object_unref(queue_audio_pad);
gst_object_unref(queue_video_pad);
gst_object_unref(queue_app_pad);


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


// Freeing resources
gst_element_release_request_pad (data.tee, tee_audio_pad);
gst_element_release_request_pad (data.tee, tee_video_pad);
gst_element_release_request_pad (data.tee, tee_app_pad);
gst_object_unref (tee_audio_pad);
gst_object_unref (tee_video_pad);
gst_object_unref (tee_app_pad);
gst_element_set_state(data.pipeline, GST_STATE_NULL);
gst_object_unref(data.pipeline);

return 0;
}
