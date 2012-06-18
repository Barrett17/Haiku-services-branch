/*
 * Copyright 2011 - 2012 Dario Casalinuovo.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <ContactGroup.h>

#include <Contact.h>

//TODO define error codes

BContactGroup::BContactGroup(uint32 groupID, bool custom)
	:
	fGroupID(groupID),
	fCustom(custom)
{

}


BContactGroup::~BContactGroup()
{
}



bool
BContactGroup::IsFixedSize() const
{ 
	return false;
}


type_code
BContactGroup::TypeCode() const
{
	return B_CONTACT_GROUP_TYPE;
}


bool
BContactGroup::AllowsTypeCode(type_code code) const
{
	if (code != B_CONTACT_GROUP_TYPE)
		return false;
	
	return true;
}


ssize_t
BContactGroup::FlattenedSize() const
{
	ssize_t size = sizeof(type_code);

	return size;
}


status_t
BContactGroup::Flatten(BPositionIO* flatData) const
{
	if (flatData == NULL)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
BContactGroup::Flatten(void* buffer, ssize_t size) const
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, size);
	return Flatten(&flatData, size);
}


status_t
BContactGroup::Unflatten(type_code code,
	const void* buffer,	ssize_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	BMemoryIO flatData(buffer, size);
	return Unflatten(code, &flatData);
}


status_t
BContactGroup::Unflatten(type_code code, BPositionIO* flatData)
{
	if (code != B_CONTACT_FIELD_TYPE)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
BContactGroup::InitCheck() const
{
	return fInitCheck;
}


status_t
BContactGroup::AddContact(BContactRef contact)
{
//	fList.AddItem(contact);
	return B_OK;
}


status_t
BContactGroup::RemoveContact(BContactRef contact)
{
//	fList.RemoveItem(contact);
	return B_OK;
}


const
BContactRefList*
BContactGroup::AllContacts() const
{
	return fList;
}

/*
const
BContactRefList&
BContactGroup::ContactsByField(ContactFieldType type, 
	const char* value) const
{
	BContactRefList* ret = new BContactRefList();

	return *ret;
}*/


BMessage*
BContactGroup::ToMessage()
{
	BMessage* msg = new BMessage();

	return msg;
}
