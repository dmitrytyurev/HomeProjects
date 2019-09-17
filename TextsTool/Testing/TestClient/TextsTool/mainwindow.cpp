#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>

char arr[1000000];


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished,
        this, [=](QNetworkReply *reply) {
            if (reply->error()) {
                qDebug() << reply->errorString();
                return;
            }

            qint64 size = reply->read(arr, sizeof(arr));
            std::string sstr = std::to_string(size);
            ui->textEdit->setText(QString::fromStdString(sstr));
            int sum = 0;
            for (int i = 0; i < size; ++i) {
                sum += (unsigned char)arr[i];
            }
            printf("sum = %d\n", sum);



//            std::string sstr;
//            for (int i=0; i < 4; ++i) {
//                unsigned int  n = arr[i];
//                sstr += std::to_string(n);
//                sstr += " ";
//            }
//ui->textEdit->setText(QString::fromStdString(sstr));


//            ui->textEdit->setText(answer);
            reply->deleteLater();
        }
    );
}

MainWindow::~MainWindow()
{
    delete ui;
    delete manager;
}

void MainWindow::on_pushButton_clicked()
{
    char data[11]="hgslhgsljk";
    data[3] = 0;
    data[4] = 1;
    data[5] = 2;
    QByteArray postData(data, 11);

    request.setUrl(QUrl("http://127.0.0.1:8000/"));

    request.setRawHeader("User-Agent", "My app name v0.1");
    request.setRawHeader("X-Custom-User-Agent", "My app name v0.1");
    request.setRawHeader("Content-Type", "application/json");
    QByteArray postDataSize = QByteArray::number(11);
    request.setRawHeader("Content-Length", postDataSize);

    //request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
 //   manager->get(request);

    manager->post(request, postData);
}
