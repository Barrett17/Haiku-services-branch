/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#include "accelerant_protos.h"
#include "accelerant.h"
#include "bios.h"
#include "display.h"

#include <stdlib.h>
#include <string.h>


#define TRACE_DISPLAY
#ifdef TRACE_DISPLAY
extern "C" void _sPrintf(const char *format, ...);
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) _sPrintf("radeon_hd: " x)


/*! Populate regs with device dependant register locations */
status_t
init_registers(register_info* regs, uint8 crtid)
{
	memset(regs, 0, sizeof(register_info));

	radeon_shared_info &info = *gInfo->shared_info;

	if (info.device_chipset >= RADEON_R1000) {
		uint32 offset = 0;

		// AMD Eyefinity on Evergreen GPUs
		if (crtid == 1) {
			offset = EVERGREEN_CRTC1_REGISTER_OFFSET;
			regs->vgaControl = D2VGA_CONTROL;
		} else if (crtid == 2) {
			offset = EVERGREEN_CRTC2_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D3VGA_CONTROL;
		} else if (crtid == 3) {
			offset = EVERGREEN_CRTC3_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D4VGA_CONTROL;
		} else if (crtid == 4) {
			offset = EVERGREEN_CRTC4_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D5VGA_CONTROL;
		} else if (crtid == 5) {
			offset = EVERGREEN_CRTC5_REGISTER_OFFSET;
			regs->vgaControl = EVERGREEN_D6VGA_CONTROL;
		} else {
			offset = EVERGREEN_CRTC0_REGISTER_OFFSET;
			regs->vgaControl = D1VGA_CONTROL;
		}

		regs->crtcOffset = offset;

		// Evergreen+ is crtoffset + register
		regs->grphEnable = offset + EVERGREEN_GRPH_ENABLE;
		regs->grphControl = offset + EVERGREEN_GRPH_CONTROL;
		regs->grphSwapControl = offset + EVERGREEN_GRPH_SWAP_CONTROL;

		regs->grphPrimarySurfaceAddr
			= offset + EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS;
		regs->grphSecondarySurfaceAddr
			= offset + EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS;
		regs->grphPrimarySurfaceAddrHigh
			= offset + EVERGREEN_GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
		regs->grphSecondarySurfaceAddrHigh
			= offset + EVERGREEN_GRPH_SECONDARY_SURFACE_ADDRESS_HIGH;

		regs->grphPitch = offset + EVERGREEN_GRPH_PITCH;
		regs->grphSurfaceOffsetX
			= offset + EVERGREEN_GRPH_SURFACE_OFFSET_X;
		regs->grphSurfaceOffsetY
			= offset + EVERGREEN_GRPH_SURFACE_OFFSET_Y;
		regs->grphXStart = offset + EVERGREEN_GRPH_X_START;
		regs->grphYStart = offset + EVERGREEN_GRPH_Y_START;
		regs->grphXEnd = offset + EVERGREEN_GRPH_X_END;
		regs->grphYEnd = offset + EVERGREEN_GRPH_Y_END;
		regs->crtControl = offset + EVERGREEN_CRTC_CONTROL;
		regs->modeDesktopHeight = offset + EVERGREEN_DESKTOP_HEIGHT;
		regs->modeDataFormat = offset + EVERGREEN_DATA_FORMAT;
		regs->viewportStart = offset + EVERGREEN_VIEWPORT_START;
		regs->viewportSize = offset + EVERGREEN_VIEWPORT_SIZE;

	} else if (info.device_chipset >= RADEON_R600
		&& info.device_chipset < RADEON_R1000) {

		// r600 - r700 are D1 or D2 based on primary / secondary crt
		regs->vgaControl
			= crtid == 1 ? D2VGA_CONTROL : D1VGA_CONTROL;
		regs->grphEnable
			= crtid == 1 ? D2GRPH_ENABLE : D1GRPH_ENABLE;
		regs->grphControl
			= crtid == 1 ? D2GRPH_CONTROL : D1GRPH_CONTROL;
		regs->grphSwapControl
			= crtid == 1 ? D2GRPH_SWAP_CNTL : D1GRPH_SWAP_CNTL;
		regs->grphPrimarySurfaceAddr
			= crtid == 1 ? D2GRPH_PRIMARY_SURFACE_ADDRESS
				: D1GRPH_PRIMARY_SURFACE_ADDRESS;
		regs->grphSecondarySurfaceAddr
			= crtid == 1 ? D2GRPH_SECONDARY_SURFACE_ADDRESS
				: D1GRPH_SECONDARY_SURFACE_ADDRESS;

		regs->crtcOffset
			= crtid == 1 ? (D2GRPH_X_END - D1GRPH_X_END) : 0;

		// Surface Address high only used on r770+
		regs->grphPrimarySurfaceAddrHigh
			= crtid == 1 ? D2GRPH_PRIMARY_SURFACE_ADDRESS_HIGH
				: D1GRPH_PRIMARY_SURFACE_ADDRESS_HIGH;
		regs->grphSecondarySurfaceAddrHigh
			= crtid == 1 ? D2GRPH_SECONDARY_SURFACE_ADDRESS_HIGH
				: D1GRPH_SECONDARY_SURFACE_ADDRESS_HIGH;

		regs->grphPitch
			= crtid == 1 ? D2GRPH_PITCH : D1GRPH_PITCH;
		regs->grphSurfaceOffsetX
			= crtid == 1 ? D2GRPH_SURFACE_OFFSET_X : D1GRPH_SURFACE_OFFSET_X;
		regs->grphSurfaceOffsetY
			= crtid == 1 ? D2GRPH_SURFACE_OFFSET_Y : D1GRPH_SURFACE_OFFSET_Y;
		regs->grphXStart
			= crtid == 1 ? D2GRPH_X_START : D1GRPH_X_START;
		regs->grphYStart
			= crtid == 1 ? D2GRPH_Y_START : D1GRPH_Y_START;
		regs->grphXEnd
			= crtid == 1 ? D2GRPH_X_END : D1GRPH_X_END;
		regs->grphYEnd
			= crtid == 1 ? D2GRPH_Y_END : D1GRPH_Y_END;
		regs->crtControl
			= crtid == 1 ? D2CRTC_CONTROL : D1CRTC_CONTROL;
		regs->modeDesktopHeight
			= crtid == 1 ? D2MODE_DESKTOP_HEIGHT : D1MODE_DESKTOP_HEIGHT;
		regs->modeDataFormat
			= crtid == 1 ? D2MODE_DATA_FORMAT : D1MODE_DATA_FORMAT;
		regs->viewportStart
			= crtid == 1 ? D2MODE_VIEWPORT_START : D1MODE_VIEWPORT_START;
		regs->viewportSize
			= crtid == 1 ? D2MODE_VIEWPORT_SIZE : D1MODE_VIEWPORT_SIZE;
	} else {
		// this really shouldn't happen unless a driver PCIID chipset is wrong
		TRACE("%s, unknown Radeon chipset: r%X\n", __func__,
			info.device_chipset);
		return B_ERROR;
	}

	// Populate common registers
	// TODO : Wait.. this doesn't work with Eyefinity > crt 1.

	regs->modeCenter
		= crtid == 1 ? D2MODE_CENTER : D1MODE_CENTER;
	regs->grphUpdate
		= crtid == 1 ? D2GRPH_UPDATE : D1GRPH_UPDATE;
	regs->crtHPolarity
		= crtid == 1 ? D2CRTC_H_SYNC_A_CNTL : D1CRTC_H_SYNC_A_CNTL;
	regs->crtVPolarity
		= crtid == 1 ? D2CRTC_V_SYNC_A_CNTL : D1CRTC_V_SYNC_A_CNTL;
	regs->crtHTotal
		= crtid == 1 ? D2CRTC_H_TOTAL : D1CRTC_H_TOTAL;
	regs->crtVTotal
		= crtid == 1 ? D2CRTC_V_TOTAL : D1CRTC_V_TOTAL;
	regs->crtHSync
		= crtid == 1 ? D2CRTC_H_SYNC_A : D1CRTC_H_SYNC_A;
	regs->crtVSync
		= crtid == 1 ? D2CRTC_V_SYNC_A : D1CRTC_V_SYNC_A;
	regs->crtHBlank
		= crtid == 1 ? D2CRTC_H_BLANK_START_END : D1CRTC_H_BLANK_START_END;
	regs->crtVBlank
		= crtid == 1 ? D2CRTC_V_BLANK_START_END : D1CRTC_V_BLANK_START_END;
	regs->crtInterlace
		= crtid == 1 ? D2CRTC_INTERLACE_CONTROL : D1CRTC_INTERLACE_CONTROL;
	regs->crtCountControl
		= crtid == 1 ? D2CRTC_COUNT_CONTROL : D1CRTC_COUNT_CONTROL;
	regs->sclUpdate
		= crtid == 1 ? D2SCL_UPDATE : D1SCL_UPDATE;
	regs->sclEnable
		= crtid == 1 ? D2SCL_ENABLE : D1SCL_ENABLE;
	regs->sclTapControl
		= crtid == 1 ? D2SCL_TAP_CONTROL : D1SCL_TAP_CONTROL;

	TRACE("%s, registers for ATI chipset r%X crt #%d loaded\n", __func__,
		info.device_chipset, crtid);

	return B_OK;
}


status_t
detect_crt_ranges(uint32 crtid)
{
	edid1_info *edid = &gInfo->shared_info->edid_info;

	// TODO : VESA edid is just for primary monitor?

	// Scan each VESA EDID description for monitor ranges
	for (uint32 index = 0; index < EDID1_NUM_DETAILED_MONITOR_DESC; index++) {

		edid1_detailed_monitor *monitor
			= &edid->detailed_monitor[index];

		if (monitor->monitor_desc_type
			== EDID1_MONITOR_RANGES) {
			edid1_monitor_range range = monitor->data.monitor_range;
			gDisplay[crtid]->vfreq_min = range.min_v;   /* in Hz */
			gDisplay[crtid]->vfreq_max = range.max_v;
			gDisplay[crtid]->hfreq_min = range.min_h;   /* in kHz */
			gDisplay[crtid]->hfreq_max = range.max_h;
			return B_OK;
		}
	}

	return B_ERROR;
}


union atom_supported_devices {
	struct _ATOM_SUPPORTED_DEVICES_INFO info;
	struct _ATOM_SUPPORTED_DEVICES_INFO_2 info_2;
	struct _ATOM_SUPPORTED_DEVICES_INFO_2d1 info_2d1;
};


status_t
detect_connectors()
{
	int index = GetIndexIntoMasterTable(DATA, SupportedDevicesInfo);
	uint8 frev;
	uint8 crev;
	uint16 size;
	uint16 data_offset;

	if (atom_parse_data_header(gAtomContext, index, &size, &frev, &crev,
		&data_offset) != B_OK) {
		ERROR("%s: unable to parse data header!\n", __func__);
		return B_ERROR;
	}

	union atom_supported_devices *supported_devices;
	supported_devices
		= (union atom_supported_devices *)
		((uint16 *)gAtomContext->bios + data_offset);

	uint16 device_support
		= B_LENDIAN_TO_HOST_INT16(supported_devices->info.usDeviceSupport);

	int32 i;
	for (i = 0; i < ATOM_MAX_SUPPORTED_DEVICE; i++) {
		ATOM_CONNECTOR_INFO_I2C ci
			= supported_devices->info.asConnInfo[i];

		gConnector[i]->valid = false;

		if (!(device_support & (1 << i)))
			continue;

		if (i == ATOM_DEVICE_CV_INDEX) {
			TRACE("%s: skipping component video\n",
				__func__);
			continue;
		}

		gConnector[i]->connector_type
			= connector_convert[ci.sucConnectorInfo.sbfAccess.bfConnectorType];

		if (gConnector[i]->connector_type
			== VIDEO_CONNECTOR_UNKNOWN) {
			TRACE("%s: skipping unknown connector at %" B_PRId32
				" of 0x%" B_PRIX8"\n", __func__, i,
				ci.sucConnectorInfo.sbfAccess.bfConnectorType);
			continue;
		}

		// uint8 dac = ci.sucConnectorInfo.sbfAccess.bfAssociatedDAC;
		gConnector[i]->line_mux = ci.sucI2cId.ucAccess;

		// TODO : give tv unique connector ids
		// TODO : ddc bus

		// Always set CRT1 and CRT2 as VGA, some cards incorrectly set
		// VGA ports as DVI
		if (i == ATOM_DEVICE_CRT1_INDEX || i == ATOM_DEVICE_CRT2_INDEX)
			gConnector[i]->connector_type = VIDEO_CONNECTOR_VGA;

		gConnector[i]->valid = true;
		gConnector[i]->devices = (1 << i);

		// TODO : add the encoder
		#if 0
		radeon_add_atom_encoder(dev,
			radeon_get_encoder_enum(dev,
				(1 << i),
				dac),
			(1 << i),
			0);
		#endif
	}

	// TODO : combine shared connectors

	// TODO : add connectors

	for (i = 0; i < ATOM_MAX_SUPPORTED_DEVICE_INFO; i++) {
		if (gConnector[i]->valid == true) {
			TRACE("%s: connector #%" B_PRId32 " is %s\n", __func__, i,
				decode_connector_name(gConnector[i]->connector_type));
		}
	}

	return B_OK;
}


// TODO : this gets connectors from object table
status_t
detect_connectors_manual()
{
	int index = GetIndexIntoMasterTable(DATA, Object_Header);

	uint8 frev;
	uint8 crev;
	uint16 size;
	uint16 data_offset;

	if (atom_parse_data_header(gAtomContext, index, &size, &frev, &crev,
		&data_offset) != B_OK) {
		ERROR("%s: ERROR: parsing data header failed!\n", __func__);
		return B_ERROR;
	}

	if (crev < 2) {
		ERROR("%s: ERROR: data header version unknown!\n", __func__);
		return B_ERROR;
	}

	ATOM_CONNECTOR_OBJECT_TABLE *con_obj;
	ATOM_ENCODER_OBJECT_TABLE *enc_obj;
	ATOM_OBJECT_TABLE *router_obj;
	ATOM_DISPLAY_OBJECT_PATH_TABLE *path_obj;
	ATOM_OBJECT_HEADER *obj_header;

	obj_header = (ATOM_OBJECT_HEADER *)
		((uint16 *)gAtomContext->bios + data_offset);
	path_obj = (ATOM_DISPLAY_OBJECT_PATH_TABLE *)
		((uint16 *)gAtomContext->bios + data_offset
		+ B_LENDIAN_TO_HOST_INT16(obj_header->usDisplayPathTableOffset));
	con_obj = (ATOM_CONNECTOR_OBJECT_TABLE *)
		((uint16 *)gAtomContext->bios + data_offset
		+ B_LENDIAN_TO_HOST_INT16(obj_header->usConnectorObjectTableOffset));
	enc_obj = (ATOM_ENCODER_OBJECT_TABLE *)
		((uint16 *)gAtomContext->bios + data_offset
		+ B_LENDIAN_TO_HOST_INT16(obj_header->usEncoderObjectTableOffset));
	router_obj = (ATOM_OBJECT_TABLE *)
		((uint16 *)gAtomContext->bios + data_offset
		+ B_LENDIAN_TO_HOST_INT16(obj_header->usRouterObjectTableOffset));
	int device_support = B_LENDIAN_TO_HOST_INT16(obj_header->usDeviceSupport);

	int path_size = 0;
	int32 i = 0;

	TRACE("%s: found %" B_PRIu8 " potential display paths.\n", __func__,
		path_obj->ucNumOfDispPath);

	for (i = 0; i < path_obj->ucNumOfDispPath; i++) {
		uint8 *addr = (uint8*)path_obj->asDispPath;
		ATOM_DISPLAY_OBJECT_PATH *path;
		addr += path_size;
		path = (ATOM_DISPLAY_OBJECT_PATH *) addr;
		path_size += B_LENDIAN_TO_HOST_INT16(path->usSize);

		int connector_type;
		uint16 connector_object_id;

		if (device_support & B_LENDIAN_TO_HOST_INT16(path->usDeviceTag)) {
			TRACE("%s: Display Path #%" B_PRId32 "\n", __func__, i);

			uint16 igp_lane_info;

			uint8 con_obj_id
				= (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
				& OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
			//uint8 con_obj_num
			//	= (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
			//	& ENUM_ID_MASK) >> ENUM_ID_SHIFT;
			//uint8 con_obj_type
			//	= (B_LENDIAN_TO_HOST_INT16(path->usConnObjectId)
			//	& OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;

			// TODO : CV support
			if (B_LENDIAN_TO_HOST_INT16(path->usDeviceTag)
				== ATOM_DEVICE_CV_SUPPORT) {
				continue;
			}

			if (0)
				ERROR("%s: TODO : IGP chip connector detection\n", __func__);
			else {
				igp_lane_info = 0;
				connector_type = manual_connector_convert[con_obj_id];
				connector_object_id = con_obj_id;
			}

			if (connector_type == VIDEO_CONNECTOR_UNKNOWN) {
				TRACE("%s: Unknown connector, skipping\n", __func__);
				continue;
			} else {
				TRACE("%s: Found connector %s\n", __func__,
					decode_connector_name(connector_type));
			}

			// We have to go deeper! -AMD
			// (find encoder for connector)
			int32 j;
			for (j = 0; j < ((B_LENDIAN_TO_HOST_INT16(path->usSize) - 8) / 2);
				j++) {
				//uint8 grph_obj_id
				//	= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j]) &
				//	OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
				//uint8 grph_obj_num
				//	= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j]) &
				//	ENUM_ID_MASK) >> ENUM_ID_SHIFT;
				uint8 grph_obj_type
					= (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j]) &
					OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;
				if (grph_obj_type == GRAPH_OBJECT_TYPE_ENCODER) {
					int32 k;
					TRACE("%s: Found encoder at #%" B_PRIu32 "\n", __func__, j);
					for (k = 0; k < enc_obj->ucNumberOfObjects; k++) {
						uint16 encoder_obj
							= B_LENDIAN_TO_HOST_INT16(
							enc_obj->asObjects[k].usObjectID);
						if (B_LENDIAN_TO_HOST_INT16(path->usGraphicObjIds[j])
							== encoder_obj) {
							ATOM_COMMON_RECORD_HEADER *record
								= (ATOM_COMMON_RECORD_HEADER *)
								((uint16 *)gAtomContext->bios + data_offset
								+ B_LENDIAN_TO_HOST_INT16(
								enc_obj->asObjects[k].usRecordOffset));
							ATOM_ENCODER_CAP_RECORD *cap_record;
							uint16 caps = 0;
							while (record->ucRecordSize > 0
								&& record->ucRecordType > 0
								&& record->ucRecordType
								<= ATOM_MAX_OBJECT_RECORD_NUMBER) {
								switch (record->ucRecordType) {
									case ATOM_ENCODER_CAP_RECORD_TYPE:
										cap_record = (ATOM_ENCODER_CAP_RECORD *)
											record;
										caps = B_LENDIAN_TO_HOST_INT16(
											cap_record->usEncoderCap);
										break;
								}
								record = (ATOM_COMMON_RECORD_HEADER *)
									((char *)record + record->ucRecordSize);
							}
							TRACE("%s: add encoder\n", __func__);
							// TODO : add the encoder - Finally!
							//radeon_add_atom_encoder(dev,
							//	encoder_obj,
							//	le16_to_cpu
							//	(path->
							//	usDeviceTag),
							//	caps);
						}
					}
				} else if (grph_obj_type == GRAPH_OBJECT_TYPE_ROUTER) {
					ERROR("%s: TODO : Router object?\n", __func__);
				}
			}

			// TODO : look up gpio for ddc, hpd

			// TODO : aux chan transactions

			// TODO : add connector
			TRACE("%s: add connector\n", __func__);
			// radeon_add_atom_connector(dev,
			// 	conn_id,
			// 	le16_to_cpu(path-> usDeviceTag),
			// 	connector_type, &ddc_bus,
			// 	igp_lane_info,
			// 	connector_object_id,
			// 	&hpd,
			// 	&router);
		}
	}

	return B_OK;
}


status_t
detect_displays()
{
	// reset known displays
	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		gDisplay[id]->active = false;
		gDisplay[id]->found_ranges = false;
	}

	uint32 index = 0;

	// Probe for DAC monitors connected
	for (uint32 id = 0; id < 2; id++) {
		if (DACSense(id)) {
			gDisplay[index]->active = true;
			gDisplay[index]->connection_type = ATOM_ENCODER_MODE_CRT;
			gDisplay[index]->connection_id = id;
			init_registers(gDisplay[index]->regs, index);
			if (detect_crt_ranges(index) == B_OK)
				gDisplay[index]->found_ranges = true;

			if (index < MAX_DISPLAY)
				index++;
			else
				return B_OK;
		}
	}

	// Probe for TMDS monitors connected
	for (uint32 id = 0; id < 1; id++) {
		if (TMDSSense(id)) {
			gDisplay[index]->active = true;
			gDisplay[index]->connection_type = ATOM_ENCODER_MODE_DVI;
				// or ATOM_ENCODER_MODE_HDMI?
			gDisplay[index]->connection_id = id;
			init_registers(gDisplay[index]->regs, index);
			if (detect_crt_ranges(index) == B_OK)
				gDisplay[index]->found_ranges = true;

			if (index < MAX_DISPLAY)
				index++;
			else
				return B_OK;
		}
	}

	// No monitors? Lets assume LVDS for now
	if (index == 0) {
		gDisplay[index]->active = true;
		gDisplay[index]->connection_type = ATOM_ENCODER_MODE_LVDS;
		gDisplay[index]->connection_id = 1;
			// 0 : LVDSA ; 1 : LVDSB / TDMSB
		init_registers(gDisplay[index]->regs, index);
	}

	return B_OK;
}


void
debug_displays()
{
	TRACE("Currently detected monitors===============\n");
	for (uint32 id = 0; id < MAX_DISPLAY; id++) {
		TRACE("Display #%" B_PRIu32 " active = %s\n",
			id, gDisplay[id]->active ? "true" : "false");

		if (gDisplay[id]->active) {
			switch (gDisplay[id]->connection_type) {
				case ATOM_ENCODER_MODE_DP:
					TRACE(" + connection: DP\n");
					break;
				case ATOM_ENCODER_MODE_LVDS:
					TRACE(" + connection: LVDS\n");
					break;
				case ATOM_ENCODER_MODE_DVI:
					TRACE(" + connection: DVI\n");
					break;
				case ATOM_ENCODER_MODE_HDMI:
					TRACE(" + connection: HDMI\n");
					break;
				case ATOM_ENCODER_MODE_SDVO:
					TRACE(" + connection: SDVO\n");
					break;
				case ATOM_ENCODER_MODE_DP_AUDIO:
					TRACE(" + connection: DP AUDIO\n");
					break;
				case ATOM_ENCODER_MODE_TV:
					TRACE(" + connection: TV\n");
					break;
				case ATOM_ENCODER_MODE_CV:
					TRACE(" + connection: CV\n");
					break;
				case ATOM_ENCODER_MODE_CRT:
					TRACE(" + connection: CRT\n");
					break;
				case ATOM_ENCODER_MODE_DVO:
					TRACE(" + connection: DVO\n");
					break;
				default:
					TRACE(" + connection: UNKNOWN\n");
			}

			TRACE(" + connection index: % " B_PRIu8 "\n",
				gDisplay[id]->connection_id);

			TRACE(" + limits: Vert Min/Max: %" B_PRIu32 "/%" B_PRIu32"\n",
				gDisplay[id]->vfreq_min, gDisplay[id]->vfreq_max);
			TRACE(" + limits: Horz Min/Max: %" B_PRIu32 "/%" B_PRIu32"\n",
				gDisplay[id]->hfreq_min, gDisplay[id]->hfreq_max);
		}
	}
	TRACE("==========================================\n");

}


void
display_crtc_lock(uint8 crtc_id, int command)
{
	ENABLE_CRTC_PS_ALLOCATION args;
	int index
		= GetIndexIntoMasterTable(COMMAND, UpdateCRTC_DoubleBufferRegisters);

	memset(&args, 0, sizeof(args));

	args.ucCRTC = crtc_id;
	args.ucEnable = command;

	atom_execute_table(gAtomContext, index, (uint32 *)&args);
}


void
display_crtc_blank(uint8 crtc_id, int command)
{
	int index = GetIndexIntoMasterTable(COMMAND, BlankCRTC);
	BLANK_CRTC_PS_ALLOCATION args;

	memset(&args, 0, sizeof(args));

	args.ucCRTC = crtc_id;
	args.ucBlanking = command;

	atom_execute_table(gAtomContext, index, (uint32 *)&args);
}


void
display_crtc_scale(uint8 crtc_id, display_mode *mode)
{
	ENABLE_SCALER_PS_ALLOCATION args;
	int index = GetIndexIntoMasterTable(COMMAND, EnableScaler);

	memset(&args, 0, sizeof(args));

	args.ucScaler = crtc_id;
	args.ucEnable = ATOM_SCALER_EXPANSION;

	atom_execute_table(gAtomContext, index, (uint32 *)&args);
}


void
display_crtc_fb_set_dce1(uint8 crtc_id, display_mode *mode)
{
	radeon_shared_info &info = *gInfo->shared_info;
	register_info* regs = gDisplay[crtc_id]->regs;

	uint32 fb_swap = R600_D1GRPH_SWAP_ENDIAN_NONE;
	uint32 fb_format;

	uint32 bytesPerPixel;
	uint32 bitsPerPixel;

	switch (mode->space) {
		case B_CMAP8:
			bytesPerPixel = 1;
			bitsPerPixel = 8;
			fb_format = AVIVO_D1GRPH_CONTROL_DEPTH_8BPP
				| AVIVO_D1GRPH_CONTROL_8BPP_INDEXED;
			break;
		case B_RGB15_LITTLE:
			bytesPerPixel = 2;
			bitsPerPixel = 15;
			fb_format = AVIVO_D1GRPH_CONTROL_DEPTH_16BPP
				| AVIVO_D1GRPH_CONTROL_16BPP_ARGB1555;
			break;
		case B_RGB16_LITTLE:
			bytesPerPixel = 2;
			bitsPerPixel = 16;
			fb_format = AVIVO_D1GRPH_CONTROL_DEPTH_16BPP
				| AVIVO_D1GRPH_CONTROL_16BPP_RGB565;
			#ifdef __POWERPC__
			fb_swap = R600_D1GRPH_SWAP_ENDIAN_16BIT;
			#endif
			break;
		case B_RGB24_LITTLE:
		case B_RGB32_LITTLE:
		default:
			bytesPerPixel = 4;
			bitsPerPixel = 32;
			fb_format = AVIVO_D1GRPH_CONTROL_DEPTH_32BPP
				| AVIVO_D1GRPH_CONTROL_32BPP_ARGB8888;
			#ifdef __POWERPC__
			fb_swap = R600_D1GRPH_SWAP_ENDIAN_32BIT;
			#endif
			break;
	}

	uint32 bytesPerRow = mode->virtual_width * bytesPerPixel;

	Write32(OUT, regs->vgaControl, 0);

	uint64 fbAddressInt = gInfo->shared_info->frame_buffer_int;

	Write32(OUT, regs->grphPrimarySurfaceAddr, (fbAddressInt & 0xFFFFFFFF));
	Write32(OUT, regs->grphSecondarySurfaceAddr, (fbAddressInt & 0xFFFFFFFF));

	if (info.device_chipset >= (RADEON_R700 | 0x70)) {
		Write32(OUT, regs->grphPrimarySurfaceAddrHigh,
			(fbAddressInt >> 32) & 0xf);
		Write32(OUT, regs->grphSecondarySurfaceAddrHigh,
			(fbAddressInt >> 32) & 0xf);
	}

	if (info.device_chipset >= RADEON_R600)
		Write32(CRT, regs->grphSwapControl, fb_swap);

	Write32(CRT, regs->grphSurfaceOffsetX, 0);
	Write32(CRT, regs->grphSurfaceOffsetY, 0);
	Write32(CRT, regs->grphXStart, 0);
	Write32(CRT, regs->grphYStart, 0);
	Write32(CRT, regs->grphXEnd, mode->virtual_width);
	Write32(CRT, regs->grphYEnd, mode->virtual_height);
	Write32(CRT, regs->grphPitch, bytesPerRow / 4);

	Write32(CRT, regs->grphEnable, 1);
		// Enable Frame buffer

	Write32(CRT, regs->modeDesktopHeight, mode->virtual_height);

	Write32(CRT, regs->viewportStart, 0);

	Write32(CRT, regs->viewportSize,
		mode->timing.v_display | (mode->timing.h_display << 16));

	uint32 tmp = Read32(CRT, AVIVO_D1GRPH_FLIP_CONTROL + regs->crtcOffset);
	tmp &= ~AVIVO_D1GRPH_SURFACE_UPDATE_H_RETRACE_EN;
	Write32(OUT, AVIVO_D1GRPH_FLIP_CONTROL + regs->crtcOffset, tmp);

	Write32(OUT, AVIVO_D1MODE_MASTER_UPDATE_MODE + regs->crtcOffset, 0);
		// Pageflip to happen anywhere in vblank
}


void
display_crtc_fb_set_legacy(uint8 crtc_id, display_mode *mode)
{
	register_info* regs = gDisplay[crtc_id]->regs;

	uint64 fbAddressInt = gInfo->shared_info->frame_buffer_int;

	Write32(CRT, regs->grphUpdate, (1<<16));
		// Lock for update (isn't this normally the other way around on VGA?

	Write32Mask(CRT, regs->grphEnable, 1, 0x00000001);
		// Enable Frame buffer

	Write32(CRT, regs->grphControl, 0);
		// Reset stored depth, format, etc

	uint32 bytesPerPixel;
	uint32 bitsPerPixel;

	// set color mode on video card
	switch (mode->space) {
		case B_CMAP8:
			bytesPerPixel = 1;
			bitsPerPixel = 8;
			Write32Mask(CRT, regs->grphControl,
				0, 0x00000703);
			break;
		case B_RGB15_LITTLE:
			bytesPerPixel = 2;
			bitsPerPixel = 15;
			Write32Mask(CRT, regs->grphControl,
				0x000001, 0x00000703);
			break;
		case B_RGB16_LITTLE:
			bytesPerPixel = 2;
			bitsPerPixel = 16;
			Write32Mask(CRT, regs->grphControl,
				0x000101, 0x00000703);
			break;
		case B_RGB24_LITTLE:
			bytesPerPixel = 4;
			bitsPerPixel = 24;
			Write32Mask(CRT, regs->grphControl,
				0x000002, 0x00000703);
			break;
		case B_RGB32_LITTLE:
		default:
			bytesPerPixel = 4;
			bitsPerPixel = 32;
			Write32Mask(CRT, regs->grphControl,
				0x000002, 0x00000703);
			break;
	}

	uint32 bytesPerRow = mode->virtual_width * bytesPerPixel;

	Write32(CRT, regs->grphSwapControl, 0);
		// only for chipsets > r600

	// Tell GPU which frame buffer address to draw from
	Write32(CRT, regs->grphPrimarySurfaceAddr, fbAddressInt & 0xFFFFFFFF);
	Write32(CRT, regs->grphSecondarySurfaceAddr, fbAddressInt & 0xFFFFFFFF);

	Write32(CRT, regs->grphSurfaceOffsetX, 0);
	Write32(CRT, regs->grphSurfaceOffsetY, 0);
	Write32(CRT, regs->grphXStart, 0);
	Write32(CRT, regs->grphYStart, 0);
	Write32(CRT, regs->grphXEnd, mode->virtual_width);
	Write32(CRT, regs->grphYEnd, mode->virtual_height);
	Write32(CRT, regs->grphPitch, bytesPerRow / 4);

	Write32(CRT, regs->modeDesktopHeight, mode->virtual_height);

	Write32(CRT, regs->grphUpdate, 0);
		// Unlock changed registers

	// update shared info
	gInfo->shared_info->bytes_per_row = bytesPerRow;
	gInfo->shared_info->current_mode = *mode;
	gInfo->shared_info->bits_per_pixel = bitsPerPixel;

	// TODO : recompute bandwidth via rv515_bandwidth_avivo_update
}


void
display_crtc_set(uint8 crtc_id, display_mode *mode)
{
	display_timing& displayTiming = mode->timing;

	TRACE("%s called to do %dx%d\n",
		__func__, displayTiming.h_display, displayTiming.v_display);

	SET_CRTC_TIMING_PARAMETERS_PS_ALLOCATION args;
	int index = GetIndexIntoMasterTable(COMMAND, SetCRTC_Timing);
	uint16 misc = 0;

	memset(&args, 0, sizeof(args));

	args.usH_Total = B_HOST_TO_LENDIAN_INT16(displayTiming.h_total);
	args.usH_Disp = B_HOST_TO_LENDIAN_INT16(displayTiming.h_display);
	args.usH_SyncStart = B_HOST_TO_LENDIAN_INT16(displayTiming.h_sync_start);
	args.usH_SyncWidth = B_HOST_TO_LENDIAN_INT16(displayTiming.h_sync_end
		- displayTiming.h_sync_start);

	args.usV_Total = B_HOST_TO_LENDIAN_INT16(displayTiming.v_total);
	args.usV_Disp = B_HOST_TO_LENDIAN_INT16(displayTiming.v_display);
	args.usV_SyncStart = B_HOST_TO_LENDIAN_INT16(displayTiming.v_sync_start);
	args.usV_SyncWidth = B_HOST_TO_LENDIAN_INT16(displayTiming.v_sync_end
		- displayTiming.v_sync_start);

	args.ucOverscanRight = 0;
	args.ucOverscanLeft = 0;
	args.ucOverscanBottom = 0;
	args.ucOverscanTop = 0;

	if ((displayTiming.flags & B_POSITIVE_HSYNC) == 0)
		misc |= ATOM_HSYNC_POLARITY;
	if ((displayTiming.flags & B_POSITIVE_VSYNC) == 0)
		misc |= ATOM_VSYNC_POLARITY;

	args.susModeMiscInfo.usAccess = B_HOST_TO_LENDIAN_INT16(misc);
	args.ucCRTC = crtc_id;

	atom_execute_table(gAtomContext, index, (uint32 *)&args);
}


void
display_crtc_set_dtd(uint8 crtc_id, display_mode *mode)
{
	display_timing& displayTiming = mode->timing;

	TRACE("%s called to do %dx%d\n",
		__func__, displayTiming.h_display, displayTiming.v_display);

	SET_CRTC_USING_DTD_TIMING_PARAMETERS args;
	int index = GetIndexIntoMasterTable(COMMAND, SetCRTC_UsingDTDTiming);
	uint16 misc = 0;

	memset(&args, 0, sizeof(args));

	uint16 blankStart
		= MIN(displayTiming.h_sync_start, displayTiming.h_display);
	uint16 blankEnd
		= MAX(displayTiming.h_sync_end, displayTiming.h_total);
	args.usH_Size = B_HOST_TO_LENDIAN_INT16(displayTiming.h_display);
	args.usH_Blanking_Time = B_HOST_TO_LENDIAN_INT16(blankEnd - blankStart);

	blankStart = MIN(displayTiming.v_sync_start, displayTiming.v_display);
	blankEnd = MAX(displayTiming.v_sync_end, displayTiming.v_total);
	args.usV_Size = B_HOST_TO_LENDIAN_INT16(displayTiming.v_display);
	args.usV_Blanking_Time = B_HOST_TO_LENDIAN_INT16(blankEnd - blankStart);

	args.usH_SyncOffset = B_HOST_TO_LENDIAN_INT16(displayTiming.h_sync_start
		- displayTiming.h_display);
	args.usH_SyncWidth = B_HOST_TO_LENDIAN_INT16(displayTiming.h_sync_end
		- displayTiming.h_sync_start);

	args.usV_SyncOffset = B_HOST_TO_LENDIAN_INT16(displayTiming.v_sync_start
		- displayTiming.v_display);
	args.usV_SyncWidth = B_HOST_TO_LENDIAN_INT16(displayTiming.v_sync_end
		- displayTiming.v_sync_start);

	args.ucH_Border = 0;
	args.ucV_Border = 0;

	if ((displayTiming.flags & B_POSITIVE_HSYNC) == 0)
		misc |= ATOM_HSYNC_POLARITY;
	if ((displayTiming.flags & B_POSITIVE_VSYNC) == 0)
		misc |= ATOM_VSYNC_POLARITY;

	args.susModeMiscInfo.usAccess = B_HOST_TO_LENDIAN_INT16(misc);
	args.ucCRTC = crtc_id;

	atom_execute_table(gAtomContext, index, (uint32 *)&args);
}


void
display_crtc_power(uint8 crt_id, int command)
{
	int index = GetIndexIntoMasterTable(COMMAND, EnableCRTC);
	ENABLE_CRTC_PS_ALLOCATION args;

	memset(&args, 0, sizeof(args));

	args.ucCRTC = crt_id;
	args.ucEnable = command;

	atom_execute_table(gAtomContext, index, (uint32*)&args);
}


