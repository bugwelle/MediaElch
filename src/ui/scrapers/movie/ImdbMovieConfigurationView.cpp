#include "ui/scrapers/movie/ImdbMovieConfigurationView.h"
#include <QGridLayout>
#include <QLabel>

namespace mediaelch {
namespace scraper {


ImdbMovieConfigurationView::ImdbMovieConfigurationView(ImdbMovieConfiguration& settings) :
    m_settings(settings), //
    m_settingsWidget{new QWidget}
{
    m_loadAllTagsWidget = new QCheckBox(tr("Load all tags"), m_settingsWidget);
    auto* layout = new QGridLayout(m_settingsWidget);
    layout->addWidget(m_loadAllTagsWidget, 0, 0);
    layout->setContentsMargins(12, 0, 12, 12);
    m_settingsWidget->setLayout(layout);
}

void ImdbMovieConfigurationView::init()
{
}

QWidget* ImdbMovieConfigurationView::widget()
{
    return m_settingsWidget;
}

void ImdbMovieConfigurationView::load()
{
    m_loadAllTagsWidget->setChecked(m_settings.shouldLoadAllTags());
}

void ImdbMovieConfigurationView::save()
{
    m_settings.setLoadAllTags(m_loadAllTagsWidget->isChecked());
}


} // namespace scraper
} // namespace mediaelch
