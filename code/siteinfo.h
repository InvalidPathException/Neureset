#ifndef SITEINFO_H
#define SITEINFO_H

#include <QString>

class SiteInfo {
public:
    SiteInfo(const int siteNum, const double before, const double after);
    QString toString() const;
    int getSite();

private:
    int siteNum;
    double before;
    double after;
};

#endif // SITEINFO_H

