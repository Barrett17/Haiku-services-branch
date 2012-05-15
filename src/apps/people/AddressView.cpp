/*
 * Copyright 2010 Dario Casalinuovo <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "AddressView.h"

#include <ControlLook.h>
#include <ContactDefs.h>
#include <ContactField.h>
#include <LayoutBuilder.h>
#include <TextControl.h>

#include <stdio.h>

AddressView::AddressView(BAddressContactField* field)
	:
	BGridView("address view"),
	fStreet(NULL),
	fPostalBox(NULL),
	fNeighbor(NULL),
	fCity(NULL),
	fRegion(NULL),
	fPostalCode(NULL),
	fCountry(NULL),
	fCount(0),
	fField(field)
{
	SetFlags(Flags() | B_WILL_DRAW);

	float spacing = be_control_look->DefaultItemSpacing();
	GridLayout()->SetInsets(spacing, spacing, spacing, spacing);

	if (field != NULL) {
		fStreet = _AddControl("Street", field->Street());
		fPostalBox = _AddControl("PostalBox", field->PostalBox());
		fNeighbor = _AddControl("Neighborhood", field->Neighborhood());
		fCity = _AddControl("City", field->City());
		fRegion = _AddControl("Region", field->Region());
		fPostalCode = _AddControl("Postal Code", field->PostalCode());
		fCountry = _AddControl("Country", field->Country());
	}
}


AddressView::~AddressView()
{
}


bool
AddressView::HasChanged()
{
	if (fField == NULL)
		return false;

	if (fField->Street() != fStreet->Text())
		return true;

	if (fField->PostalBox() != fPostalBox->Text())
		return true;

	if (fField->Neighborhood() != fNeighbor->Text())
		return true;

	if (fField->City() != fCity->Text())
		return true;

	if (fField->Region() != fRegion->Text())
		return true;

	if (fField->PostalCode() != fPostalCode->Text())
		return true;

	if (fField->Country() != fCountry->Text())
		return true;

	return false;
}


void
AddressView::Reload()
{
	if (fField == NULL)
		return;

	fStreet->SetText(fField->Street());
	fPostalBox->SetText(fField->PostalBox());
	fNeighbor->SetText(fField->Neighborhood());
	fCity->SetText(fField->City());
	fRegion->SetText(fField->Region());
	fPostalCode->SetText(fField->PostalCode());
	fCountry->SetText(fField->Country());
}


void
AddressView::UpdateAddressField()
{
	if (fField == NULL)
		return;

	fField->SetStreet(fStreet->Text());
	fField->SetPostalBox(fPostalBox->Text());
	fField->SetNeighborhood(fNeighbor->Text());
	fField->SetCity(fCity->Text());
	fField->SetRegion(fRegion->Text());
	fField->SetPostalCode(fPostalCode->Text());
	fField->SetCountry(fCountry->Text());
}


BString
AddressView::Value() const
{
	if (fField == NULL)
		return "";

	return fField->Value();
}

/*
 * use BLayoutBuilder::Grid::AddTextView(), it's prettier!
 */

BTextControl*
AddressView::_AddControl(const char* label, const char* value)
{
	BTextControl* control = new BTextControl(label, value, NULL);

	BGridLayout* layout = GridLayout();
	int count = layout->CountRows();

	BLayoutItem* item = control->CreateLabelLayoutItem();
	item->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	layout->AddItem(item, 0, count);
	layout->AddItem(control->CreateTextViewLayoutItem(), 1, count);

	return control;
}


BAddressContactField*
AddressView::Field() const
{
	return fField;
}
