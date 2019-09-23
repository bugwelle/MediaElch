#pragma once

#include <QObject>

class MacTouchbar : public QObject
{
    Q_OBJECT

public:
    void init();
    static MacTouchbar* instance();
};
