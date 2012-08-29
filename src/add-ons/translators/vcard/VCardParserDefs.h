/*
 * Copyright 2010-2012 Casalinuovo Dario
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _VCARD_PARSER_DEFS_H
#define _VCARD_PARSER_DEFS_H
#include <ContactField.h>

// VCard 2.0 standard fields

#define VCARD_ADDRESS              "ADR"
#define VCARD_BIRTHDAY             "BDAY"
#define VCARD_DELIVERY_LABEL       "LABEL"
#define VCARD_EMAIL                "EMAIL"
#define VCARD_FORMATTED_NAME       "FN"
#define VCARD_REVISION             "REV"
#define VCARD_SOUND                "SOUND"
#define VCARD_TELEPHONE            "TEL"
#define VCARD_TIME_ZONE            "TZ"
#define VCARD_TITLE                "TITLE"
#define VCARD_URL                  "URL"
#define VCARD_GEOGRAPHIC_POSITION  "GEO"
#define VCARD_NAME                 "N"
#define VCARD_NICKNAME             "NICKNAME"
#define VCARD_NOTE                 "NOTE"
#define VCARD_ORGANIZATION         "ORG"
#define VCARD_PHOTO                "PHOTO"
#define VCARD_VERSION              "VERSION"

// Currently unsupported fields

#define VCARD_AGENT                "AGENT"
#define VCARD_CATEGORIES           "CATEGORIES"
#define VCARD_CLASS                "CLASS"
#define VCARD_ROLE                 "ROLE"
#define VCARD_SORT_STRING          "SORT-STRING"
#define VCARD_KEY                  "KEY"
#define VCARD_LOGO                 "LOGO"
#define VCARD_MAILER               "MAILER"
#define VCARD_PRODUCT_IDENTIFIER   "PRODID"

// Custom Haiku OS extensions

#define X_VCARD_IM					"X-HAIKU-IM"
#define X_VCARD_PROTOCOLS			"X-HAIKU-PROTOCOLS"
#define X_VCARD_SIMPLE_GROUP		"X-HAIKU-SIMPLEGROUP"
#define X_VCARD_GROUP				"X-HAIKU-GROUP"
#define X_VCARD_UID					"X-HAIKU-UID"

// Non-Haiku custom fields


// VCard <-> BContact defs
struct fieldMap {
	const char* key;
	field_type type;
};

// This is a translation table
// that will be used to fill a map with the purpose to convert
// fields from BContact to VCard.
static fieldMap gFieldsMap[] = {
		{ VCARD_ADDRESS, B_CONTACT_ADDRESS },
		{ VCARD_BIRTHDAY, B_CONTACT_BIRTHDAY },
 		{ VCARD_DELIVERY_LABEL, B_CONTACT_DELIVERY_LABEL },
		{ VCARD_FORMATTED_NAME, B_CONTACT_FORMATTED_NAME },
		{ VCARD_EMAIL, B_CONTACT_EMAIL },
		{ VCARD_REVISION, B_CONTACT_REV },
		{ VCARD_SOUND, B_CONTACT_SOUND },
		{ VCARD_TELEPHONE, B_CONTACT_PHONE },
		{ VCARD_TIME_ZONE, B_CONTACT_TIME_ZONE },
		{ VCARD_TITLE, B_CONTACT_TITLE },
		{ VCARD_URL, B_CONTACT_URL },
		{ VCARD_GEOGRAPHIC_POSITION, B_CONTACT_GEO },
		{ VCARD_NAME, B_CONTACT_NAME },
		{ VCARD_NICKNAME, B_CONTACT_NICKNAME },
		{ VCARD_NOTE, B_CONTACT_NOTE },
		{ VCARD_ORGANIZATION, B_CONTACT_ORGANIZATION },
		{ VCARD_PHOTO, B_CONTACT_PHOTO },

		// Custom VCard Haiku fields
		{ X_VCARD_IM, B_CONTACT_IM },
		{ X_VCARD_PROTOCOLS, B_CONTACT_PROTOCOLS },
		{ X_VCARD_SIMPLE_GROUP, B_CONTACT_SIMPLE_GROUP },
		{ X_VCARD_GROUP, B_CONTACT_GROUP },
		{ X_VCARD_UID, B_CONTACT_UID },
		
		{ NULL, 0 }
};

#endif
