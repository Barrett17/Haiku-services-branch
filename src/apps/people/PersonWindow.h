/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PERSON_WINDOW_H
#define PERSON_WINDOW_H


#include <Contact.h>
#include <String.h>
#include <Window.h>

#include "ContactWatcher.h"

#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			420
#define WIND_HEIGHT			340


class PersonView;
class BFilePanel;
class BMenuItem;


class PersonWindow : public BWindow {
public:

								PersonWindow(BRect frame,
									const char* title,
									const entry_ref* ref,
									BContact* contact);
	virtual						~PersonWindow();

	virtual	void				MenusBeginning();
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();
	virtual	void				Show();

			void				SaveAs(int32 format = 0);

			bool				RefersPersonFile(const entry_ref& ref) const;
private:
			const char* 		_FieldLabel(field_type code) const;
			void				_GetDefaultFileName(char* name);
			void				_SetToRef(entry_ref* ref);

			BFilePanel*			fPanel;
			BMenuItem*			fCopy;
			BMenuItem*			fCut;
			BMenuItem*			fPaste;
			BMenuItem*			fRevert;
			BMenuItem*			fSave;
			BMenuItem*			fUndo;
			PersonView*			fView;
			const entry_ref*	fRef;
			BContactWatcher*	fWatcher;
};


#endif // PERSON_WINDOW_H
