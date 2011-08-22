/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_DISPLAY_H
#define RADEON_HD_DISPLAY_H


#include <video_electronics.h>


// convert radeon connector to common connector type
const int connector_convert[] = {
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_VGA,
	VIDEO_CONNECTOR_DVII,
	VIDEO_CONNECTOR_DVID,
	VIDEO_CONNECTOR_DVIA,
	VIDEO_CONNECTOR_SVIDEO,
	VIDEO_CONNECTOR_COMPOSITE,
	VIDEO_CONNECTOR_LVDS,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_HDMIA,
	VIDEO_CONNECTOR_HDMIB,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_9DIN,
	VIDEO_CONNECTOR_DP
};

const int manual_connector_convert[] = {
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_DVII,
	VIDEO_CONNECTOR_DVII,
	VIDEO_CONNECTOR_DVID,
	VIDEO_CONNECTOR_DVID,
	VIDEO_CONNECTOR_VGA,
	VIDEO_CONNECTOR_COMPOSITE,
	VIDEO_CONNECTOR_SVIDEO,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_9DIN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_HDMIA,
	VIDEO_CONNECTOR_HDMIB,
	VIDEO_CONNECTOR_LVDS,
	VIDEO_CONNECTOR_9DIN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_UNKNOWN,
	VIDEO_CONNECTOR_DP,
	VIDEO_CONNECTOR_EDP,
	VIDEO_CONNECTOR_UNKNOWN
};

status_t init_registers(register_info* reg, uint8 crtid);
status_t detect_crt_ranges(uint32 crtid);
status_t detect_connectors();
status_t detect_displays();
void debug_displays();

void display_crtc_lock(uint8 crtc_id, int command);
void display_crtc_blank(uint8 crtc_id, int command);
void display_crtc_scale(uint8 crtc_id, display_mode *mode);
void display_crtc_fb_set_legacy(uint8 crtc_id, display_mode *mode);
void display_crtc_fb_set_dce1(uint8 crtc_id, display_mode *mode);
void display_crtc_set(uint8 crtc_id, display_mode *mode);
void display_crtc_set_dtd(uint8 crtc_id, display_mode *mode);
void display_crtc_power(uint8 crt_id, int command);


#endif /* RADEON_HD_DISPLAY_H */
