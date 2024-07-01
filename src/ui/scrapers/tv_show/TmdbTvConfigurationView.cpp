#include "ui/scrapers/tv_show/TmdbTvConfigurationView.h"

#include <QGridLayout>
#include <QLabel>

namespace mediaelch {
namespace scraper {


TmdbTvConfigurationView::TmdbTvConfigurationView(TmdbTvConfiguration& settings) :
    m_settings(settings), //
    m_widget{new QWidget},
    m_box{new QComboBox(m_widget)}
{
}

void TmdbTvConfigurationView::init()
{
    for (const mediaelch::Locale& lang : m_settings.supportedLanguages()) {
        m_box->addItem(lang.languageTranslated(), lang.toString());
    }

    auto* layout = new QGridLayout(m_widget);
    layout->addWidget(new QLabel(tr("Language")), 0, 0);
    layout->addWidget(m_box, 0, 1);
    layout->setColumnStretch(2, 1);
    layout->setContentsMargins(12, 0, 12, 12);
}

QWidget* TmdbTvConfigurationView::widget()
{
    // TODO: Make this a factory?
    QWidget* new_widget = new QWidget;
    QComboBox* box = new QComboBox(new_widget);

    for (const mediaelch::Locale& lang : m_settings.supportedLanguages()) {
        box->addItem(lang.languageTranslated(), lang.toString());
    }

    auto* layout = new QGridLayout(new_widget);
    layout->addWidget(new QLabel(tr("Language")), 0, 0);
    layout->addWidget(box, 0, 1);
    layout->setColumnStretch(2, 1);
    layout->setContentsMargins(12, 0, 12, 12);

    // FIXME: Settings take ownership!
    return new_widget;
}

void TmdbTvConfigurationView::load()
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

void TmdbTvConfigurationView::save()
{
    mediaelch::Locale language = m_box->itemData(m_box->currentIndex()).toString();
    m_settings.setLanguage(language);
}


} // namespace scraper
} // namespace mediaelch
