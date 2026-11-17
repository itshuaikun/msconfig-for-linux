#pragma once
#include <QString>

namespace Bootloader {
struct Info {
    QString name;
    QString selectedEntry;
    bool isLimine = false;
};
// LoaderInfo comes from the loader which actually booted this session.
QString decodeEfiString(const QByteArray& bytes);
Info detect();
QString kernelCommandLine();
}
