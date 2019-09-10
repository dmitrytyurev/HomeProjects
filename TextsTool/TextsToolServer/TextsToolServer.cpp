
#include "pch.h"
#include <iostream>
#include <chrono>
#include <thread>

#include "Common.h"
#include "DbSerializer.h"

//===============================================================================
//
//===============================================================================

void test()
{
	{
		TextsDatabase db;
		db.CreateFromBase("D:/Dimka/HomeProjects/", "TestDB");

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

		db._dbSerializer->SaveDatabase();
	}

	TextsDatabase db;
	db.CreateFromBase("D:/Dimka/HomeProjects/", "TestDB");
	db._dbSerializer->SetPath("D:/Dimka/");
	db._dbSerializer->SaveDatabase();
}

//===============================================================================
//
//===============================================================================

void test2()
{
	//using namespace std::chrono_literals;

	//STextsToolApp app;

	//app._dbs.emplace_back(std::make_shared<TextsDatabase>("D:/Dimka/HomeProjects/", "TestDB"));

	//std::cout << "TS:" << app._dbs.back()->_folders[0].timestampCreated;

	//app._dbs.back()->_dbSerializer->PushCreateFolder(app._dbs.back()->_folders[0], "tiurev-d");
	//app._dbs.back()->_dbSerializer->PushCreateFolder(app._dbs.back()->_folders[0], "tiurev-d");

	//auto prevTime = std::chrono::high_resolution_clock::now();
	//while (true)
	//{
	//	auto curTime = std::chrono::high_resolution_clock::now();
	//	std::chrono::duration<double, std::milli> elapsed = curTime - prevTime;
	//	prevTime = curTime;
	//	app.Update(elapsed.count() / 1000.f);
	//	std::this_thread::sleep_for(50ms);
	//}
}

//===============================================================================
//
//===============================================================================

void test3()
{
	//STextsToolApp app;
	//SClientMessagesMgr messagesMgr(&app);
	//messagesMgr.test();
}

//===============================================================================
//
//===============================================================================

int main()
{
//	test();
//	test2();
//	test3();
}

