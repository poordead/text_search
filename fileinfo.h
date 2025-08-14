#pragma once

#include <QDateTime>
#include <QString>
#include <unzip.h>

struct FileInfo
{
	QString fileName;
	quint64 fileSize;
	QDateTime lastModified;
    unz64_file_pos filePos;
};
