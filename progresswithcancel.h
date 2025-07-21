#pragma once

#include <QWidget>

namespace Ui {

class ProgressBar;

}

class ProgressWithCancel : public QWidget
{
	Q_OBJECT

public:
	explicit ProgressWithCancel(QWidget *parent = nullptr);
	~ProgressWithCancel();
	void setValue(int value);
	void setRange(int minimum, int maximum);

signals:
	void canceled();

private:
	std::unique_ptr<Ui::ProgressBar> ui;
};
