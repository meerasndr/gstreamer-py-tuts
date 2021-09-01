#include<gst/gst.h>
#include<gst/audio/audio.h>

#define SAMPLE_RATE 44100
#define CHUNK_SIZE 1024

typedef struct CustomData{
  GstElement *app_source;
  GstElement *tee;
  GstElement *audio_queue;
  GstElement *audio_convert1;
  GstElement *audio_resample;
  GstElement *audio_sink;
  GstElement *video_queue;
  GstElement *audioconvert2;
  GstElement *visual;
  GstElement *video_convert;
  GstElement *video_sink;
  GstElement *app_queue;
  GstElement *app_sink;
  GstElement *pipeline;
  GMainLoop *main_loop
  guint64 num_samples;
  gfloat a, b, c, d;
  guint sourceid;
} CustomData;

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

static gboolean new_sample(GstElement *sink, CustomData *data){
  GstSample *sample;
  g_signal_emit_by_name(sink, "pull-sample", &sample);
  if(sample){
    g_print("*");
    gst_sample_unref(sample);
    return TRUE;
  }
  return FALSE;
}


int main(int argc, char *argv[]){
  gst_init(&argc, &argv);
  CustomData data;
  GstAudioInfo info;
  data.app_source = gst_element_factory_make("appsrc", "audio_source");
  data.tee = gst_element_factory_make("tee", "tee");
  data.audio_queue = gst_element_factory_make("queue", "audio_queue");
  data.audio_convert1 = gst_element_factory_make("audioconvert", "audio_convert1");
  data.audio_resample = gst_element_factory_make("audioresample", "audio_resample");
  data.audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");
  data.video_queue = gst_element_factory_make("queue", "video_queue");
  data.audio_convert2 = gst_element_factory_make("audioconvert", "audio_convert2");
  data.visual = gst_element_factory_make("waveform", "visual");
  data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
  data.video_sink = gst_element_factory_make("autovideosink", "video_sink");
  data.app_queue = gst_element_factory_make("queue", "app_queue");
  data.app_sink = gst_element_factory_make("appsink", "app_sink");
  data.pipeline = gst_pipeline_new("test-pipeline");

if(!data.app_source || !data.tee || !data.audio_queue || !data.audio_convert1 || !data.audio_resample || !data.audio_sink || !data.video_queue || !data.audio_convert2 || !data.visual || !data.video_convert2 || !data.video_sink || !data.app_queue || !data.app_sink || !data.pipeline){
  g_printerr("All Elements could not be created \n")
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

if(link1 || link2 || link3 || link4){
  g_printerr("Elements could not be linked \n");
  return -1;
}

return 0;
}
