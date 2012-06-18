/*
 * Copyright 2005-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Dario Casalinuovo
 */
#ifndef ADDRESS_VIEW_H
#define ADDRESS_VIEW_H

#include <GroupView.h>
#include <ObjectList.h>
#include <String.h>

enum {
	M_ADD_ADDRESS		= 'adar',
	M_REMOVE_ADDRESS	= 'rmad',
	M_SHOW_ADDRESS	= 'shad'
};


class AddressFieldView;
class BAddressContactField;
class BMenuField;
class BPopUpMenu;
class BTextControl;

class AddressView : public BGroupView {
public:
							AddressView();

	virtual					~AddressView();

	virtual	void			MessageReceived(BMessage* msg);
	
			bool			HasChanged();
			void			Reload();
			void			UpdateAddressField();

			void			AddAddress(BAddressContactField* field);
			void			RemoveAddress();
			void			SelectView(AddressFieldView* view);
			void			SelectView(int index);

private:
			BTextControl*	_AddControl(const char* label, const char* value);

			BMenuField*		fMenuField;
			BPopUpMenu* 	fLocationsMenu;

			BGroupView*		fFieldView;

			BObjectList<AddressFieldView> fFieldsList;
			AddressFieldView* fCurrentView;
};

#endif // ADDRESS_VIEW_H
