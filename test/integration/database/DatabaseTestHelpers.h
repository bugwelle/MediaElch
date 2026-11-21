#pragma once

#include "test/test_helpers.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace test {

inline int rowCount(QSqlDatabase db, const QString& table)
{
    QSqlQuery query(db);
    REQUIRE(query.exec("SELECT COUNT(*) FROM " + table));
    REQUIRE(query.next());
    return query.value(0).toInt();
}

inline void executeSql(QSqlDatabase db, const QString& sql)
{
    QSqlQuery query(db);
    const bool ok = query.exec(sql);
    INFO(query.lastError().text().toStdString());
    REQUIRE(ok);
}

} // namespace test
