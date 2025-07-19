#include "mainwindow.h"
#include "foundfilesmodel.h"
#include "minizip_wrapper.h"
#include "ui_mainwindow.h"

#include <QFutureWatcher>
#include <QProgressBar>
#include <QtConcurrent/QtConcurrentRun>


MainWindow::MainWindow(const QString &fileName, QWidget *parent)
	: QWidget(parent)
	, ui{std::make_unique<Ui::MainWindow>()}
{
	ui->setupUi(this);

	QTreeWidgetItem *analyzeItem = new QTreeWidgetItem(ui->summary_treeWidget, {tr("Анализ")});
	new QTreeWidgetItem(analyzeItem, {fileName});
	m_currentFileItem = new QTreeWidgetItem(analyzeItem);
	analyzeItem->setExpanded(true);

	auto progressItem = new QTreeWidgetItem(analyzeItem);
	ui->summary_treeWidget->setItemWidget(progressItem, 0, m_progressBar = new QProgressBar);

	QTreeWidgetItem *saveItem = new QTreeWidgetItem(ui->summary_treeWidget, {tr("Сохранение")});
	new QTreeWidgetItem(saveItem);

	ui->foundFiles->setModel(m_foundFiles = new FoundFilesModel{this});

	connect(&m_watcher,
			&QFutureWatcherBase::progressValueChanged,
			m_progressBar,
			&QProgressBar::setValue);
	connect(&m_watcher,
			&QFutureWatcherBase::progressRangeChanged,
			m_progressBar,
			&QProgressBar::setRange);
	connect(&m_watcher, &QFutureWatcherBase::progressTextChanged, this, [this](const QString &text) {
		m_currentFileItem->setText(0, text);
	});
	connect(&m_watcher,
			&QFutureWatcherBase::progressValueChanged,
			m_progressBar,
			&QProgressBar::setValue);

	connect(&m_watcher, &QFutureWatcherBase::resultReadyAt, this, [this](int resultIndex) {
		if (m_foundFiles->rowCount() < resultIndex + 1) {
			m_foundFiles->insertRow(m_foundFiles->rowCount());
		}
		m_foundFiles->setFileInfo(resultIndex, m_watcher.resultAt(resultIndex));
	});

	m_watcher.setFuture(QtConcurrent::run(findTextInZip, fileName, "secret"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	m_watcher.cancel();
}
