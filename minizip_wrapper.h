#pragma once

#include <QPromise>
#include "fileinfo.h"

void findTextInZip(QPromise<FileInfo> &promise,
				   const QString &zipPath,
				   const std::string &search_str);

void zipSelectedFiles(QPromise<void> &promise,
                      const QString &zipPath,
                      const QString &newZipPath,
                      const QList<FileInfo> &files);
