﻿
#include "pch.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <set>
#include <experimental/filesystem>

#include "Common.h"
#include "DbSerializer.h"
#include "Utils.h"

const std::string databasesPath = "D:/Dimka/HomeProjects/";

//===============================================================================

void test()
{
	TextsDatabase db;
	db.CreateDatabase("D:/Dimka/HomeProjects/", "TestDB");

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

std::vector<SerializationBufferPtr>   _msgsQueueOut; 
const int packetMaxSize = 5;

void link_and_split()
{
	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	_msgsQueueOut.back()->buffer.resize(15);

	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	_msgsQueueOut.back()->buffer.resize(1);

	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	_msgsQueueOut.back()->buffer.resize(4);

	_msgsQueueOut.emplace_back(std::make_shared<SerializationBuffer>());
	_msgsQueueOut.back()->buffer.resize(7);

	auto& v = _msgsQueueOut;

	while (v.size()) {
		if (v[0]->buffer.size() > packetMaxSize) {
			// Сообщение превышает максимальный размер пакета. Разбиваем его на несколько пакетов
			int n = (v[0]->buffer.size() + packetMaxSize - 1) / packetMaxSize; // Число пакетов, на которое разобьём сообщение
			for (int i = 0; i < n; ++i) {
				int curSize = (i == n - 1 ? v[0]->buffer.size() - (n-1) * packetMaxSize : packetMaxSize);
				printf("%d\n", curSize);
				printf("----\n");
			}
			v.erase(v.begin());
		}
		else {
			// Объединяем по несколько сообщений в пакет (от 1 до N)
			int n = 0;  
			int sum = 0;
			// Считаем число сообщений n, который поместятся в один пакет
			while (sum + v[n]->buffer.size() <= packetMaxSize) {
				sum += v[n]->buffer.size();
				if (++n == v.size()) {
					break;
				}
			};
			// Заносим n очередных сообщений в один пакет
			for (int i = 0; i < n; ++i) {
				printf("%d\n", v[0]->buffer.size());
				v.erase(v.begin());
			}
			printf("----\n");
		}
	}
}

 

//===============================================================================

int main()
{
	link_and_split();
	return 0;

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

