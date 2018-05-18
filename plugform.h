#ifndef PLUGFORM_H
#define PLUGFORM_H

#include <QWidget>

namespace Ui {
class PlugForm;
}

class PlugForm : public QWidget
{
    Q_OBJECT

public:
    explicit PlugForm(QWidget *parent = 0);
    ~PlugForm();
    void initComboBox(const QStringList& list);

signals:
    void button_clicked(int index);
private slots:
    void on_pushButton_clicked();

private:
    Ui::PlugForm *ui;
};

#endif // PLUGFORM_H
