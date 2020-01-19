#include <QDebug>
#include "deletefolderdialog.h"
#include "ui_deletefolderdialog.h"
#include "DatabaseManager.h"

//---------------------------------------------------------------

DeleteFolderDialog::DeleteFolderDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DeleteFolderDialog)
{
	ui->setupUi(this);

	connect(this, SIGNAL(accepted()), this, SLOT(okPressed()));
	ui->lineEdit->setFocus();
}

//---------------------------------------------------------------

DeleteFolderDialog::~DeleteFolderDialog()
{
	delete ui;
}

//---------------------------------------------------------------

void DeleteFolderDialog::okPressed()
{
//	std::string FolderIdToDelete = ui->lineEdit->Folder().toLocal8Bit().constData();
//	DatabaseManager::Instance().OnFolderDeletedFromGUI(textIdToDelete);

}
