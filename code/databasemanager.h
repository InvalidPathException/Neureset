#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <iostream>
#include <QVector>
#include <QVariant>
#include <QDebug>

#include "siteinfo.h"
#include "defs.h"


class DataBaseManager {
public:
    explicit DataBaseManager(const QString& connectionName);
    ~DataBaseManager();
    void addSession(const QString& date);
    void addBaseline(int s, double b, double a, const QString& date);
    QVector<QString> getSession();
    QVector<SiteInfo*> getSiteRecords(const QString& date);
    QString getDate();
    void updateDate(QString d);

private:
    QSqlDatabase neuresetDB;
    QVector<SiteInfo*> sites;
    void DBInit();
};

#endif // DATABASEMANAGER_H
