/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *      Dario Casalinuovo
 */

#include "ContactWatcher.h"

#include <Entry.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>

#include <stdio.h>


BContactWatcher::BContactWatcher(const entry_ref* ref)
	:
	BLooper(),
	fRef(ref),
	fMessenger(NULL)
{
	fFile = new BFile(ref, B_READ_WRITE);
	Run();
}


BContactWatcher::~BContactWatcher()
{
	delete fFile;
	delete fRef;
	delete fMessenger;
}


void
BContactWatcher::SetMessenger(BMessenger* messenger)
{
	delete fMessenger;
	fMessenger = messenger;
}


void
BContactWatcher::SetRef(const entry_ref* ref)
{
	delete fFile;
	delete fRef;
	fRef = ref;
	fFile = new BFile(ref, B_READ_WRITE);
}


void
BContactWatcher::WatchChanges(bool enable)
{
	if (fRef == NULL)
		return;

	node_ref nodeRef;

	BNode node(fRef);
	node.GetNodeRef(&nodeRef);

	uint32 flags;
	BString action;

	if (enable) {
		// Start watching.
		flags = B_WATCH_ALL;
		action = "starting";
	} else {
		// Stop watching.
		flags = B_STOP_WATCHING;
		action = "stoping";
	}

	if (watch_node(&nodeRef, flags, this) != B_OK) {
		printf("Error %s node monitor.\n", action.String());
	}
}


BFile*
BContactWatcher::GetFile()
{
	return fFile;
}


void
BContactWatcher::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) == B_OK) {
				msg->PrintToStream();
				switch (opcode) {
					case B_ENTRY_REMOVED:
						fMessenger->SendMessage(B_CONTACT_REMOVED);
						break;

					case B_ENTRY_MOVED:
						fMessenger->SendMessage(B_CONTACT_MOVED);
						break;

					case B_ATTR_CHANGED:
					case B_STAT_CHANGED:
						printf("12\n");
						fMessenger->SendMessage(B_CONTACT_MODIFIED);	
						break;
				}
			}
			break;
		}

		default:
			BLooper::MessageReceived(msg);
	}
}
