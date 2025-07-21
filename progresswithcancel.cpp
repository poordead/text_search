#include "progresswithcancel.h"
#include "ui_progressbar.h"

ProgressWithCancel::ProgressWithCancel(QWidget *parent)
	: QWidget(parent)
	, ui{std::make_unique<Ui::ProgressBar>()}
{
	ui->setupUi(this);
	connect(ui->cancelBtn, &QAbstractButton::clicked, this, &ProgressWithCancel::canceled);
}

ProgressWithCancel::~ProgressWithCancel() {}

void ProgressWithCancel::setValue(int value)
{
	ui->progressBar->setValue(value);
}

void ProgressWithCancel::setRange(int minimum, int maximum)
{
	ui->progressBar->setRange(minimum, maximum);
}
