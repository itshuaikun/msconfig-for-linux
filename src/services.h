#pragma once
#include <QDateTime>
#include <QString>
#include <QVector>

struct ServiceInfo {
    QString unit;       // xxx.service
    QString state;      // enabled / enabled-runtime / disabled / static / masked ...
    QString desc;       // Description=
    QString path;       // unit 文件路径
    QDateTime modified; // 文件修改时间（对应 msconfig 的“日期”列）
    bool toggleable = false; // enabled/disabled 才能切换
    bool userInstalled = false; // 在 /etc、/run 或 /usr/local 下（非发行版自带）
};

namespace Services {

QVector<ServiceInfo> list(QString* err = nullptr);

// 批量操作保护名单：这些服务一旦禁用可能无法进入图形界面/无法再授权恢复
bool isProtected(const QString& unit);

} // namespace Services
