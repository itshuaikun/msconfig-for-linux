#include "bootloader.h"
#include "grub.h"
#include <QFile>
#include <QtEndian>

namespace Bootloader {
QString decodeEfiString(const QByteArray& bytes)
{
    // efivarfs prepends four attribute bytes, then UTF-16LE including NUL.
    if (bytes.size() < 6 || (bytes.size() - 4) % 2)
        return {};
    QString value;
    for (int i = 4; i + 1 < bytes.size(); i += 2) {
        const auto ch = qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar*>(bytes.constData() + i));
        if (!ch)
            break;
        value += QChar(ch);
    }
    return value;
}

static QString readVariable(const QString& name)
{
    QFile f("/sys/firmware/efi/efivars/" + name
            + "-4a67b082-0a4c-41cf-b6c7-440b29bb8c4f");
    return f.open(QIODevice::ReadOnly) ? decodeEfiString(f.readAll()) : QString();
}

Info detect()
{
    Info info;
    info.name = readVariable("LoaderInfo");
    info.selectedEntry = readVariable("LoaderEntrySelected");
    info.isLimine = info.name.startsWith("Limine", Qt::CaseInsensitive);
    // Installed binaries do not establish which loader was used.
    if (info.name.isEmpty())
        info.name = Grub::cfgPath().isEmpty() ? "Unknown" : "GRUB (configuration found)";
    return info;
}

QString kernelCommandLine()
{
    QFile f("/proc/cmdline");
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString();
}
}
