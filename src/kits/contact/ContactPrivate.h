/*
 * Copyright 2011-2012 Casalinuovo Dario
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <HashMap.h>

struct standardFieldsMap {
	field_type type;
	const char* label;
};

static standardFieldsMap gStandardFields[] = {
		{ B_CONTACT_ADDRESS, "Address" },
		{ B_CONTACT_ASSISTANT, "Assistant" },
		{ B_CONTACT_BIRTHDAY, "Birthday" },
		{ B_CONTACT_CUSTOM, "Custom" },
 		{ B_CONTACT_DELIVERY_LABEL, "Delivery Label" },
 		{ B_CONTACT_EMAIL, "Email" },
		{ B_CONTACT_FORMATTED_NAME, "Formatted name" },
		{ B_CONTACT_GENDER, "Gender" },
		{ B_CONTACT_GEO, "Geographic Position" },
		{ B_CONTACT_GROUP, "Group" },
		{ B_CONTACT_IM, "Instant Messaging" },
		{ B_CONTACT_MANAGER, "Manager" },
		{ B_CONTACT_NAME, "Name" },
		{ B_CONTACT_NICKNAME, "Nickname" },
		{ B_CONTACT_NOTE, "Note" },
		{ B_CONTACT_ORGANIZATION, "Organization" },
		{ B_CONTACT_PHONE, "Phone" },
		{ B_CONTACT_PHOTO, "Photo" },
		{ B_CONTACT_PROTOCOLS, "Protocols" },
		{ B_CONTACT_SIMPLE_GROUP, "Simple Group" },
		{ B_CONTACT_SOUND, "Sound" },
		{ B_CONTACT_SPOUSE, "Spouse" },
		{ B_CONTACT_TIME_ZONE, "Time Zone" },
		{ B_CONTACT_TITLE, "Title" },
		{ B_CONTACT_URL, "Url" },
		{ B_CONTACT_REV, "Revision" },
		{ B_CONTACT_UID, "Haiku UID" },
		{ 0, 0 }
};

/*static standardFieldsMap gHiddenStandardFields[] = {
	{ B_CONTACT_UID, "Haiku UID" },
	{ 0, 0 }
};*/
