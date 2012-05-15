/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 * Dario Casalinuovo
 */
#ifndef CONTACT_WATCHER_H
#define CONTACT_WATCHER_H

#include <ContactDefs.h>
#include <Contact.h>
#include <ContactField.h>
#include <File.h>
#include <Looper.h>
#include <ObjectList.h>

enum {
	B_CONTACT_REMOVED = 'BcRm',
	B_CONTACT_MOVED = 'BcMv',
	B_CONTACT_MODIFIED = 'BcMd'
};


class BContactWatcher : public BLooper {
public:
								BContactWatcher(const entry_ref* ref);
	virtual						~BContactWatcher();

	virtual void				MessageReceived(BMessage* msg);
			void				WatchChanges(bool doIt);
			void				SetMessenger(BMessenger* messenger);
			void				SetRef(const entry_ref* ref);
			BFile*				GetFile();
private:
			const entry_ref*	fRef;
			BFile*				fFile;
			BMessenger*			fMessenger;
};

#endif
