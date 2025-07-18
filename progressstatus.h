#pragma once

#include <QWidget>

namespace Ui {
class ProgressStatus;
}

class ProgressStatus : public QWidget
{
	Q_OBJECT

public:
	explicit ProgressStatus(QWidget *parent = nullptr);
	~ProgressStatus();

private:
	Ui::ProgressStatus *ui;
};
