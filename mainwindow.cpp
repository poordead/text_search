#include "mainwindow.h"
#include "foundfilesmodel.h"
#include "minizip_wrapper.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFutureWatcher>
#include <QProgressBar>
#include <QPushButton>
#include <QtConcurrent/QtConcurrentRun>

MainWindow::MainWindow(const QString &fileName, QWidget *parent)
	: QWidget(parent)
	, ui{std::make_unique<Ui::MainWindow>()}
	, m_fileName(fileName)
{
	ui->setupUi(this);

	QTreeWidgetItem *analyzeItem = new QTreeWidgetItem(ui->summary_treeWidget, {tr("Анализ")});
	new QTreeWidgetItem(analyzeItem, {fileName});
	m_currentFileItem = new QTreeWidgetItem(analyzeItem);
	analyzeItem->setExpanded(true);

	auto progressItem = new QTreeWidgetItem(analyzeItem);
	ui->summary_treeWidget->setItemWidget(progressItem, 0, m_progressBar = new QProgressBar);

	QTreeWidgetItem *saveItem = new QTreeWidgetItem(ui->summary_treeWidget, {tr("Сохранение")});
	auto saveBtnItem = new QTreeWidgetItem(saveItem);
	ui->summary_treeWidget
		->setItemWidget(saveBtnItem,
						0,
						m_saveButton = new QPushButton{"Coхранить", ui->summary_treeWidget});
	auto saveProgressItem = new QTreeWidgetItem(saveItem);
	ui->summary_treeWidget
		->setItemWidget(saveProgressItem, 0, m_saveProgressBar = new QProgressBar);

	ui->foundFiles->setModel(m_foundFiles = new FoundFilesModel{this});
	ui->foundFiles->setFileName(fileName);

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

	connect(&m_watcher, &QFutureWatcherBase::resultReadyAt, this, [this](int resultIndex) {
		if (m_foundFiles->rowCount() < resultIndex + 1) {
			m_foundFiles->insertRow(m_foundFiles->rowCount());
		}
		m_foundFiles->setFileInfo(resultIndex, m_watcher.resultAt(resultIndex));
	});

	connect(&m_saveWatcher,
			&QFutureWatcherBase::progressValueChanged,
			m_saveProgressBar,
			&QProgressBar::setValue);
	connect(&m_saveWatcher,
			&QFutureWatcherBase::progressRangeChanged,
			m_saveProgressBar,
			&QProgressBar::setRange);

	connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveZip);

	m_watcher.setFuture(QtConcurrent::run(findTextInZip, fileName, "secret"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	m_watcher.cancel();
}

void MainWindow::saveZip()
{
	QString fileName{QFileDialog::getSaveFileName(this,
												  tr("Сохранить выбранные файлы в архив"),
												  QString(),
												  tr("Архив (*.zip)"))};
	if (fileName.isEmpty())
		return;

	std::vector<std::string> files;

	for (int i = 0, size = m_foundFiles->rowCount(); i < size; ++i) {
		if (m_foundFiles->index(i, FoundFilesModel::C_Filename)
				.data(Qt::CheckStateRole)
				.value<Qt::CheckState>()
			== Qt::Checked) {
			auto fileName = m_foundFiles->index(i, FoundFilesModel::C_Filename).data().toString();
			files.emplace_back(fileName.toUtf8());
		}
	}
	m_saveWatcher.setFuture(QtConcurrent::run(zipSelectedFiles, m_fileName, fileName, files));
}
