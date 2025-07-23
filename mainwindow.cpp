#include "mainwindow.h"
#include "foundfilesmodel.h"
#include "minizip_wrapper.h"
#include "progresswithcancel.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentRun>

MainWindow::MainWindow(const QString &fileName, QWidget *parent)
	: QWidget(parent)
	, ui{std::make_unique<Ui::MainWindow>()}
	, m_fileName(fileName)
{
	ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);

    QTreeWidgetItem *analyzeItem = new QTreeWidgetItem(ui->summary_treeWidget, {tr("Анализ")});
	new QTreeWidgetItem(analyzeItem, {fileName});
	m_currentFileItem = new QTreeWidgetItem(analyzeItem);
	analyzeItem->setExpanded(true);

	auto progressItem = new QTreeWidgetItem(analyzeItem);
	ui->summary_treeWidget->setItemWidget(progressItem, 0, m_progressBar = new ProgressWithCancel);
	progressItem->setHidden(true);

	QTreeWidgetItem *saveItem = new QTreeWidgetItem(ui->summary_treeWidget, {tr("Сохранение")});
	auto saveBtnItem = new QTreeWidgetItem(saveItem);
	m_saveButton = new QPushButton{"Coхранить"};
	m_saveButton->setFixedWidth(m_saveButton->sizeHint().width());
	ui->summary_treeWidget->setItemWidget(saveBtnItem, 0, m_saveButton);
	m_currentSaveFileItem = new QTreeWidgetItem(saveItem);
	auto saveProgressItem = new QTreeWidgetItem(saveItem);
	ui->summary_treeWidget
		->setItemWidget(saveProgressItem, 0, m_saveProgressBar = new ProgressWithCancel);
	saveProgressItem->setHidden(true);

	ui->foundFiles->setModel(m_foundFiles = new FoundFilesModel{this});
	ui->foundFiles->setFileName(fileName);

	connect(&m_watcher,
			&QFutureWatcherBase::progressValueChanged,
			m_progressBar,
			&ProgressWithCancel::setValue);
	connect(&m_watcher,
			&QFutureWatcherBase::progressRangeChanged,
			m_progressBar,
			&ProgressWithCancel::setRange);
	connect(&m_watcher, &QFutureWatcherBase::progressTextChanged, this, [this](const QString &text) {
		m_currentFileItem->setText(0, text);
	});
	connect(&m_watcher, &QFutureWatcherBase::started, this, [progressItem]() {
		progressItem->setHidden(false);
	});
	connect(&m_watcher, &QFutureWatcherBase::finished, this, [progressItem]() {
		progressItem->setHidden(true);
	});
	connect(&m_watcher, &QFutureWatcherBase::canceled, this, [progressItem]() {
		progressItem->setHidden(true);
	});
	connect(m_progressBar, &ProgressWithCancel::canceled, &m_watcher, &QFutureWatcherBase::cancel);

	connect(&m_watcher, &QFutureWatcherBase::resultReadyAt, this, [this](int resultIndex) {
		if (m_foundFiles->rowCount() < resultIndex + 1) {
			m_foundFiles->insertRow(m_foundFiles->rowCount());
		}
		m_foundFiles->setFileInfo(resultIndex, m_watcher.resultAt(resultIndex));
        ui->foundCount->setValue(ui->foundCount->value() + 1);
	});

	connect(&m_saveWatcher,
			&QFutureWatcherBase::progressValueChanged,
			m_saveProgressBar,
			&ProgressWithCancel::setValue);
	connect(&m_saveWatcher,
			&QFutureWatcherBase::progressRangeChanged,
			m_saveProgressBar,
			&ProgressWithCancel::setRange);
	connect(&m_saveWatcher,
			&QFutureWatcherBase::progressTextChanged,
			this,
			[this](const QString &text) { m_currentSaveFileItem->setText(0, text); });
	connect(&m_saveWatcher, &QFutureWatcherBase::started, this, [saveProgressItem]() {
		saveProgressItem->setHidden(false);
	});
	connect(&m_saveWatcher, &QFutureWatcherBase::finished, this, [saveProgressItem]() {
		saveProgressItem->setHidden(true);
	});
	connect(&m_saveWatcher, &QFutureWatcherBase::canceled, this, [saveProgressItem]() {
		saveProgressItem->setHidden(true);
	});
	connect(m_saveProgressBar,
			&ProgressWithCancel::canceled,
			&m_saveWatcher,
			&QFutureWatcherBase::cancel);

	connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveZip);
	connect(ui->selectAll_btn, &QAbstractButton::clicked, this, &MainWindow::selectAll);

    connect(m_foundFiles,
            &FoundFilesModel::dataChanged,
            this,
            &MainWindow::filesFoundModelDataChanged);

    if (!fileName.isEmpty())
        m_watcher.setFuture(QtConcurrent::run(findTextInZip, fileName, "secret"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	m_watcher.cancel();
	m_saveWatcher.cancel();
	m_watcher.waitForFinished();
	m_saveWatcher.waitForFinished();
}

void MainWindow::saveZip()
{
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
	if (files.empty()) {
		QMessageBox::warning(this, windowTitle(), tr("Необходимо выбрать файлы на вкладке Файлы"));
		return;
	}

	const QString fileName{QFileDialog::getSaveFileName(this,
														tr("Сохранить выбранные файлы в архив"),
														QStandardPaths::writableLocation(
															QStandardPaths::DocumentsLocation),
														tr("Архив (*.zip)"))};
	if (fileName.isEmpty())
		return;

	m_saveWatcher.setFuture(QtConcurrent::run(zipSelectedFiles, m_fileName, fileName, files));
}

void MainWindow::selectAll(bool checked)
{
    ui->selectAll_btn->setText(checked ? tr("Снять выделение") : tr("Выделить все"));
    for (int i = 0, size = m_foundFiles->rowCount(); i < size; ++i) {
        m_foundFiles->setData(m_foundFiles->index(i, FoundFilesModel::C_Filename),
                              checked ? Qt::Checked : Qt::Unchecked,
                              Qt::CheckStateRole);
    }
}

void MainWindow::filesFoundModelDataChanged(const QModelIndex &topLeft,
                                            const QModelIndex &bottomRight,
                                            const QList<int> &roles)
{
    if (topLeft.column() == FoundFilesModel::C_Filename && roles.contains(Qt::CheckStateRole)) {
        for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
            if (m_foundFiles->index(row, FoundFilesModel::C_Filename)
                    .data(Qt::CheckStateRole)
                    .value<Qt::CheckState>()
                == Qt::Checked)
                ui->selectedCount->setValue(ui->selectedCount->value() + 1);
            else
                ui->selectedCount->setValue(ui->selectedCount->value() - 1);
        }
    }
}
