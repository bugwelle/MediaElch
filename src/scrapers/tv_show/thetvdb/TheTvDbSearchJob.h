#pragma once

#include "scrapers/tv_show/TvScraper.h"

#include <QJsonObject>
#include <QString>

namespace mediaelch {
namespace scraper {

class TheTvDbApi;

class TheTvDbSearchJob : public ShowSearchJob
{
    Q_OBJECT

public:
    explicit TheTvDbSearchJob(TheTvDbApi& api, ShowSearchJob::Config config, QObject* parent = nullptr);
    ~TheTvDbSearchJob() override = default;

    void execute() override;

private:
    TheTvDbApi& m_api;

    QVector<ShowSearchJob::Result> parseSearch(const QString& json);
    ShowSearchJob::Result parseSingleSearchResult(const QJsonObject& showObject);
};

} // namespace scraper
} // namespace mediaelch
