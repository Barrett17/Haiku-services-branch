#include "VCardTranslator.h"

#include <shared/AutoDeleter.h>
#include <ObjectList.h>

#include <new>
#include <stdio.h>
#include <syslog.h>

#include "VCardParserDefs.h"
#include "VCardParser.h"


const char* kTranslatorName = "VCardTranslator";
const char* kTranslatorInfo = "Translator for VCard files";

#define VCARD_MIME_TYPE "text/x-vCard"
#define CONTACT_MIME_TYPE "application/x-hcontact"

static const translation_format sInputFormats[] = {
		{
		B_CONTACT_FORMAT,
		B_TRANSLATOR_CONTACT,
		IN_QUALITY,
		IN_CAPABILITY,
		CONTACT_MIME_TYPE,
		"Haiku binary contact"
	},
	{
		B_VCARD_FORMAT ,
		B_TRANSLATOR_CONTACT,
		IN_QUALITY,
		IN_CAPABILITY,
		VCARD_MIME_TYPE,
		"vCard Contact file"
	}
};

// The output formats that this translator supports.
static const translation_format sOutputFormats[] = {
		{
		B_CONTACT_FORMAT,
		B_TRANSLATOR_CONTACT,
		OUT_QUALITY,
		OUT_CAPABILITY,
		CONTACT_MIME_TYPE,
		"Haiku binary contact"
	},
	{
		B_VCARD_FORMAT ,
		B_TRANSLATOR_CONTACT,
		OUT_QUALITY,
		OUT_CAPABILITY,
		VCARD_MIME_TYPE,
		"vCard Contact file"
	}
};

// Default settings for the Translator
static const TranSetting sDefaultSettings[] = {
};

const uint32 kNumInputFormats = sizeof(sInputFormats) 
	/ sizeof(translation_format);
const uint32 kNumOutputFormats = sizeof(sOutputFormats)
	/ sizeof(translation_format);
const uint32 kNumDefaultSettings = sizeof(sDefaultSettings)
	/ sizeof(TranSetting);


struct VCardVisitor : public BContactFieldVisitor {
public:
					VCardVisitor(BPositionIO* destination)
					:
					fDest(destination)
					{
						
					}
	virtual		 	~VCardVisitor()
					{
					}

	void			WriteBegin()
	{
		// TODO VCard 3/4 support
		fDest->Seek(0, SEEK_SET);
		const char data[] = "BEGIN:VCARD\nVERSION:2.1\n";
		fDest->Write(data, sizeof(data)-1);
	}

	void			WriteEnd()
	{
		const char data[] = "END:VCARD\n";
		fDest->Write(data, sizeof(data)-1);	
	}

	virtual void 	Visit(BStringContactField* field)
	{
		// TODO actually it have problems if the destination file
		// is more big than the data we have to write. Add code
		// to empty the file before writing.
		BString str;
		switch (field->FieldType()) {
			case B_CONTACT_BIRTHDAY:
				str << VCARD_BIRTHDAY;
			break;
			case B_CONTACT_EMAIL:
				str << VCARD_EMAIL;
				switch (field->Usage()) {
					case CONTACT_DATA_HOME:
					str << ";HOME";
					case CONTACT_DATA_WORK:
					str << ";WORK";					
					case CONTACT_EMAIL_MOBILE:
					str << ";MOBILE";
				}
			break;
			case B_CONTACT_FORMATTED_NAME:
				str << VCARD_FORMATTED_NAME;
			break;
			case B_CONTACT_GEO:
				str << VCARD_GEOGRAPHIC_POSITION;
			break;
			case B_CONTACT_IM:
				str << X_VCARD_IM;
			break;
			case B_CONTACT_NAME:
				str << VCARD_NAME;
			break;
			case B_CONTACT_NICKNAME:
				str << VCARD_NICKNAME;
			break;
			case B_CONTACT_NOTE:
				str << VCARD_NOTE;
			break;
			case B_CONTACT_ORGANIZATION:
				str << VCARD_ORGANIZATION;
			break;
 			case B_CONTACT_PHONE:
				str << VCARD_TELEPHONE;
				switch (field->Usage()) {
					case CONTACT_DATA_HOME:
						str << ";HOME";
					break;
					case CONTACT_DATA_WORK:
						str << ";WORK";
					break;
					case CONTACT_PHONE_MOBILE:
						str << ";MOBILE";
					break;
					case CONTACT_PHONE_FAX_WORK:
						str << ";WORK;FAX";
					break;
					case CONTACT_PHONE_FAX_HOME:
						str << ";WORK;FAX";
					break;
					/*case CONTACT_DATA_OTHER:
					
					break;*/
					case CONTACT_PHONE_PAGER:
					case CONTACT_PHONE_CALLBACK:
					case CONTACT_PHONE_CAR:
					case CONTACT_PHONE_ORG_MAIN:
					case CONTACT_PHONE_ISDN:
					case CONTACT_PHONE_MAIN:
					case CONTACT_PHONE_RADIO:
					case CONTACT_PHONE_TELEX:
					case CONTACT_PHONE_TTY_TDD:
					case CONTACT_PHONE_WORK_MOBILE:
					case CONTACT_PHONE_WORK_PAGER:
					case CONTACT_PHONE_ASSISTANT:
					case CONTACT_PHONE_MMS:
					break;
				}
			break;
			case B_CONTACT_PROTOCOLS:
				str << X_VCARD_PROTOCOLS;
			break;
			case B_CONTACT_SIMPLE_GROUP:
				str << X_VCARD_SIMPLE_GROUP;
			break;
			case B_CONTACT_SOUND:
				str << VCARD_SOUND;
			break;
			case B_CONTACT_TIME_ZONE:
				str << VCARD_TIME_ZONE;
			break;
			case B_CONTACT_TITLE:
				str << VCARD_TITLE;
			break;
			case B_CONTACT_URL:
				str << VCARD_URL;
			break;

			/* hidden fields */
			case B_CONTACT_GROUP:
				str << X_VCARD_GROUP;
			break;
			case B_CONTACT_UID:
				str << X_VCARD_UID;
			break;
			case B_CONTACT_REV:
				str << VCARD_REVISION;
			break;
		}

		_Write(str, field->Value());
	}

	virtual void 	Visit(BAddressContactField* field)
	{
		field_type type = field->FieldType();
		BString str;

		if (type == B_CONTACT_ADDRESS)
			str = VCARD_ADDRESS;
		else if (type == B_CONTACT_DELIVERY_LABEL)
			str = VCARD_DELIVERY_LABEL;

		_Write(str, field->Value());
	}

	virtual void 	Visit(BPhotoContactField* field)
	{
		// TODO add a specific extension or find another 
		// in-standard way to support this field.
	}

private:
			void	_Write(BString& str, const BString& value)
	{
		if (str.Length() > 0) {
			str << ":" << value << "\n";
			fDest->Write(str.String(), str.Length());
		}
	}

	BPositionIO* fDest;
};


// required by the BaseTranslator class
BTranslator *
make_nth_translator(int32 n, image_id you, uint32 flags, ...)
{
	if (!n)
		return new (std::nothrow) VCardTranslator();

	return NULL;
}


VCardTranslator::VCardTranslator()
	:
	BaseTranslator(kTranslatorName, kTranslatorInfo, VCARD_TRANSLATOR_VERSION,
		sInputFormats, kNumInputFormats, sOutputFormats, kNumOutputFormats,
		"VCardTranslatorSettings", sDefaultSettings, kNumDefaultSettings,
		B_TRANSLATOR_CONTACT, B_VCARD_FORMAT)
{
}


VCardTranslator::~VCardTranslator()
{
}


status_t
VCardTranslator::Identify(BPositionIO* inSource,
	const translation_format* inFormat, BMessage* ioExtension,
	translator_info* outInfo, uint32 outType)
{
	if (!outType)
		outType = B_CONTACT_FORMAT;

	if (outType != B_CONTACT_FORMAT && outType != B_VCARD_FORMAT)
		return B_NO_TRANSLATOR;

	BMessage msg;
 	if (outType == B_VCARD_FORMAT && msg.Unflatten(inSource) == B_OK) {
 		msg.PrintToStream();
		outInfo->type = B_CONTACT_FORMAT;
		outInfo->group = B_TRANSLATOR_CONTACT;
		outInfo->quality = IN_QUALITY;
		outInfo->capability = IN_CAPABILITY;
		snprintf(outInfo->name, sizeof(outInfo->name), kTranslatorName);
		strcpy(outInfo->MIME, CONTACT_MIME_TYPE);
		return B_OK;
 	} else if (outType == B_CONTACT_FORMAT)
 		return _IdentifyVCard(inSource, outInfo);

	return B_NO_TRANSLATOR;
}


status_t
VCardTranslator::Translate(BPositionIO* inSource, const translator_info* info,
	BMessage* ioExtension, uint32 outType, BPositionIO* outDestination)
{
	if (!outType)
		outType = B_CONTACT_FORMAT;

	if (outType != B_CONTACT_FORMAT && outType != B_VCARD_FORMAT)
		return B_NO_TRANSLATOR;

	// add no translation
	BMessage msg;
 	if (outType == B_VCARD_FORMAT && msg.Unflatten(inSource) == B_OK)
		return TranslateContact(&msg, ioExtension, outDestination);
 	else if (outType == B_CONTACT_FORMAT)
		return TranslateVCard(inSource, ioExtension, outDestination);

	return B_ERROR;
}


status_t
VCardTranslator::TranslateContact(BMessage* inSource, 
		BMessage* ioExtension, BPositionIO* outDestination)
{
	int32 count;
	type_code code = B_CONTACT_FIELD_TYPE;

	VCardVisitor visitor(outDestination);

	visitor.WriteBegin();

	status_t ret = inSource->GetInfo(CONTACT_FIELD_IDENT, &code, &count);
	if (ret != B_OK)
		return ret;

	for (int i = 0; i < count; i++) {
		const void* data;
		ssize_t size;

		ret = inSource->FindData(CONTACT_FIELD_IDENT, code,
			i, &data, &size);

		BContactField* field = BContactField::UnflattenChildClass(data, size);
		if (field == NULL)
			return B_ERROR;

		field->Accept(&visitor);

		delete field;
	}
	visitor.WriteEnd();
	return B_OK;
}


status_t
VCardTranslator::TranslateVCard(BPositionIO* inSource, 
		BMessage* ioExtension, BPositionIO* outDestination)
{
	VCardParser parser(inSource);

	if (parser.Parse() != B_OK)
		return B_ERROR;

	BObjectList<BContactField>* list = parser.Properties();

	BMessage msg;
	int count = list->CountItems();
	for (int32 i = 0; i < count; i++) {
		BContactField* object = list->ItemAt(i);
		ssize_t size = object->FlattenedSize();
		void* buffer = new char[size];
		if (buffer == NULL)
			return B_NO_MEMORY;
		MemoryDeleter deleter(buffer);

		status_t ret = object->Flatten(buffer, size);
		if (ret != B_OK)
			return ret;

		ret = msg.AddData(CONTACT_FIELD_IDENT,
			B_CONTACT_FIELD_TYPE, buffer, size, false);
	}

	outDestination->Seek(0, SEEK_SET);
	return msg.Flatten(outDestination);
}


status_t
VCardTranslator::_IdentifyVCard(BPositionIO* inSource,
	translator_info* outInfo)
{
	// TODO change it to specifically check
	// for BEGIN:VCARD and BEGIN:VCARD fields.
	// I.E. implement a custom ParseAndIdentify() method
	// that just don't use the parser and gain performances.

	// Using this initialization
	// the parse will only verify
	// that it's a VCard file
	// without saving any property
	// but still will waste resource (see the TODO)
	VCardParser parser(inSource, true);
	
	if (parser.Parse() != B_OK)
		return B_ERROR;

	if (parser.HasBegin() && parser.HasEnd()) {
		outInfo->type = B_VCARD_FORMAT;
		outInfo->group = B_TRANSLATOR_CONTACT;
		outInfo->quality = IN_QUALITY;
		outInfo->capability = IN_CAPABILITY;
		snprintf(outInfo->name, sizeof(outInfo->name), kTranslatorName);
		strcpy(outInfo->MIME, VCARD_MIME_TYPE);
		return B_OK;
	}
	return B_NO_TRANSLATOR;
}


BView*
VCardTranslator::NewConfigView(TranslatorSettings *settings)
{
	return 	new VCardView(BRect(0, 0, 225, 175), 
		"VCardTranslator Settings", B_FOLLOW_ALL,
		B_WILL_DRAW, settings);
}
