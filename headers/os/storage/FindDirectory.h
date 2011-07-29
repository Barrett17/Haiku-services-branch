/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FIND_DIRECTORY_H
#define _FIND_DIRECTORY_H


#include <SupportDefs.h>


typedef enum {
	/* Per volume directories */
	B_DESKTOP_DIRECTORY					= 0,
	B_TRASH_DIRECTORY,

	/* System directories */
	B_SYSTEM_DIRECTORY					= 1000,
	B_SYSTEM_ADDONS_DIRECTORY			= 1002,
	B_SYSTEM_BOOT_DIRECTORY,
	B_SYSTEM_FONTS_DIRECTORY,
	B_SYSTEM_LIB_DIRECTORY,
 	B_SYSTEM_SERVERS_DIRECTORY,
	B_SYSTEM_APPS_DIRECTORY,
	B_SYSTEM_BIN_DIRECTORY,
	B_SYSTEM_DOCUMENTATION_DIRECTORY	= 1010,
	B_SYSTEM_PREFERENCES_DIRECTORY,
	B_SYSTEM_TRANSLATORS_DIRECTORY,
	B_SYSTEM_MEDIA_NODES_DIRECTORY,
	B_SYSTEM_SOUNDS_DIRECTORY,
	B_SYSTEM_DATA_DIRECTORY,

	/* Common directories, shared among all users. */
	B_COMMON_DIRECTORY					= 2000,
	B_COMMON_SYSTEM_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_COMMON_BOOT_DIRECTORY,
	B_COMMON_FONTS_DIRECTORY,
	B_COMMON_LIB_DIRECTORY,
	B_COMMON_SERVERS_DIRECTORY,
	B_COMMON_BIN_DIRECTORY,
	B_COMMON_ETC_DIRECTORY,
	B_COMMON_DOCUMENTATION_DIRECTORY,
	B_COMMON_SETTINGS_DIRECTORY,
	B_COMMON_DEVELOP_DIRECTORY,
	B_COMMON_LOG_DIRECTORY,
	B_COMMON_SPOOL_DIRECTORY,
	B_COMMON_TEMP_DIRECTORY,
	B_COMMON_VAR_DIRECTORY,
	B_COMMON_TRANSLATORS_DIRECTORY,
	B_COMMON_MEDIA_NODES_DIRECTORY,
	B_COMMON_SOUNDS_DIRECTORY,
	B_COMMON_DATA_DIRECTORY,
	B_COMMON_CACHE_DIRECTORY,

	/* User directories. These are interpreted in the context
	   of the user making the find_directory call. */
	B_USER_DIRECTORY					= 3000,
	B_USER_CONFIG_DIRECTORY,
	B_USER_ADDONS_DIRECTORY,
	B_USER_BOOT_DIRECTORY,
	B_USER_FONTS_DIRECTORY,
	B_USER_LIB_DIRECTORY,
	B_USER_SETTINGS_DIRECTORY,
	B_USER_DESKBAR_DIRECTORY,
	B_USER_PRINTERS_DIRECTORY,
	B_USER_TRANSLATORS_DIRECTORY,
	B_USER_MEDIA_NODES_DIRECTORY,
	B_USER_SOUNDS_DIRECTORY,
	B_USER_DATA_DIRECTORY,
	B_USER_CACHE_DIRECTORY,

	/* Global directories. */
	B_APPS_DIRECTORY					= 4000,
	B_PREFERENCES_DIRECTORY,
	B_UTILITIES_DIRECTORY,

	/* Obsolete: Legacy BeOS definition to be phased out */
	B_BEOS_DIRECTORY					= 1000,
	B_BEOS_SYSTEM_DIRECTORY,
	B_BEOS_ADDONS_DIRECTORY,
	B_BEOS_BOOT_DIRECTORY,
	B_BEOS_FONTS_DIRECTORY,
	B_BEOS_LIB_DIRECTORY,
 	B_BEOS_SERVERS_DIRECTORY,
	B_BEOS_APPS_DIRECTORY,
	B_BEOS_BIN_DIRECTORY,
	B_BEOS_ETC_DIRECTORY,
	B_BEOS_DOCUMENTATION_DIRECTORY,
	B_BEOS_PREFERENCES_DIRECTORY,
	B_BEOS_TRANSLATORS_DIRECTORY,
	B_BEOS_MEDIA_NODES_DIRECTORY,
	B_BEOS_SOUNDS_DIRECTORY,
	B_BEOS_DATA_DIRECTORY,
} directory_which;

#ifdef __cplusplus
extern "C" {
#endif

/* C interface */

status_t find_directory(directory_which which, dev_t volume, bool createIt,
	char* pathString, int32 length);

#ifdef __cplusplus
}

/* C++ interface */

class BVolume;
class BPath;

status_t find_directory(directory_which which, BPath* path,
	bool createIt = false, BVolume* volume = NULL);

#endif	/* __cplusplus */

#endif	/* _FIND_DIRECTORY_H */