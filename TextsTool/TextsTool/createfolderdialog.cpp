#include "createfolderdialog.h"
#include "ui_createfolderdialog.h"
#include "DatabaseManager.h"

CreateFolderDialog::CreateFolderDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CreateFolderDialog)
{
	ui->setupUi(this);

	connect(this, SIGNAL(accepted()), this, SLOT(okPressed()));
	ui->lineEdit->setFocus();
}

CreateFolderDialog::~CreateFolderDialog()
{
	delete ui;
}

//---------------------------------------------------------------

void CreateFolderDialog::okPressed()
{
	std::string FolderIdToCreate = ui->lineEdit->text().toLocal8Bit().constData();
	DatabaseManager::Instance().OnFolderCreatedFromGUI(FolderIdToCreate);
}
