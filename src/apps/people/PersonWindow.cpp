/*
 * Copyright 2005-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Dario Casalinuovo
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

#include "PersonWindow.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <ContactField.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <String.h>
#include <TextView.h>
#include <TranslatorFormats.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "PeopleApp.h"
#include "PersonView.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "People"


PersonWindow::PersonWindow(BRect frame, const char* title,
	const entry_ref* ref, BContact* contact)
	:
	BWindow(frame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE
		| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS, B_WILL_DRAW),
	fPanel(NULL),
	fRef(ref)
{
	BMenu* menu;
	BMenuItem* item;

	BMenuBar* menuBar = new BMenuBar("");
	menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("New contact" B_UTF8_ELLIPSIS),
		new BMessage(M_NEW), 'N'));
	item->SetTarget(be_app);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W'));
	menu->AddSeparatorItem();
	menu->AddItem(fSave = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(M_SAVE), 'S'));
	fSave->SetEnabled(FALSE);
	
	BMenu* saveMenu = new BMenu(B_TRANSLATE("Save as"));

	BMessage* msg = new BMessage(M_SAVE_AS);
	msg->AddInt32("format", B_PERSON_FORMAT);
	saveMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Person file"), msg));

	msg = new BMessage(M_SAVE_AS);
	msg->AddInt32("format", B_CONTACT_FORMAT);
	saveMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Haiku binary contact"), msg));

	msg = new BMessage(M_SAVE_AS);
	msg->AddInt32("format", B_VCARD_FORMAT);
	saveMenu->AddItem(new BMenuItem(
		B_TRANSLATE("vCard 2.0"), msg));

	menu->AddItem(saveMenu);
	menu->AddItem(fRevert = new BMenuItem(B_TRANSLATE("Revert"),
		new BMessage(M_REVERT), 'R'));
	fRevert->SetEnabled(FALSE);
	menu->AddSeparatorItem();
	item = new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);
	menuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Edit"));
	menu->AddItem(fUndo = new BMenuItem(B_TRANSLATE("Undo"),
		new BMessage(B_UNDO), 'Z'));
	fUndo->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	menu->AddItem(fCopy = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
	menu->AddItem(fPaste = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE), 'V'));
	BMenuItem* selectAllItem = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(M_SELECT), 'A');
	menu->AddItem(selectAllItem);

	// TODO edit it when the mixed person/vcard format will be added
	if (contact->RawContact().FinalFormat() == B_PERSON_FORMAT) {
		menu->AddSeparatorItem();
		menu->AddItem(item = new BMenuItem(B_TRANSLATE("Configure attributes"),
			new BMessage(M_CONFIGURE_ATTRIBUTES), 'F'));
		item->SetTarget(be_app);
	}
	menu->AddSeparatorItem();

	BMenu* fieldMenu = new BMenu(B_TRANSLATE("Add field"));

	int count;
	field_type* supportedFields = BContact::SupportedFields(&count, false);
	for (int i = 0; i < count; i++) {
		BMessage* msg = new BMessage(M_ADD_FIELD);
		msg->AddInt32("field_type", supportedFields[i]);

		BMenuItem* field = new BMenuItem(
			B_TRANSLATE(_FieldLabel(supportedFields[i])), msg);
		fieldMenu->AddItem(field);
	}
	menu->AddItem(new BMenuItem(fieldMenu));
	menuBar->AddItem(menu);

	if (ref != NULL) {
		SetTitle(ref->name);
		fRef = new entry_ref(*ref);
		fWatcher = new BContactWatcher(fRef);
		fWatcher->WatchChanges(true);
		fWatcher->SetMessenger(new BMessenger(this)); 
	}

	fView = new PersonView("PeopleView", contact);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, 0, 0, 0)
		.Add(menuBar)
		.Add(fView);

	fRevert->SetTarget(fView);
	selectAllItem->SetTarget(fView);

}


PersonWindow::~PersonWindow()
{
}


void
PersonWindow::MenusBeginning()
{
	bool enabled = !fView->IsSaved();
	fSave->SetEnabled(enabled);
	fRevert->SetEnabled(enabled);

	bool isRedo = false;
	bool undoEnabled = false;
	bool cutAndCopyEnabled = false;

	BTextView* textView = dynamic_cast<BTextView*>(CurrentFocus());
	if (textView != NULL) {
		undo_state state = textView->UndoState(&isRedo);
		undoEnabled = state != B_UNDO_UNAVAILABLE;

		cutAndCopyEnabled = fView->IsTextSelected();
	}

	if (isRedo)
		fUndo->SetLabel(B_TRANSLATE("Redo"));
	else
		fUndo->SetLabel(B_TRANSLATE("Undo"));

	fUndo->SetEnabled(undoEnabled);
	fCut->SetEnabled(cutAndCopyEnabled);
	fCopy->SetEnabled(cutAndCopyEnabled);

	be_clipboard->Lock();
	fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE));
	be_clipboard->Unlock();

//	fView->BuildGroupMenu();
}


void
PersonWindow::MessageReceived(BMessage* msg)
{
	char			str[256];
	BDirectory		directory;
	BEntry			entry;
	BFile			file;
//	BNodeInfo		*node;

	switch (msg->what) {
		case M_SAVE:
			if (!fRef) {
				SaveAs();
				break;
			}
			// supposed to fall through
		case M_REVERT:
		case M_SELECT:
			fView->MessageReceived(msg);
			break;

		case M_SAVE_AS:
			int32 format;
			if (msg->FindInt32("format", &format) == B_OK)
				SaveAs(format);
			break;

		case M_ADD_FIELD:
			fView->MessageReceived(msg);
			break;
		case B_UNDO: // fall through
		case B_CUT:
		case B_COPY:
		case B_PASTE:
		{
			BView* view = CurrentFocus();
			if (view != NULL)
				view->MessageReceived(msg);
			break;
		}

		case B_SAVE_REQUESTED:
		{
			entry_ref dir;
			if (msg->FindRef("directory", &dir) == B_OK) {
				const char* name = NULL;
				msg->FindString("name", &name);
				directory.SetTo(&dir);
				if (directory.InitCheck() == B_NO_ERROR) {
					directory.CreateFile(name, &file);
					if (file.InitCheck() == B_NO_ERROR) {
						int32 format;
						if (msg->FindInt32("format", &format) == B_OK) {
							directory.FindEntry(name, &entry);
							entry.GetRef(&dir);
							_SetToRef(new entry_ref(dir));
							SetTitle(fRef->name);
							fView->CreateFile(fRef, format);
						}
					}
					else {
						sprintf(str, B_TRANSLATE("Could not create %s."), name);
						(new BAlert("", str, B_TRANSLATE("Sorry")))->Go();
					}
				}
			}
			break;
		}

		case B_CONTACT_REMOVED:
			// We lost our file. Close the window.
			PostMessage(B_QUIT_REQUESTED);
			break;
		
		case B_CONTACT_MOVED:
		{
			// We may have renamed our entry. Obtain relevant data
			// from message.
			BString name;
			msg->FindString("name", &name);

			int64 directory;
			msg->FindInt64("to directory", &directory);

			int32 device;
			msg->FindInt32("device", &device);

			// Update our file
			fRef = new entry_ref(device,
				directory, name.String());
			fWatcher->SetRef(fRef);
			fView->Reload(fRef);

			// And our window title.
			SetTitle(name);

			// If moved to Trash, close window.
			BVolume volume(device);
			BPath trash;
			find_directory(B_TRASH_DIRECTORY, &trash, false,
				&volume);
			BPath folder(fRef);
			folder.GetParent(&folder);
			if (folder == trash)
				be_app->PostMessage(B_QUIT_REQUESTED);	
			break;
		}
		
		case B_CONTACT_MODIFIED:
		{
			fView->Reload();
			break;
		}

		default:
			BWindow::MessageReceived(msg);
	}
}


bool
PersonWindow::QuitRequested()
{
	status_t result;

	if (!fView->IsSaved()) {
		result = (new BAlert("", B_TRANSLATE("Save changes before quitting?"),
							B_TRANSLATE("Cancel"), B_TRANSLATE("Quit"),
							B_TRANSLATE("Save")))->Go();
		if (result == 2) {
			if (fRef) {
				fView->Save();
			} else {
				SaveAs();
				return false;
			}
		} else if (result == 0)
			return false;
	}

	delete fPanel;

	BMessage message(M_WINDOW_QUITS);
	message.AddRect("frame", Frame());
	if (be_app->Lock()) {
		be_app->PostMessage(&message);
		be_app->Unlock();
	}

	return true;
}


void
PersonWindow::Show()
{
	fView->MakeFocus();
	BWindow::Show();
}


void
PersonWindow::SaveAs(int32 format)
{
	if (format == 0)
		format = B_PERSON_FORMAT;

	char name[B_FILE_NAME_LENGTH];
	_GetDefaultFileName(name);

	if (fPanel == NULL) {
		BPath path;
		BMessenger target(this);
		BMessage msg(B_SAVE_REQUESTED);
		msg.AddInt32("format", format);
		fPanel = new BFilePanel(B_SAVE_PANEL, &target, NULL, 0, true, &msg);
		

		find_directory(B_USER_DIRECTORY, &path, true);

		BDirectory dir;
		dir.SetTo(path.Path());

		BEntry entry;
		if (dir.FindEntry("people", &entry) == B_OK
			|| (dir.CreateDirectory("people", &dir) == B_OK
					&& dir.GetEntry(&entry) == B_OK)) {
			fPanel->SetPanelDirectory(&entry);
		}
	}

	if (fPanel->Window()->Lock()) {
		fPanel->SetSaveText(name);
		if (fPanel->Window()->IsHidden())
			fPanel->Window()->Show();
		else
			fPanel->Window()->Activate();
		fPanel->Window()->Unlock();
	}
}


bool
PersonWindow::RefersPersonFile(const entry_ref& ref) const
{
	if (fRef == NULL)
		return false;
	return *fRef == ref;
}


void
PersonWindow::_GetDefaultFileName(char* name)
{
	if (fRef == NULL) {
		strncpy(name, "No Name", B_FILE_NAME_LENGTH);
		return;
	}
	strncpy(name, fRef->name, B_FILE_NAME_LENGTH);
}


void
PersonWindow::_SetToRef(entry_ref* ref)
{
	if (fRef != NULL) {
		fWatcher->WatchChanges(false);
	}

	fRef = ref;

	fWatcher->WatchChanges(true);
	fWatcher->SetRef(fRef);
}


const char*
PersonWindow::_FieldLabel(field_type code) const
{
	return BContactField::SimpleLabel(code);
}
