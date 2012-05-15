/*
 * Copyright 2005-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *      Dario Casalinuovo
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

#include "ContactFieldTextControl.h"

#include <Catalog.h>
#include <Font.h>
#include <GroupView.h>
#include <MenuField.h>
#include <LayoutBuilder.h>
#include <PopUpMenu.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "PersonView.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "People"


ContactFieldTextControl::ContactFieldTextControl(BContactField* field)
	:
	BTextControl("", field->Value(), NULL),
	fField(field)
{
	printf("ContactFieldTextControl field pointer %p\n", field);

	const char* label = 
		BContactField::ExtendedLabel(field->FieldType(), field->Usage());

	SetLabel(label);
}


ContactFieldTextControl::~ContactFieldTextControl()
{
}


void
ContactFieldTextControl::MouseDown(BPoint position)
{
	uint32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		buttons = Window()->CurrentMessage()->FindInt32("buttons");

	if (buttons == B_SECONDARY_MOUSE_BUTTON)
		_ShowPopUpMenu(ConvertToScreen(position));
}


void
ContactFieldTextControl::_ShowPopUpMenu(BPoint screen)
{
	BPopUpMenu* menu = new BPopUpMenu("PopUpMenu", this);

	BMessage* msg = new BMessage(M_REMOVE_FIELD);
	msg->AddPointer("fieldtextcontrol", this);

	BMenuItem* item = new BMenuItem("Remove field", msg);
	menu->AddItem(item);

	menu->SetTargetForItems(Parent()->Parent());
	menu->Go(screen, true, true, true);
}


bool
ContactFieldTextControl::HasChanged()
{
	return fField->Value() != Text();
}


void
ContactFieldTextControl::Reload()
{
	if (HasChanged())
		SetText(fField->Value());
}


void
ContactFieldTextControl::UpdateField()
{
	fField->SetValue(Text());
}


BString
ContactFieldTextControl::Value() const
{
	return fField->Value();
}


BContactField*
ContactFieldTextControl::Field() const
{
	return fField;
}
