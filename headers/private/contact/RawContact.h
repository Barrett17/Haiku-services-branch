/*
 * Copyright 2010 Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef B_RAW_CONTACT_H
#define B_RAW_CONTACT_H

#include <Archivable.h>
#include <Message.h>
#include <TranslatorFormats.h>
#include <TranslatorRoster.h>

class BRawContact : public BArchivable {
public:
					// Initialize the RawContact from
					// a BMessage if it's valid.
					BRawContact(BMessage* data);

					BRawContact(uint32 finalFormat = 0,
						BPositionIO* destination = NULL);

	// TODO add a entry_ref constructor

					~BRawContact();

	status_t		Archive(BMessage* data, bool deep);

	static BArchivable*	Instantiate(BMessage* data);

	status_t		InitCheck() const;

	status_t		Commit(BMessage* data);
	status_t		Read(BMessage* data);
	int32			FinalFormat() const;

	BPositionIO*	Destination() const;
	status_t		SetDestination(BPositionIO* destination, bool del);
private:
	void			_Init();
	void			_InitTranslator();
	bool			_CheckDestination(BPositionIO* destination);
	status_t		_FindFormat();

	BPositionIO*	fDest;

	status_t		fInitCheck;
	// translation stuff
	uint32			fFormat;
	// the id of the suitable translator
	translator_id	fTranslatorID;

	BTranslatorRoster* fRoster;
};

#endif // B_RAW_CONTACT_H