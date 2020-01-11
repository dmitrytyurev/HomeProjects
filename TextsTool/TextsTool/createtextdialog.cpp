#include <QDebug>
#include "createtextdialog.h"
#include "ui_createtextdialog.h"

//---------------------------------------------------------------

CreateTextDialog::CreateTextDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CreateTextDialog)
{
	ui->setupUi(this);

	connect(this, SIGNAL(accepted()), this, SLOT(okPressed()));
	ui->lineEdit->setFocus();
}

//---------------------------------------------------------------

CreateTextDialog::~CreateTextDialog()
{
	delete ui;
}

//---------------------------------------------------------------

void CreateTextDialog::okPressed()
{
	qDebug() << ui->lineEdit->text();
}
