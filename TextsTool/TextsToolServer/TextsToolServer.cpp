#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <set>
#include <experimental/filesystem>

#include "Common.h"
#include "DbSerializer.h"
#include "Utils.h"
#include "../SharedSrc/Shared.h"

const std::string databasesPath = "D:/Dimka/HomeProjects/TextsTool/DatabaseServer/";

//===============================================================================

void test()
{
	TextsDatabase db;
	db.CreateDatabase(databasesPath, "TestDB");

	// ---------------------------------------------------   
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! При изменении базы, удалять базу на сервере, поскольку без именения таймстэмпов база не обновится !!!!!!!
	// ---------------------------------------------------   

	AttributeProperty atProp;
	atProp.type = AttributePropertyDataType::Id_t;
	atProp.id = 0;
	atProp.isVisible = true;
	atProp.visiblePosition = 0;
	atProp.name = "TextId";
	db._attributeProps.emplace_back(atProp);

	atProp.type = AttributePropertyDataType::CreationTimestamp_t;
	atProp.id = 1;
	atProp.isVisible = true;
	atProp.visiblePosition = 1;
	atProp.name = "Created";
	db._attributeProps.emplace_back(atProp);

	atProp.type = AttributePropertyDataType::ModificationTimestamp_t;
	atProp.id = 2;
	atProp.isVisible = true;
	atProp.visiblePosition = 2;
	atProp.name = "Modified";
	db._attributeProps.emplace_back(atProp);

	atProp.type = AttributePropertyDataType::BaseText_t;
	atProp.id = 3;
	atProp.isVisible = true;
	atProp.visiblePosition = 3;
	atProp.name = "BaseText";
	db._attributeProps.emplace_back(atProp);

	atProp.type = AttributePropertyDataType::CommonText_t;
	atProp.id = 4;
	atProp.isVisible = true;
	atProp.visiblePosition = 4;
	atProp.name = "CommonText";
	db._attributeProps.emplace_back(atProp);

	// ---------------------------------------------------   
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! При изменении базы, удалять базу на сервере, поскольку без именения таймстэмпов база не обновится !!!!!!!
	// ---------------------------------------------------   

	Folder folder;
	folder.id = 123;
	folder.name = "FolderA";
	folder.parentId = UINT32_MAX;
	folder.timestampCreated = 12345;
	folder.timestampModified = 54322;


	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText1";
		textPtr->id = "TextID1";
		textPtr->timestampModified = 101;
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText2";
		textPtr->id = "TextID2";
		textPtr->timestampModified = 1021;
		AttributeInText attInText;
		attInText.id = 4;
		attInText.text = "Common text";
		attInText.type = AttributePropertyDataType::CommonText_t;
		textPtr->attributes.emplace_back(attInText);
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText3";
		textPtr->id = "TextID3";
		textPtr->timestampModified = 103;
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText4";
		textPtr->id = "TextID4";
		textPtr->timestampModified = 104;
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText5";
		textPtr->id = "TextID5";
		textPtr->timestampModified = 105;
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText6";
		textPtr->id = "TextID6_";
		textPtr->timestampModified = 106;
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText7";
		textPtr->id = "TextID7";
		textPtr->timestampModified = 107;
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText8";
		textPtr->id = "TextID8";
		textPtr->timestampModified = 108;
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText9";
		textPtr->id = "TextID9";
		textPtr->timestampModified = 109;
		folder.texts.emplace_back(textPtr);
	}

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText10";
		textPtr->id = "TextID10";
		textPtr->timestampModified = 110;
		folder.texts.emplace_back(textPtr);
	}

	db._folders.emplace_back(folder);

	// Второй каталог
	// ---------------------------------------------------   
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! При изменении базы, удалять базу на сервере, поскольку без именения таймстэмпов база не обновится !!!!!!!
	// ---------------------------------------------------   

	folder.id = 124;
	folder.parentId = 123;
	folder.name = "FolderB";
	folder.texts.clear();

	{
		TextTranslatedPtr textPtr = std::make_shared<TextTranslated>();
		textPtr->baseText = "BaseText11";
		textPtr->id = "TextID11";
		textPtr->timestampModified = 110;
		folder.texts.emplace_back(textPtr);
	}

	db._folders.emplace_back(folder);


	db._dbSerializer->SaveDatabase();
}

//===============================================================================

std::unique_ptr<STextsToolApp> app;


//===============================================================================

void MakeDatabasesNamesList(const std::string path, std::set<std::string>& basesNames)
{
	namespace fs = std::experimental::filesystem;

	std::string resultFileName;

	const std::string prefix = "TextsBase_";

	for (auto& p : fs::directory_iterator(path)) {
		std::string fullFileName = p.path().string();
		std::string fileName = fullFileName.c_str() + path.length();
		if (fileName.compare(0, prefix.length(), prefix)) {
			continue;
		}
		int baseNameLength = fileName.length() - 15 - prefix.length();
		std::string baseName = fileName.substr(prefix.length(), baseNameLength);
		basesNames.insert(baseName);
	}
}

//===============================================================================

void Init()
{
	Log("\n\n=== Start App ===========================================================");

	app = std::make_unique<STextsToolApp>();
	app->_httpMgr._connections._accounts.emplace_back("mylogin", "mypassword");

	//test();
	//exit(1);

	std::set<std::string> basesNames;
	MakeDatabasesNamesList(databasesPath, basesNames);  // Подготовить список баз по этому пути
	for (const auto& baseName : basesNames) {           // Загрузить базы по списку
		Log(std::string("Loading database: ") + baseName);
		app->_dbs.emplace_back(std::make_shared<TextsDatabase>());
		app->_dbs.back()->LoadDatabase(databasesPath, baseName);
	}
}

//===============================================================================

void MainLoop()
{
	using namespace std::chrono_literals;

	auto prevTime = std::chrono::high_resolution_clock::now();
	while (true)
	{
		auto curTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = curTime - prevTime;
		prevTime = curTime;
		app->Update(elapsed.count() / 1000.f);
		std::this_thread::sleep_for(50ms);
	}
}

//===============================================================================

int main()
{
	try {
		Init();
		MainLoop();
	}
	catch (std::exception& e) {
		std::string errorStr = std::string("Outter catch exception: ") + e.what();
		Log(errorStr);
		std::cerr << errorStr << std::endl;
	}
}

