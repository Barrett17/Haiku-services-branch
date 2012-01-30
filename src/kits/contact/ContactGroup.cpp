/*
 * Copyright 2011 Dario Casalinuovo <your@email.address>
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


status_t
BContactGroup::InitCheck() const
{
	return fInitCheck;
}


status_t
BContactGroup::AddContact(BContactRef* contact)
{
//	fList.AddItem(contact);
	return B_OK;
}


status_t
BContactGroup::RemoveContact(BContactRef* contact)
{
//	fList.RemoveItem(contact);
	return B_OK;
}


const
BContactRefList&
BContactGroup::AllContacts() const
{
	return fList;
}


const
BContactRefList&
BContactGroup::ContactsByField(ContactFieldType type, 
	const char* value) const
{
	BContactRefList* ret = new BContactRefList();

	return *ret;
}
