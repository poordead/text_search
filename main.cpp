#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QFileDialog>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QCommandLineParser parser;
	parser.addPositionalArgument("source", QCoreApplication::translate("main", "Исходный файл."));
	parser.process(a);

	QString fileName;
	if (const auto args{parser.positionalArguments()}; args.isEmpty()) {
		fileName = QFileDialog::getOpenFileName(nullptr,
												QFileDialog::tr("Исследовать файл"),
												QString(),
												QFileDialog::tr("Архив (*.zip)"));
	} else
		fileName = args.first();

	MainWindow w(fileName);
	w.show();
	return a.exec();
}
