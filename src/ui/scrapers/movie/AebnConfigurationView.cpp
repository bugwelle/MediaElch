#include "ui/scrapers/movie/AebnConfigurationView.h"
#include <QGridLayout>
#include <QLabel>

namespace mediaelch {
namespace scraper {


AebnConfigurationView::AebnConfigurationView(AebnConfiguration& settings) :
    m_settings(settings), //
    m_widget{new QWidget},
    m_box{new QComboBox(m_widget)},
    m_genreBox{new QComboBox(m_widget)}
{
}

void AebnConfigurationView::init()
{
    for (const mediaelch::Locale& lang : m_settings.supportedLanguages()) {
        m_box->addItem(lang.languageTranslated(), lang.toString());
    }

    // Genre IDs overrides URL (http://[straight|gay]...)
    m_genreBox->addItem(QObject::tr("Straight"), "101");
    m_genreBox->addItem(QObject::tr("Gay"), "102");

    auto* layout = new QGridLayout(m_widget);
    layout->addWidget(new QLabel(QObject::tr("Language")), 0, 0);
    layout->addWidget(m_box, 0, 1);
    layout->addWidget(new QLabel(QObject::tr("Genre")), 1, 0);
    layout->addWidget(m_genreBox, 1, 1);
    layout->setColumnStretch(2, 1);
    layout->setContentsMargins(12, 0, 12, 12);
    m_widget->setLayout(layout);
}

QWidget* AebnConfigurationView::widget()
{
    return m_widget;
}

void AebnConfigurationView::load()
{
    mediaelch::Locale language = m_settings.language();
    for (int i = 0, n = m_box->count(); i < n; ++i) {
        if (m_box->itemData(i).toString() == language) {
            m_box->setCurrentIndex(i);
        }
    }
    QString genreId = m_settings.genreId();
    for (int i = 0, n = m_genreBox->count(); i < n; ++i) {
        if (m_genreBox->itemData(i).toString() == genreId) {
            m_genreBox->setCurrentIndex(i);
        }
    }
}

void AebnConfigurationView::save()
{
    mediaelch::Locale language = m_box->itemData(m_box->currentIndex()).toString();
    m_settings.setLanguage(language);

    QString genreId = m_genreBox->itemData(m_genreBox->currentIndex()).toString();
    m_settings.setGenreId(genreId);
}


} // namespace scraper
} // namespace mediaelch
