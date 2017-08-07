/*
 * Gstreamer 101 Hello World Example
 *
 * References:
 * https://gstreamer.freedesktop.org/documentation/tutorials/basic/hello-world.html
 * https://gstreamer.freedesktop.org/documentation/tutorials/basic/short-cutting-the-pipeline.html
 *
 * Jocodoma
 * jocodoma@gmail.com
 *
 */

#include <gst/gst.h>
#include "string.h"  // for memset()


// Structure to contain all our information, so we can pass it to callbacks
typedef struct _CustomData
{
    GstElement *pipeline;  // The running pipeline
    GMainLoop *mainLoop;   // GLib Main Loop
}CustomData;


// This function is called when a message is posted on the bus
static void cb_message(GstBus *bus, GstMessage *msg, CustomData *data)
{
    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_ERROR:  // error message
        {
            GError *err;
            gchar *debug_info;

            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("[ERROR]: Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("[ERROR]: Debugging information: %s\n", debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);

            gst_element_set_state(data->pipeline, GST_STATE_READY);
            g_main_loop_quit(data->mainLoop);
            break;
        }

        case GST_MESSAGE_EOS:  // end-of-stream
            g_print("[INFO]: EOS is detected!\n");
            gst_element_set_state(data->pipeline, GST_STATE_READY);
            g_main_loop_quit(data->mainLoop);
            break;

        case GST_MESSAGE_CLOCK_LOST:
            /* Get a new clock */
            gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
            gst_element_set_state(data->pipeline, GST_STATE_PLAYING);
            break;

        default:  // Unhandled message
            break;
    }
}

int main(int argc, char *argv[])
{
    CustomData data;
    GstBus *bus;
    GError *error = NULL;

    // Initialize cumstom data structure
    memset(&data, 0, sizeof(data));

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Build the pipeline
    data.pipeline = gst_parse_launch("filesrc location=../../mp3Files/heyJude.mp3 \
        ! decodebin ! audioconvert ! autoaudiosink", &error);

    if(error)
    {
        g_printerr("[ERROR]: Unable to build pipeline: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    // -v, --verbose, output status information and property notifications
    g_signal_connect(data.pipeline, "deep_notify", G_CALLBACK(gst_object_default_deep_notify), NULL);

    // Instruct the bus to emit signals for each received message, and connect to the interesting signals
    bus = gst_element_get_bus(data.pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(cb_message), &data);
    gst_object_unref(bus);

    // Start playing
    g_print("[INFO]: Starting pipeline ... \n");
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    // Create a GLib Main Loop and set it to run
    data.mainLoop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.mainLoop);

    // Clean up
    g_print("[INFO]: Nulling pipeline ... \n");
    g_main_loop_unref(data.mainLoop);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}
