/*
 *  gstvaapiprofile.c - VA profile abstraction
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

/**
 * SECTION:gstvaapiprofile
 * @short_description: VA profile abstraction
 */

#include "config.h"
#include <string.h>
#include <gst/gstbuffer.h>
#include "gstvaapicompat.h"
#include "gstvaapiprofile.h"

#define GST_VAAPI_PROFILE_CODEC(profile) \
    ((GstVaapiCodec)(((guint32)profile) & GST_MAKE_FOURCC(0xff,0xff,0xff,0)))

typedef struct _GstVaapiProfileMap              GstVaapiProfileMap;
typedef struct _GstVaapiEntrypointMap           GstVaapiEntrypointMap;

struct _GstVaapiProfileMap {
    GstVaapiProfile             profile;
    VAProfile                   va_profile;
    const char                 *caps_str;
};

struct _GstVaapiEntrypointMap {
    GstVaapiEntrypoint          entrypoint;
    VAEntrypoint                va_entrypoint;
};

/* Profiles */
static const GstVaapiProfileMap gst_vaapi_profiles[] = {
    { GST_VAAPI_PROFILE_MPEG2_SIMPLE, VAProfileMPEG2Simple,
      "video/mpeg, mpegversion=2, profile=simple"
    },
    { GST_VAAPI_PROFILE_MPEG2_MAIN, VAProfileMPEG2Main,
      "video/mpeg, mpegversion=2, profile=main"
    },
    { GST_VAAPI_PROFILE_MPEG4_SIMPLE, VAProfileMPEG4Simple,
      "video/mpeg, mpegversion=4, profile=simple"
    },
    { GST_VAAPI_PROFILE_MPEG4_ADVANCED_SIMPLE, VAProfileMPEG4AdvancedSimple,
      "video/mpeg, mpegversion=4, profile=advanced-simple"
    },
    { GST_VAAPI_PROFILE_MPEG4_MAIN, VAProfileMPEG4Main,
      "video/mpeg, mpegversion=4, profile=main"
    },
#if VA_CHECK_VERSION(0,30,0)
    { GST_VAAPI_PROFILE_H263_BASELINE, VAProfileH263Baseline,
      "video/x-h263, variant=itu, h263version=h263, profile=baseline"
    },
#endif
    { GST_VAAPI_PROFILE_H264_BASELINE, VAProfileH264Baseline,
      "video/x-h264, variant=itu, profile=baseline"
    },
    { GST_VAAPI_PROFILE_H264_MAIN, VAProfileH264Main,
      "video/x-h264, variant=itu, profile=main"
    },
    { GST_VAAPI_PROFILE_H264_HIGH, VAProfileH264High,
      "video/x-h264, variant=itu, profile=high"
    },
    { GST_VAAPI_PROFILE_VC1_SIMPLE, VAProfileVC1Simple,
      "video/x-vc1, profile=simple"
    },
    { GST_VAAPI_PROFILE_VC1_MAIN, VAProfileVC1Main,
      "video/x-vc1, profile=main"
    },
    { GST_VAAPI_PROFILE_VC1_ADVANCED, VAProfileVC1Advanced,
      "video/x-vc1, profile=advanced"
    },
    { 0, }
};

/* Entry-points */
static const GstVaapiEntrypointMap gst_vaapi_entrypoints[] = {
    { GST_VAAPI_ENTRYPOINT_VLD,         VAEntrypointVLD         },
    { GST_VAAPI_ENTRYPOINT_IDCT,        VAEntrypointIDCT        },
    { GST_VAAPI_ENTRYPOINT_MOCO,        VAEntrypointMoComp      },
    { 0, }
};

static const GstVaapiProfileMap *
get_profiles_map(GstVaapiProfile profile)
{
    const GstVaapiProfileMap *m;

    for (m = gst_vaapi_profiles; m->profile; m++)
        if (m->profile == profile)
            return m;
    return NULL;
}

static const GstVaapiEntrypointMap *
get_entrypoints_map(GstVaapiEntrypoint entrypoint)
{
    const GstVaapiEntrypointMap *m;

    for (m = gst_vaapi_entrypoints; m->entrypoint; m++)
        if (m->entrypoint == entrypoint)
            return m;
    return NULL;
}

/**
 * gst_vaapi_profile:
 * @profile: a #VAProfile
 *
 * Converts a VA profile into the corresponding #GstVaapiProfile. If
 * the profile cannot be represented by #GstVaapiProfile, then zero is
 * returned.
 *
 * Return value: the #GstVaapiProfile describing the @profile
 */
GstVaapiProfile
gst_vaapi_profile(VAProfile profile)
{
    const GstVaapiProfileMap *m;

    for (m = gst_vaapi_profiles; m->profile; m++)
        if (m->va_profile == profile)
            return m->profile;
    return 0;
}

/**
 * gst_vaapi_profile_from_codec_data:
 * @codec: a #GstVaapiCodec
 * @buffer: a #GstBuffer holding code data
 *
 * Tries to parse VA profile from @buffer data and @codec information.
 *
 * Return value: the #GstVaapiProfile described in @buffer
 */
static GstVaapiProfile
gst_vaapi_profile_from_codec_data_h264(GstBuffer *buffer)
{
    /* MPEG-4 Part 15: Advanced Video Coding (AVC) file format */
    uint8_t * const buf = GST_BUFFER_DATA(buffer);

    if (buf[0] != 1)    /* configurationVersion = 1 */
        return 0;

    switch (buf[1]) {   /* AVCProfileIndication */
    case 66:    return GST_VAAPI_PROFILE_H264_BASELINE;
    case 77:    return GST_VAAPI_PROFILE_H264_MAIN;
    case 100:   return GST_VAAPI_PROFILE_H264_HIGH;
    }
    return 0;
}

static GstVaapiProfile
gst_vaapi_profile_from_codec_data(GstVaapiCodec codec, GstBuffer *buffer)
{
    GstVaapiProfile profile;

    if (!codec || !buffer)
        return 0;

    switch (codec) {
    case GST_VAAPI_CODEC_H264:
        profile = gst_vaapi_profile_from_codec_data_h264(buffer);
        break;
    default:
        profile = 0;
        break;
    }
    return profile;
}

/**
 * gst_vaapi_profile_from_caps:
 * @caps: a #GstCaps
 *
 * Converts @caps into the corresponding #GstVaapiProfile. If the
 * profile cannot be represented by #GstVaapiProfile, then zero is
 * returned.
 *
 * Return value: the #GstVaapiProfile describing the @caps
 */
GstVaapiProfile
gst_vaapi_profile_from_caps(GstCaps *caps)
{
    const GstVaapiProfileMap *m;
    GstCaps *caps_test;
    GstStructure *structure;
    GstVaapiProfile profile;
    GstBuffer *codec_data = NULL;
    const gchar *name;
    gsize namelen;

    if (!caps)
        return 0;

    structure = gst_caps_get_structure(caps, 0);
    if (!structure)
        return 0;

    name    = gst_structure_get_name(structure);
    namelen = strlen(name);

    if (!gst_structure_has_field(structure, "profile")) {
        const GValue *v_codec_data;
        v_codec_data = gst_structure_get_value(structure, "codec_data");
        if (v_codec_data)
            codec_data = gst_value_get_buffer(v_codec_data);
    }

    profile = 0;
    for (m = gst_vaapi_profiles; !profile && m->profile; m++) {
        if (strncmp(name, m->caps_str, namelen) != 0)
            continue;
        caps_test = gst_caps_from_string(m->caps_str);
        if (gst_caps_is_always_compatible(caps, caps_test))
            profile = m->profile;
        else if (codec_data)
            profile = gst_vaapi_profile_from_codec_data(
                GST_VAAPI_PROFILE_CODEC(m->profile),
                codec_data
            );
        gst_caps_unref(caps_test);
    }
    return profile;
}

/**
 * gst_vaapi_profile_get_va_profile:
 * @profile: a #GstVaapiProfile
 *
 * Converts a #GstVaapiProfile into the corresponding VA profile. If
 * no matching VA profile was found, -1 is returned and this error
 * must be reported to be fixed.
 *
 * Return value: the VA profile, or -1 if none was found
 */
VAProfile
gst_vaapi_profile_get_va_profile(GstVaapiProfile profile)
{
    const GstVaapiProfileMap * const m = get_profiles_map(profile);

    return m ? m->va_profile : (VAProfile)-1;
}

/**
 * gst_vaapi_profile_get_caps:
 * @profile: a #GstVaapiProfile
 *
 * Converts a #GstVaapiProfile into the corresponding #GstCaps. If no
 * matching caps were found, %NULL is returned.
 *
 * Return value: the newly allocated #GstCaps, or %NULL if none was found
 */
GstCaps *
gst_vaapi_profile_get_caps(GstVaapiProfile profile)
{
    const GstVaapiProfileMap * const m = get_profiles_map(profile);

    return m ? gst_caps_from_string(m->caps_str) : NULL;
}

/**
 * gst_vaapi_profile_get_codec:
 * @profile: a #GstVaapiProfile
 *
 * Extracts the #GstVaapiCodec from @profile.
 *
 * Return value: the #GstVaapiCodec from @profile
 */
GstVaapiCodec
gst_vaapi_profile_get_codec(GstVaapiProfile profile)
{
    return GST_VAAPI_PROFILE_CODEC(profile);
}

/**
 * gst_vaapi_entrypoint:
 * @entryprofile: a #VAEntrypoint
 *
 * Converts a VA entry-point into the corresponding #GstVaapiEntrypoint.
 * If the entry-point cannot be represented by #GstVaapiEntrypoint,
 * then zero is returned.
 *
 * Return value: the #GstVaapiEntrypoint describing the @entrypoint
 */
GstVaapiEntrypoint
gst_vaapi_entrypoint(VAEntrypoint entrypoint)
{
    const GstVaapiEntrypointMap *m;

    for (m = gst_vaapi_entrypoints; m->entrypoint; m++)
        if (m->va_entrypoint == entrypoint)
            return m->entrypoint;
    return 0;
}

/**
 * gst_vaapi_entrypoint_get_va_entrypoint:
 * @entrypoint: a #GstVaapiEntrypoint
 *
 * Converts a #GstVaapiEntrypoint into the corresponding VA
 * entry-point. If no matching VA entry-point was found, -1 is
 * returned and this error must be reported to be fixed.
 *
 * Return value: the VA entry-point, or -1 if none was found
 */
VAEntrypoint
gst_vaapi_entrypoint_get_va_entrypoint(GstVaapiEntrypoint entrypoint)
{
    const GstVaapiEntrypointMap * const m = get_entrypoints_map(entrypoint);

    return m ? m->va_entrypoint : (VAEntrypoint)-1;
}