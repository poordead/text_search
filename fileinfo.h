#pragma once

#include <QDateTime>
#include <QString>

struct FileInfo
{
	QString fileName;
	quint64 fileSize;
	QDateTime lastModified;
};
