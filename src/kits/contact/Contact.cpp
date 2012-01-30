/*
 * Copyright 2011 Dario Casalinuovo
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <Contact.h>

#include <shared/AutoDeleter.h>
#include <Message.h>

#include <stdio.h>
#include <string.h>


BContact::BContact(BRawContact* contact)
	:
	fInitCheck(B_OK),
	fList(20, true)
{
	if (contact == NULL) {
		// Create a memory RawContact
		// because arg is NULL
		fRawContact = new BRawContact(B_CONTACT_FORMAT);
	} else {
		fRawContact = contact;
	}

	if (fRawContact->InitCheck() == B_OK) {
		BMessage msg;
		if (contact != NULL) {
			fRawContact->Read(&msg);

			msg.PrintToStream();

			_UnflattenFields(&msg);
			return;
		}
	}

	fInitCheck = B_ERROR;
}


BContact::BContact(BContactRef& contact)
	:
	fInitCheck(B_OK),
	fList(20, true)
{
	fInitCheck = B_ERROR;
}


BContact::BContact(BMessage* data)
	:
	fInitCheck(B_OK),
	fList(20, true)
{
	BMessage msg;
	fInitCheck = data->FindMessage("RawContact", &msg);

	fRawContact = new BRawContact(&msg);

	_UnflattenFields(data);
}


BContact::~BContact()
{
	delete fRawContact;
}


status_t
BContact::Archive(BMessage* archive, bool deep) const
{
	if (fInitCheck != B_OK)
		return fInitCheck;
	status_t ret;
	BArchivable::Archive(archive, deep);

	BMessage msg;

	ret = fRawContact->Archive(&msg, deep);
	if (ret != B_OK)
		return ret;

	archive->AddMessage("RawContact", &msg);

	_FlattenFields(archive);

	return B_OK;
}


BArchivable*
BContact::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "BContact"))
		return new BContact(data);

	return NULL;
}


status_t
BContact::InitCheck() const
{
	return fInitCheck;
}

/*
status_t
BContact::Append(BRawContact* contact)
{
	if (contact != NULL && 
		contact->InitCheck() == B_OK)
			fRawContact = contact;
}
*/

const BRawContact&
BContact::RawContact() const
{
	return *fRawContact;	
}


status_t
BContact::Append(BRawContact* contact)
{
	delete fRawContact;
	fRawContact = contact;

	return B_OK;
}


status_t
BContact::Reload()
{
	if (fRawContact->InitCheck() != B_OK)
		return B_ERROR;
		
	if (fList.CountItems() != 0) {
		fList.MakeEmpty();
	}
	BMessage msg;
	status_t ret = fRawContact->Read(&msg);

	msg.PrintToStream();

	if (ret == B_OK) {
		_UnflattenFields(&msg);
		return B_OK;
	}
	return ret;
}


status_t
BContact::Commit()
{
	if (fInitCheck != B_OK)
		return fInitCheck;

	BMessage msg;
	_FlattenFields(&msg);

	msg.PrintToStream();

	status_t ret = fRawContact->Commit(&msg);	
	if (ret != B_OK)
		printf("Translate %s\n", strerror(ret));

	return ret;
}


int32
BContact::ID()
{
	return 0;
	//return fID;
}


int32
BContact::GroupID()
{
	return 0;
	//return fGroupID;
}


status_t
BContact::AddField(BContactField* field)
{
	if (fInitCheck != B_OK)
		return fInitCheck;

	if (fList.AddItem(field))
		return B_OK;

	return B_ERROR;	
}


status_t
BContact::ReplaceField(BContactField* field)
{
	if (fInitCheck != B_OK)
		return fInitCheck;

	int count = fList.CountItems();
	for (int i = 0; i < count; i++) {
		BContactField* ret = fList.ItemAt(i);
		if (ret->IsEqual(field)) {
			if (fList.ReplaceItem(i, field))
				return B_OK;
			else
				return B_ERROR;
		}
	}
	return B_ERROR;
}


bool
BContact::HasField(BContactField* field)
{
	if (fInitCheck != B_OK)
		return fInitCheck;

	int count = fList.CountItems();
	BContactField* ret;
	for (int i = 0; i < count; i++) {
		ret = fList.ItemAt(i);
		if (ret->IsEqual(field))
			return true;
	}
	return B_ERROR;
}


// TODO improve performances sorting
// fList's objects by type
BContactField*
BContact::FieldAt(type_code type, int32 index)
{
	if (fInitCheck != B_OK)
		return NULL;

	BContactField* ret = fList.ItemAt(index);
	if (ret->TypeCode() == type)
		return ret;

	return NULL;
}


BContactField*
BContact::FieldAt(int index)
{
	if (fInitCheck != B_OK)
		return NULL;

	return fList.ItemAt(index);
}



int32
BContact::CountFields() const
{
	return fList.CountItems();
}


status_t
BContact::RemoveEquivalentFields(BContactField* field)
{
	if (fInitCheck != B_OK)
		return fInitCheck;

	BContactField* ret;
	for (int i = 0; i < fList.CountItems(); i++) {
		ret = fList.ItemAt(i);
		if (ret->IsEqual(field))
			fList.RemoveItemAt(i);
	}
	return B_OK;
}


status_t
BContact::RemoveField(BContactField* field)
{
	if (fInitCheck != B_OK)
		return fInitCheck;

	if (fList.RemoveItem(field) == 0)
		return B_NAME_NOT_FOUND;

	return B_OK;
}

const BContactFieldList&
BContact::FieldList() const
{
	return fList;
}
	

status_t
BContact::CopyFieldsFrom(BContact& contact)
{
	for (int i = 0; i < contact.CountFields(); i++) {
		BContactField* field = BContactField::Duplicate(contact.FieldAt(i));
		if (field == NULL)
			return B_ERROR;

		fList.AddItem(field);
	}
	return B_OK;
}


status_t
BContact::CreateDefaultFields()
{
	fList.AddItem(new BStringContactField(B_CONTACT_NAME));
	fList.AddItem(new BStringContactField(B_CONTACT_NICKNAME));
	fList.AddItem(new BStringContactField(B_CONTACT_EMAIL));
	fList.AddItem(new BStringContactField(B_CONTACT_ORGANIZATION));
//	fList.AddItem(new BStringContactField(B_CONTACT_IM));
	fList.AddItem(new BStringContactField(B_CONTACT_URL));
	BStringContactField* homePhone 
		= new BStringContactField(B_CONTACT_PHONE);
	
	homePhone->SetUsage(CONTACT_DATA_HOME);
	fList.AddItem(homePhone);

	BStringContactField* workPhone 
		= new BStringContactField(B_CONTACT_PHONE);
	workPhone->SetUsage(CONTACT_DATA_WORK);
	fList.AddItem(workPhone);

	BStringContactField* fax 
		= new BStringContactField(B_CONTACT_PHONE);
	fax->SetUsage(CONTACT_PHONE_FAX_WORK);
	fList.AddItem(fax);

	fList.AddItem(new BAddressContactField());

	return B_OK;
}


status_t
BContact::_FlattenFields(BMessage* msg) const
{
	int count = fList.CountItems();
	status_t ret;

	for (int i = 0; i < count; i++) {
		BContactField* object = fList.ItemAt(i);
		ssize_t size = object->FlattenedSize();

		void* buffer = new char[size];
		MemoryDeleter deleter(buffer);
		if (buffer == NULL)
			return B_NO_MEMORY;

		ret = object->Flatten(buffer, size);

		ret = msg->AddData(CONTACT_FIELD_IDENT, object->TypeCode(),
			buffer, size, false);
		if (ret != B_OK)
			return ret;
	}
	return B_OK;
}


status_t
BContact::_UnflattenFields(BMessage* msg)
{
	status_t ret;
	int32 count;
	type_code code = B_CONTACT_FIELD_TYPE;

	ret = msg->GetInfo(CONTACT_FIELD_IDENT, &code, &count);
	if (ret != B_OK)
		return ret;

	for (int i = 0; i < count; i++) {
		printf("%d\n", i);
		const void* data;
		ssize_t size;

		ret = msg->FindData(CONTACT_FIELD_IDENT, code,
			i, &data, &size);

		if (ret != B_OK)
			return ret;

		BContactField* field = BContactField::UnflattenChildClass(data, size);

		if (field) 
			fList.AddItem(field);
		else
			continue;
	}
	return ret;
}
