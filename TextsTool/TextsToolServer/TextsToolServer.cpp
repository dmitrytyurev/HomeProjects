
#include "pch.h"
#include <iostream>
#include "Common.h"

void test()
{
	TextsDatabase db("TestDB");
	db._dbSerializer.SetPath("D:/Dimka/HomeProjects/");

	//AttributeProperty ap;
	//ap.id = 0x99;
	//ap.visiblePosition = 0x56;
	//ap.isVisible = true;
	//ap.timestampCreated = 0x99887766;
	//ap.name = "Dima";
	//ap.type = 0x12;
	//ap.param1 = 0x11223344;
	//ap.param2 = 0x55667788;

	//db._attributeProps.push_back(ap);
	//db._attributeProps.push_back(ap);


	//db._dbSerializer.SaveDatabase();

	db._dbSerializer.LoadDatabaseAndHistory();
}


int main()
{
	test();
}

