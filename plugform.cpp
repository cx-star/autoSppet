#include "plugform.h"
#include "ui_plugform.h"

PlugForm::PlugForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlugForm)
{
    ui->setupUi(this);
}

PlugForm::~PlugForm()
{
    delete ui;
}

void PlugForm::initComboBox(const QStringList &list)
{
    ui->comboBox->clear();
    ui->comboBox->insertItems(ui->comboBox->count(),list);
}

void PlugForm::on_pushButton_clicked()
{
    emit button_clicked(ui->comboBox->currentIndex());
}
