#include "progressstatus.h"
#include "ui_progressstatus.h"

ProgressStatus::ProgressStatus(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::ProgressStatus)
{
	ui->setupUi(this);
}

ProgressStatus::~ProgressStatus()
{
	delete ui;
}
