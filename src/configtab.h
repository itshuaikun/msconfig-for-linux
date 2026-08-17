#pragma once
#include <QApplication>
#include <QStringList>
#include <QWidget>

// 所有标签页的公共接口：apply() 返回错误信息列表（空 = 全部成功）
class ConfigTab : public QWidget {
    Q_OBJECT
public:
    explicit ConfigTab(QWidget* parent = nullptr)
        : QWidget(parent) {}
    ~ConfigTab() override = default;

    virtual QStringList apply() = 0; // 应用本页更改
    virtual void reload() = 0;       // 重新载入数据
};

// 提权命令执行期间的等待光标 RAII
struct BusyCursor {
    BusyCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); }
    ~BusyCursor() { QApplication::restoreOverrideCursor(); }
};
