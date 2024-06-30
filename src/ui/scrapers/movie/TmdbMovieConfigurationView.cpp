#include "ui/scrapers/movie/TmdbMovieConfigurationView.h"
#include <QGridLayout>
#include <QLabel>

namespace mediaelch {
namespace scraper {


TmdbMovieConfigurationView::TmdbMovieConfigurationView(TmdbMovieConfiguration& settings) :
    m_settings(settings), //
    m_widget{new QWidget},
    m_box{new QComboBox(m_widget)}
{
}

void TmdbMovieConfigurationView::init()
{
    for (const mediaelch::Locale& lang : m_settings.supportedLanguages()) {
        m_box->addItem(lang.languageTranslated(), lang.toString());
    }

    auto* layout = new QGridLayout(m_widget);
    layout->addWidget(new QLabel(tr("Language")), 0, 0);
    layout->addWidget(m_box, 0, 1);
    layout->setColumnStretch(2, 1);
    layout->setContentsMargins(12, 0, 12, 12);
    m_widget->setLayout(layout);
}

QWidget* TmdbMovieConfigurationView::widget()
{
    return m_widget;
}

void TmdbMovieConfigurationView::load()
{
    mediaelch::Locale language = m_settings.language();
    const QString locale = language.toString('-');
    const QString lang = language.language();

    for (int i = 0, n = m_box->count(); i < n; ++i) {
        if (m_box->itemData(i).toString() == locale || m_box->itemData(i).toString() == lang) {
            m_box->setCurrentIndex(i);
            break;
        }
    }
}

void TmdbMovieConfigurationView::save()
{
    mediaelch::Locale language = m_box->itemData(m_box->currentIndex()).toString();
    m_settings.setLanguage(language);
}


} // namespace scraper
} // namespace mediaelch
