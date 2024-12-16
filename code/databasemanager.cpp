#include "databasemanager.h"

DataBaseManager::DataBaseManager(const QString& connectionName) {
    neuresetDB = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    neuresetDB.setDatabaseName("neureset.db");

    if (!neuresetDB.open()) {
        qDebug() << "Failed to open database:";
    } else {
        qDebug() << "Database opened successfully.";
    }
    DBInit();
}

DataBaseManager::~DataBaseManager() {
    if (!sites.isEmpty()) {
        for (SiteInfo *s: sites) {
            delete s;
        }
        sites.clear();
    }
    neuresetDB.close();
}

// Initialize the database
void DataBaseManager::DBInit() {
    neuresetDB.transaction();
    QSqlQuery stmt(neuresetDB);

    stmt.exec(
            "CREATE TABLE IF NOT EXISTS Sessions  ( SID INTEGER PRIMARY KEY AUTOINCREMENT, SDATE VARCHAR(30) UNIQUE NOT NULL);");
    stmt.exec(
            "CREATE TABLE IF NOT EXISTS Baselines ( BID INTEGER PRIMARY KEY AUTOINCREMENT,SITE INT, BEFORE DOUBLE, AFTER DOUBLE, SID INT, foreign key (SID) references SESSIONS (SID));");
    stmt.exec("CREATE TABLE IF NOT EXISTS Date (CurrentDate VARCHAR(30));");
    if (!neuresetDB.commit()) std::cerr << "Error: Fail to initialize the database" << std::endl;

    //check if this is the first time database initialize
    stmt.prepare("SELECT CurrentDate FROM date");
    stmt.exec();
    if (!stmt.next()) {
        qDebug() << "yes";
        //DEFAULT DATE
        QString date = "2024-01-01 00:00:00";
        stmt.prepare("INSERT INTO Date (CurrentDate) VALUES (:date)");
        stmt.bindValue(":date", date);
        stmt.exec();
    }
}

// Add a session to the database (can be incomplete session)
void DataBaseManager::addSession(const QString& date) {
    neuresetDB.transaction();

    QSqlQuery stmt(neuresetDB);
    stmt.prepare("INSERT INTO Sessions (sdate) VALUES (:date)");
    stmt.bindValue(":date", date);
    stmt.exec();

    if (!neuresetDB.commit()) {
        std::cerr << "Error: Failed to insert session into the database." << std::endl;
    }
}

// Add treatment data to db
void DataBaseManager::addBaseline(int s, double b, double a, const QString& d) {
    int sid = -1;
    QSqlQuery stmt(neuresetDB);
    neuresetDB.transaction();

    stmt.prepare("SELECT sid FROM Sessions WHERE sdate = :date");
    stmt.bindValue(":date", d);
    stmt.exec();
    if (stmt.next()) {
        sid = stmt.value(0).toInt();
    } else {
        std::cerr << "Error: Session ID not found for the given date." << std::endl;
        return;
    }

    stmt.prepare("INSERT INTO Baselines (SITE, BEFORE, AFTER, SID) VALUES (:s, :b, :a, :sid)");
    stmt.bindValue(":s", s);
    stmt.bindValue(":b", b);
    stmt.bindValue(":a", a);
    stmt.bindValue(":sid", sid);
    stmt.exec();

    if (!neuresetDB.commit()) {
        std::cerr << "Error: Failed to commit baseline values info to the database." << std::endl;
    }
}

// Get session info from database for display
QVector<QString> DataBaseManager::getSession() {
    QVector<QString> sessions;

    QSqlQuery stmt(neuresetDB);
    stmt.exec("SELECT * FROM SESSIONS");
    while (stmt.next()) {
        sessions.push_back(stmt.value(1).toString());
    }
    return sessions;
}

// Get the record for a particular date
QVector<SiteInfo *> DataBaseManager::getSiteRecords(const QString& date) {
    //clear the qvector
    if (!sites.isEmpty()) {
        for (SiteInfo *s: sites) {
            delete s;
        }
        sites.clear();
    }
    int sid = -1;
    QSqlQuery stmt(neuresetDB);

    stmt.prepare("SELECT sid FROM Sessions WHERE sdate = :date");
    stmt.bindValue(":date", date);
    stmt.exec();
    if (stmt.next()) {
        sid = stmt.value(0).toInt();
    } else {
        std::cerr << "Error: Session ID not found for the given date." << std::endl;
        return sites;
    }

    stmt.prepare("SELECT * FROM Baselines WHERE sid = :id");
    stmt.bindValue(":id", sid);
    stmt.exec();

    while (stmt.next()) {
        SiteInfo *site = new SiteInfo(stmt.value(1).toInt(), stmt.value(2).toDouble(), stmt.value(3).toDouble());
        sites.push_back(site);
    }
    return sites;
}

QString DataBaseManager::getDate() {
    QString date = "";
    QSqlQuery stmt(neuresetDB);
    stmt.prepare("SELECT CurrentDate FROM date");
    stmt.exec();
    if (stmt.next()) {
        date = stmt.value(0).toString();
    } else {
        std::cerr << "Error: Can't find current date." << std::endl;
    }
    return date;
}

void DataBaseManager::updateDate(QString d) {
    QSqlQuery stmt(neuresetDB);
    stmt.prepare("UPDATE DATE SET CURRENTDATE = :d");
    stmt.bindValue(":d", d);
    stmt.exec();
}
