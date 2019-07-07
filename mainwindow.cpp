#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDesktopWidget>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

#define QStringLiteral(str) QString::fromUtf8(str, sizeof(str) - 1)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QDesktopWidget *desktop = QApplication::desktop();
    move((desktop->width() - this->width())/2, (desktop->height() - this->height())/2);
    setWindowTitle(QStringLiteral("Login"));
    ui->lineEditPass->setEchoMode(QLineEdit::Password);

    //添加Sqlite数据库
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    //设置数据库
    db.setDatabaseName("../info.db");

    //打开数据库
    if( !db.open() ) //数据库打开失败
    {
        QMessageBox::warning(this, "错误", db.lastError().text());
        return;
    }
    //对数据库进行初始化
    QSqlQuery query;
    query.exec("DROP TABLE IF EXISTS `admin`;");
    query.exec("CREATE TABLE `admin` (`id` int(11) NOT NULL DEFAULT '0',`password` text,PRIMARY KEY (`id`));");
    query.exec("INSERT INTO `admin` VALUES ('10000', '123456');");
    query.exec("DROP TABLE IF EXISTS `account`;");

    db.close();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_LoginButton_clicked()
{
    QString username = ui->lineEditNum->text();
    QString password = ui->lineEditPass->text();
    if(username==""||password=="") {
        QMessageBox::information(this, QStringLiteral("警告！"),QStringLiteral("请按照格式正确填写用户名，密码！！"));
        QString username = ui->lineEditNum->text();
        QString password = ui->lineEditPass->text();
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("../info.db");

    //打开数据库
    if( !db.open() ) {
        QMessageBox::warning(this, "错误", db.lastError().text());
        return;
    }

    if(!db.open()) {
        QMessageBox::warning(this, "error", db.lastError().text());
    }
    bool in=false;
    QSqlQuery select;
    QString sql = QString("select * from admin where id = '%1' and password='%2';").arg(username,password);
    in = select.exec(sql);

    if(select.next()) {
        this->hide();
        monitor = new Monitor;
        this->monitor->show();
    } else {
        QMessageBox::information(this, QStringLiteral("警告！"),QStringLiteral("用户名或密码有误！！！"));
    }
}

void MainWindow::on_ExitButton_clicked() {
    this->close();
}
