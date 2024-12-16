#include "siteinfo.h"

SiteInfo::SiteInfo(const int sn, const double b, const double a) : siteNum(sn), before(b), after(a) {}

QString SiteInfo::toString() const {
    QString record;
    //this is the overall record
    if (siteNum == -1) {
        record = "Overall before treatment: " + QString::number(before) + +"     Overall after treatment: " +
                 QString::number(after) + "\n";
    } else {
        record = "site: " + QString::number(siteNum) + " before: " + QString::number(before) + "    after: " +
                 QString::number(after) + "\n";
    }
    return record;
}

int SiteInfo::getSite() {
    return siteNum;
}
