/*
 * Copyright 2010-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Stephan Aßmus <superstippi@gmx.de>
 * 	 	Dario Casalinuovo
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "PersonView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <Box.h>
#include <ControlLook.h>
#include <File.h>
#include <GridLayout.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <TabView.h>
#include <TranslationUtils.h>
#include <Translator.h>
#include <Window.h>

#include "ContactFieldTextControl.h"
#include "AddressView.h"
#include "PictureView.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "People"

struct ContactVisitor : public BContactFieldVisitor {
public:
					ContactVisitor(PersonView* owner)
					:
					fOwner(owner)
					{
					}
	virtual		 	~ContactVisitor()
					{
					}

	virtual void 	Visit(BStringContactField* field)
	{
		ContactFieldTextControl* control = new ContactFieldTextControl(field);

		BGridLayout* layout = fOwner->fGeneralView->GridLayout();
		int count = layout->CountRows();

		BLayoutItem* item = control->CreateLabelLayoutItem();
		BAlignment align(B_ALIGN_LEFT, B_ALIGN_TOP);
		item->SetExplicitAlignment(align);

		layout->AddItem(item, 0, count);
		item = control->CreateTextViewLayoutItem();
		item->SetExplicitMinSize(BSize(10, B_SIZE_UNSET));
		layout->AddItem(item, 1, count);

		fOwner->fControls.AddItem(control);
	}

	virtual void 	Visit(BAddressContactField* field)
	{
		if (fOwner->fAddrView != NULL 
			&& fOwner->fAddrView->Field()->Value() == field->Value())
				return;

		fOwner->fAddrView = new AddressView(field);

		BTab* tab = new BTab();
		fOwner->fTabView->AddTab(fOwner->fAddrView, tab);
		tab->SetLabel("Location");
	}

	virtual void 	Visit(BPhotoContactField* field)
	{
		if (field != NULL) {
			BBitmap* bitmap = field->Photo();
			fOwner->UpdatePicture(bitmap);
		}
	}

private:
	PersonView* fOwner;
};


PersonView::PersonView(const char* name, BContact* contact)
	:
	BGridView(B_VERTICAL),
//	fGroups(NULL),
	fControls(20, false),
	fAddrView(NULL),
	fPictureView(NULL),
	fSaving(false),
	fSaved(true),
	fContact(contact),
	fPhotoField(NULL)
	//fFile(file)
{
	SetName(name);
	SetFlags(Flags() | B_WILL_DRAW);

	UpdatePicture(NULL);

	float spacing = be_control_look->DefaultItemSpacing();
	GridLayout()->SetInsets(spacing);

	fGeneralView = new BGridView("GeneralView");
	fGeneralView->GridLayout()->
		SetInsets(spacing, spacing, spacing, spacing);

	fTabView = new BTabView("tab_view");

	fGeneralView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));

	BTab* tab = new BTab();
	fTabView->AddTab(fGeneralView, tab);
	tab->SetLabel("General");

	GridLayout()->AddView(fTabView, 0, GridLayout()->CountRows());

	_LoadFieldsFromContact();
}


PersonView::~PersonView()
{
	delete fContact;
	// TODO memory leaks
}


void
PersonView::AddField(BContactField* field)
{
	if (field == NULL)
		return;

	ContactVisitor visitor(this);

	field->Accept(&visitor);
}


void
PersonView::MakeFocus(bool focus)
{
	if (focus && fControls.CountItems() > 0)
		fControls.ItemAt(0)->MakeFocus();
	else
		BView::MakeFocus(focus);
}


void
PersonView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case M_SAVE:
			Save();
			break;

		case M_REVERT:
			if (fPictureView)
				fPictureView->Revert();

			if (fAddrView)
				fAddrView->Reload();

			for (int32 i = fControls.CountItems() - 1; i >= 0; i--)
				fControls.ItemAt(i)->Reload();
			break;

		case M_SELECT:
			for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
				BTextView* text = fControls.ItemAt(i)->TextView();
				if (text->IsFocus()) {
					text->Select(0, text->TextLength());
					break;
				}
			}
			break;

		case M_GROUP_MENU:
		{
			/*const char* name = NULL;
			if (msg->FindString("group", &name) == B_OK)
				SetAttribute(fCategoryAttribute, name, false);*/
			break;
		}

		case M_ADD_FIELD:
		{
			msg->PrintToStream();
			field_type type;
			if (msg->FindInt32("field_type", (int32*)&type) == B_OK) {
				BContactField* contactField = BContactField::InstantiateChildClass(type);
				fContact->AddField(contactField);
				AddField(contactField);
				fSaved = false;
			}
			break;
		}

		case M_REMOVE_FIELD:
		{
			ContactFieldTextControl* control;
			if (msg->FindPointer("fieldtextcontrol", (void**)&control) == B_OK) {
				_RemoveField(control);
				fSaved = false;
			}
			break;
		}
	}
}


void
PersonView::Draw(BRect updateRect)
{
	if (!fPictureView)
		return;

	// Draw a alert/get info-like strip
	BRect stripeRect = Bounds();
	stripeRect.right = GridLayout()->HorizontalSpacing()
		+ fPictureView->Bounds().Width() / 2;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);
}


/*
void
PersonView::BuildGroupMenu()
{
	if (fGroups == NULL)
		return;

	BMenuItem* item;
	while ((item = fGroups->ItemAt(0)) != NULL) {
		fGroups->RemoveItem(item);
		delete item;
	}

	int32 count = 0;

	BVolumeRoster volumeRoster;
	BVolume volume;
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		BQuery query;
		query.SetVolume(&volume);

		char buffer[256];
		snprintf(buffer, sizeof(buffer), "%s=*", fCategoryAttribute.String());
		query.SetPredicate(buffer);
		query.Fetch();

		BEntry entry;
		while (query.GetNextEntry(&entry) == B_OK) {
			BFile file(&entry, B_READ_ONLY);
			attr_info info;

			if (file.InitCheck() == B_OK
				&& file.GetAttrInfo(fCategoryAttribute, &info) == B_OK
				&& info.size > 1) {
				if (info.size > sizeof(buffer))
					info.size = sizeof(buffer);

				if (file.ReadAttr(fCategoryAttribute.String(), B_STRING_TYPE,
						0, buffer, info.size) < 0) {
					continue;
				}

				const char *text = buffer;
				while (true) {
					char* offset = strstr(text, ",");
					if (offset != NULL)
						offset[0] = '\0';

					if (!fGroups->FindItem(text)) {
						int32 index = 0;
						while ((item = fGroups->ItemAt(index)) != NULL) {
							if (strcmp(text, item->Label()) < 0)
								break;
							index++;
						}
						BMessage* message = new BMessage(M_GROUP_MENU);
						message->AddString("group", text);
						fGroups->AddItem(new BMenuItem(text, message), index);
						count++;
					}
					if (offset) {
						text = offset + 1;
						while (*text == ' ')
							text++;
					}
					else
						break;
				}
			}
		}
	}

	if (count == 0) {
		fGroups->AddItem(item = new BMenuItem(
			B_TRANSLATE_WITH_CONTEXT("none", "Groups list"),
			new BMessage(M_GROUP_MENU)));
		item->SetEnabled(false);
	}

	fGroups->SetTargetForItems(this);
}*/


void
PersonView::CreateFile(const entry_ref* ref, int32 format)
{
	fContact->Append(new BRawContact(format, new BFile(ref,
		B_READ_WRITE | B_CREATE_FILE)));

	Save();
}


bool
PersonView::IsSaved() const
{
	if (!fSaved)
		return false;

	if (fPictureView && fPictureView->HasChanged())
		return false;

	if (fAddrView != NULL && fAddrView->HasChanged())
		return false;

	for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
		if (fControls.ItemAt(i)->HasChanged())
			return false;
	}

	return true;
}


void
PersonView::Save()
{
	fSaving = true;

	int32 count = fControls.CountItems();
	for (int32 i = 0; i < count; i++) {
		ContactFieldTextControl* control = fControls.ItemAt(i);
		control->UpdateField();
	}

	if (fAddrView != NULL)
		fAddrView->UpdateAddressField();

	if (fPictureView && fPictureView->HasChanged()) {
		fPictureView->Update();
		BBitmap* bitmap = fPictureView->Bitmap();

		if (fPhotoField == NULL) {
			fPhotoField = new BPhotoContactField(bitmap);
			fContact->AddField(fPhotoField);
		} else {
			fPhotoField->SetPhoto(bitmap);
		}
	}

	if (fContact->Commit() != B_OK) {
		BAlert* panel = new BAlert( "", "Error : BContact::Commit() != B_OK!\n",
			"OK");
		panel->Go();
	}
	fSaving = false;
	fSaved = true;
}


void
PersonView::Reload(const entry_ref* ref)
{
	if (fReloading == true)
		return;
	else
		fReloading = true;

	BAlert* panel = new BAlert( "",
		"The file has been modified by another application\n"
		"Do you want to reload it?\n\n",
		"Reload", "No");

	int32 index = panel->Go();
	if (index == 1) {
		fSaved = false;
		fReloading = false;
		return;
	}

	if (ref != NULL) {
		fContact->Append(new BRawContact(B_CONTACT_ANY,
			new BFile(ref, B_READ_WRITE)));
	}

	fContact->Reload();

	for (int i = 0; i < fControls.CountItems(); i++) {
		ContactFieldTextControl* control = fControls.ItemAt(i);
		fControls.RemoveItem(control);
		fGeneralView->RemoveChild(control);
		delete control;
	}
	_LoadFieldsFromContact();
	fSaved = true;
	fReloading = false;
}


void
PersonView::UpdatePicture(BBitmap* bitmap)
{
	if (fPictureView == NULL) {
		fPictureView = new PictureView(70, 90, bitmap);

		GridLayout()->AddView(fPictureView, 0, 0, 1, 5);
		GridLayout()->ItemAt(0, 0)->SetExplicitAlignment(
			BAlignment(B_ALIGN_CENTER, B_ALIGN_TOP));
		return;
	}

	// TODO check if it's really needed
	if (fSaving)
		return;

	fPictureView->Update(bitmap);
}


bool
PersonView::IsTextSelected() const
{
	for (int32 i = fControls.CountItems() - 1; i >= 0; i--) {
		BTextView* text = fControls.ItemAt(i)->TextView();

		int32 start, end;
		text->GetSelection(&start, &end);
		if (start != end)
			return true;
	}
	return false;
}


BContact*
PersonView::GetContact() const
{
	return fContact;
}


void
PersonView::_LoadFieldsFromContact()
{
	if (fContact->CountFields() == 0)
		fContact->CreateDefaultFields();

	for (int i = 0; i < fContact->CountFields(); i++) {
		BContactField* field = fContact->FieldAt(i);
		if (field != NULL)
			AddField(field);
	}
}


void
PersonView::_RemoveField(ContactFieldTextControl* control)
{
	fControls.RemoveItem(control);
	fGeneralView->RemoveChild(control);
	fContact->RemoveField(control->Field());
	delete control;
}
