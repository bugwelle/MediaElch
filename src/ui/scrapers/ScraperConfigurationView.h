#pragma once

#include "utils/Meta.h"

#include <QWidget>

namespace mediaelch {

class ScraperConfigurationView
{
public:
    ScraperConfigurationView() = default;
    virtual ~ScraperConfigurationView() = default;

    virtual void init() = 0;
    virtual void load() = 0;
    virtual void save() = 0;

    ELCH_NODISCARD virtual QWidget* widget() = 0;
};

} // namespace mediaelch
