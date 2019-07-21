﻿
#include "pch.h"
#include <iostream>
#include "Common.h"

void test()
{
	{
		TextsDatabase db("TestDB");
		db._dbSerializer.SetPath("D:/Dimka/HomeProjects/");

		AttributeProperty ap;
		ap.id = 99;
		ap.visiblePosition = 56;
		ap.isVisible = true;
		ap.timestampCreated = 99887766;
		ap.name = "Dima";
		ap.type = 2;
		ap.param1 = 11223344;
		ap.param2 = 55667788;

		db._attributeProps.push_back(ap);

		ap.id = 100;
		ap.type = 1;
		ap.name = "Dima2";
		db._attributeProps.push_back(ap);

		Folder folder;
		folder.id = 55443322;
		folder.name = "ggss";
		folder.parentId = 567;
		folder.timestampCreated = 12345;
		folder.timestampModified = 54321;

		TextTranslated::Ptr textPtr = std::make_shared<TextTranslated>();

		textPtr->baseText = "Base Text";
		textPtr->id = "Text ID";
		textPtr->loginOfLastModifier = "Login123";
		textPtr->offsLastModified = 11111;
		textPtr->timestampCreated = 12;
		textPtr->timestampModified = 23;
		
		AttributeInText attr;
		attr.flagState = 1;
		attr.id = 99;
		attr.text = "Attrib text";
		attr.type = 2;
		textPtr->attributes.emplace_back(attr);

		attr.flagState = 0;
		attr.id = 100;
		attr.text = "Attrib text 2";
		attr.type = 1;
		textPtr->attributes.emplace_back(attr);

		folder.texts.emplace_back(textPtr);

		db._folders.emplace_back(folder);

		db._dbSerializer.SaveDatabase();
	}

	TextsDatabase db("TestDB");
	db._dbSerializer.SetPath("D:/Dimka/HomeProjects/");
	db._dbSerializer.LoadDatabaseAndHistory();
	db._dbSerializer.SetPath("D:/Dimka/");
	db._dbSerializer.SaveDatabase();
}


int main()
{
	test();
}

