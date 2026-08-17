#pragma once
#include <QString>

// 初始化语言（根据系统 locale 选择中文/英文）
void initLang();
// 按 key 取界面字符串；找不到时返回 key 本身
QString L(const char* key);
