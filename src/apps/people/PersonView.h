/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Stephan Aßmus <superstippi@gmx.de>
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PERSON_VIEW_H
#define PERSON_VIEW_H


#include <ContactDefs.h>
#include <Contact.h>
#include <ContactField.h>
#include <GridView.h>
#include <GroupView.h>
#include <ObjectList.h>
#include <String.h>
#include <TextControl.h>

class AddressView;
class ContactFieldTextControl;
class BBox;
class BFile;
class BPopUpMenu;
class BTabView;
class PictureView;

enum {
	M_SAVE			= 'save',
	M_REVERT		= 'rvrt',
	M_SELECT		= 'slct',
	M_GROUP_MENU	= 'grmn',
	M_ADD_FIELD		= 'adfl',
	M_REMOVE_FIELD	= 'rmfd'
};


class ContactVisitor;

class PersonView : public BGridView {
public:
								PersonView(const char* name,
									BContact* contact);
	virtual						~PersonView();

	virtual	void				MakeFocus(bool focus = true);
	virtual	void				MessageReceived(BMessage* message);
	virtual void				Draw(BRect updateRect);

//			void				BuildGroupMenu();

			void				CreateFile(const entry_ref* ref, int32 format);

			bool				IsSaved() const;
			void				Save();

			void				AddField(BContactField* field);

			void				UpdatePicture(BBitmap* bitmap);
			void				Reload(const entry_ref* ref = NULL);

			bool				IsTextSelected() const;

			BContact*			GetContact() const;

private:
			void				_LoadFieldsFromContact();
			void				_RemoveField(ContactFieldTextControl* control);
//			time_t				fLastModificationTime;
//			BPopUpMenu*			fGroups;

			typedef BObjectList<ContactFieldTextControl> FieldList;
			FieldList			fControls;

			AddressView*		fAddrView;

//			BString				fCategoryAttribute;
			PictureView*		fPictureView;
			bool				fSaving;
			bool				fSaved;
			bool				fReloading;

			BContact*			fContact;

			BPhotoContactField*	fPhotoField;

			BGridView*			fGeneralView;
			BTabView* 			fTabView;

	friend class ContactVisitor;
};

#endif // PERSON_VIEW_H
