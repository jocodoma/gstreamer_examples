/* GStreamer AppSink-AppSrc Example
 *
 * References:
 * https://searchcode.com/file/63275229/gst-plugins-base-0.10.28/tests/examples/app/appsink-src.c
 * https://fossies.org/linux/gst-plugins-base/tests/examples/app/appsink-src.c
 * https://gstreamer.freedesktop.org/documentation/tutorials/basic/short-cutting-the-pipeline.html
 *
 * Jocodoma
 * jocodoma@gmail.com
 * 
 */

#include <gst/gst.h>
#include <string.h>  // for memset()

#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappbuffer.h>


//const gchar *audioCaps = "audio/x-raw,format=S16LE,channels=2,rate=44100, layout=interleaved";
const gchar *audioCaps = "audio/x-raw-int, endianness=(int)1234, channels=(int)1, width=(int)16, depth=(int)16, signed=(boolean)true, rate=(int)48000";

typedef struct _CustomData
{
    GstElement *sourcePipeline;
    GstElement *sinkPipeline;
    GMainLoop  *mainLoop;
}CustomData;

/* This signal callback is triggered when the new buffer is ready at appsink for processing */
static void on_new_buffer_from_appsink(GstElement *myAppSink, CustomData *data)
{
    guint size;
    gpointer rawBuffer;
    GstBuffer *audioBuffer, *buffer;
    GstElement *myAppSource;

    /* Get the buffer from appsink */
    buffer = gst_app_sink_pull_buffer(GST_APP_SINK(myAppSink));

    /* turn it into an app buffer, it's not really needed, we could simply push
     * the retrieved buffer from appsink into appsrc just fine.  */
    size = GST_BUFFER_SIZE (buffer);
    // g_print ("Pushing a buffer of size %d\n", size);
    rawBuffer = g_malloc0 (size);
    memcpy (rawBuffer, GST_BUFFER_DATA (buffer), size);
    audioBuffer = gst_app_buffer_new (rawBuffer, size, g_free, rawBuffer);

    /* newer basesrc will set caps for use automatically but it does not really
     * hurt to set it on the buffer again */
    gst_buffer_set_caps (audioBuffer, GST_BUFFER_CAPS (buffer));

    gst_buffer_unref (buffer);

    /* Get source an push new buffer */
    myAppSource = gst_bin_get_by_name(GST_BIN(data->sinkPipeline), "myAppSourceElement");
    gst_app_src_push_buffer (GST_APP_SRC (myAppSource), audioBuffer);
    gst_object_unref(myAppSource);
}

/* This signal callback is called when new message arrives from sourcePipeline */
static gboolean on_source_message(GstBus *bus, GstMessage *message, CustomData *data)
{
    GstElement *myAppSource;

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            g_print("The source got dry\n");
            myAppSource = gst_bin_get_by_name(GST_BIN(data->sinkPipeline), "myAppSourceElement");
            gst_app_src_end_of_stream(GST_APP_SRC(myAppSource));  // send EOS to AppSrc
            gst_object_unref(myAppSource);
            break;

        case GST_MESSAGE_ERROR:
            g_print("Received error\n");
            g_main_loop_quit(data->mainLoop);
            break;

        default:
            break;
    }

    return TRUE;
}

/* This signal callback is called when new message arrives from sinkPipeline */
static gboolean on_sink_message(GstBus *bus, GstMessage *message, CustomData *data)
{
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            g_print("Finished playback\n");
            g_main_loop_quit(data->mainLoop);
            break;

        case GST_MESSAGE_ERROR:
            g_print("Received error\n");
            g_main_loop_quit(data->mainLoop);
            break;

        default:
            break;
    }

    return TRUE;
}

int main(int argc, char *argv[])
{
    gchar *fileName = NULL;
    CustomData *data = NULL;
    gchar *stringPipeline = NULL;
    GstBus *bus = NULL;
    GstElement *myAppSink = NULL;
    GstElement *myAppSource = NULL;

    gst_init(&argc, &argv);

    fileName = g_strdup("../../mp3Files/transcript.wav");

    if(!g_file_test(fileName, G_FILE_TEST_EXISTS))
    {
        g_print("File %s does not exist\n", fileName);
        return -1;
    }

    data = g_new0(CustomData, 1);
    data->mainLoop = g_main_loop_new(NULL, FALSE);

    /* First pipeline to read audio data from a file and push it to appsink */
    stringPipeline = g_strdup_printf("filesrc location=\"%s\" \
        ! decodebin ! audioconvert ! audioresample \
        ! appsink caps=\"%s\" name=myAppSinkElement", fileName, audioCaps);
    g_free(fileName);
    data->sourcePipeline = gst_parse_launch(stringPipeline, NULL);
    g_free(stringPipeline);

    if (data->sourcePipeline == NULL) {
        g_print ("Bad source\n");
        return -1;
    }

    // -v, --verbose, output status information and property notifications
    g_signal_connect(data->sourcePipeline, "deep_notify", G_CALLBACK(gst_object_default_deep_notify), NULL);

    /* Handle message, including EOS */
    bus = gst_element_get_bus(data->sourcePipeline);
    gst_bus_add_watch(bus, (GstBusFunc)on_source_message, data);
    gst_object_unref(bus);

    /* appsink push mode
     * Send a signal when data is available and pull it out in the signal callback
     * Use the sync=false to push the data as fast as it can */
    myAppSink = gst_bin_get_by_name(GST_BIN (data->sourcePipeline), "myAppSinkElement");
    g_object_set (G_OBJECT(myAppSink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(myAppSink, "new-buffer", G_CALLBACK(on_new_buffer_from_appsink), data);
    gst_object_unref(myAppSink);

    /* Second pipeline is to push audio data in and play it back using the autoaudiosink
     * No blocking on the src, meaning that we will push the entire file into memory. */
    stringPipeline = g_strdup_printf("appsrc name=myAppSourceElement caps=\"%s\" \
        ! autoaudiosink", audioCaps);
    data->sinkPipeline = gst_parse_launch(stringPipeline, NULL);
    g_free(stringPipeline);

    if (data->sinkPipeline == NULL) {
        g_print("Bad sink\n");
        return -1;
    }

    myAppSource = gst_bin_get_by_name(GST_BIN (data->sinkPipeline), "myAppSourceElement");
    /* configure for time-based format */
    g_object_set(myAppSource, "format", GST_FORMAT_TIME, NULL);
    /* Below to block when appsrc has buffered enough */
    // g_object_set (myAppSource, "block", TRUE, NULL);
    gst_object_unref(myAppSource);

    // -v, --verbose, output status information and property notifications
    g_signal_connect(data->sinkPipeline, "deep_notify", G_CALLBACK(gst_object_default_deep_notify), NULL);

    bus = gst_element_get_bus(data->sinkPipeline);
    gst_bus_add_watch(bus, (GstBusFunc)on_sink_message, data);
    gst_object_unref(bus);

    /* launching things */
    gst_element_set_state(data->sinkPipeline, GST_STATE_PLAYING);
    gst_element_set_state(data->sourcePipeline, GST_STATE_PLAYING);

    /* let's run !, this loop will quit when the sink pipeline goes EOS or when an
    * error occurs in the source or sink pipelines. */
    g_print("Let's run!\n");
    g_main_loop_run(data->mainLoop);
    g_print("Going out\n");

    gst_element_set_state(data->sourcePipeline, GST_STATE_NULL);
    gst_element_set_state(data->sinkPipeline, GST_STATE_NULL);

    gst_object_unref(data->sourcePipeline);
    gst_object_unref(data->sinkPipeline);
    g_main_loop_unref(data->mainLoop);
    g_free(data);

    return 0;
}
