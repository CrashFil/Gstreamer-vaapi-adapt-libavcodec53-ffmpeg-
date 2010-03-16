/*
 *  gstvaapivideosink.h - VA sink interface
 *
 *  gstreamer-vaapi (C) 2010 Splitted-Desktop Systems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef GST_VAAPI_VIDEO_SINK_H
#define GST_VAAPI_VIDEO_SINK_H

#include <gst/gstelement.h>
#include <gst/vaapi/gstvaapidisplay.h>

G_BEGIN_DECLS

#define GST_VAAPI_TYPE_VIDEO_SINK \
    (gst_vaapi_video_sink_get_type())

#define GST_VAAPI_VIDEO_SINK(obj)                               \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),                          \
                                GST_VAAPI_TYPE_VIDEO_SINK,      \
                                GstVaapiVideoSink))

#define GST_VAAPI_IS_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_VAAPI_TYPE_VIDEO_SINK))

#define GST_VAAPI_VIDEO_SINK_GET_INTERFACE(obj)                 \
    (G_TYPE_INSTANCE_GET_INTERFACE((obj),                       \
                                   GST_VAAPI_TYPE_VIDEO_SINK,   \
                                   GstVaapiVideoSinkInterface))

typedef struct _GstVaapiVideoSink               GstVaapiVideoSink; /* dummy */
typedef struct _GstVaapiVideoSinkInterface      GstVaapiVideoSinkInterface;

struct _GstVaapiVideoSinkInterface {
    /*< private >*/
    GTypeInterface g_iface;

    GstVaapiDisplay *(*get_display)(GstVaapiVideoSink *sink);
};

GType
gst_vaapi_video_sink_get_type(void);

GstVaapiDisplay *
gst_vaapi_video_sink_get_display(GstVaapiVideoSink *sink);

GstVaapiVideoSink *
gst_vaapi_video_sink_lookup(GstElement *element);

G_END_DECLS

#endif /* GST_VAAPI_VIDEO_SINK_H */