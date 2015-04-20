#include <limits>

#include <QStringList>
#include <QtDebug>
#include <cstdlib>
#include <cstring>
#include "util/stat.h"
#include "util/time.h"
#include "util/math.h"
#include "util/statsmanager.h"

Stat::Stat()
        : m_type(UNSPECIFIED),
          m_compute(NONE),
          m_report_count(0),
          m_bins(1024),
          m_bin_count(m_bins.size()),
          m_bin_low_count(0),
          m_bin_high_count(0),
          m_bin_left(0),
          m_bin_right(0),
          m_bin_scale(0),
          m_sum(0),
          m_min(std::numeric_limits<double>::max()),
          m_max(std::numeric_limits<double>::min()),
          m_variance_mk(0),
          m_variance_sk(0) {
}

QString Stat::valueUnits() const {
    switch (m_type) {
        case DURATION_MSEC:
            return "ms";
        case DURATION_NANOSEC:
            return "ns";
        case DURATION_SEC:
            return "s";
        case EVENT:
        case EVENT_START:
        case EVENT_END:
        case UNSPECIFIED:
        default:
            return "";
    }
}

void Stat::rebin(){
  if(!m_report_count)
    return;
  double var      = variance();
  double mean     = m_sum/m_report_count;
  m_bin_low_count = 0;
  m_bin_high_count= 0;
  m_bin_left      = mean - sqrt(var);
  m_bin_right     = mean + sqrt(var);
  m_bin_scale     = (m_bin_count-1)/(m_bin_right-m_bin_left);
  memset(&m_bins[0],0,m_bin_count*sizeof(quint64));
  for(quint64 i = 0; i < m_report_count;i++){
    double val = m_values[i];
    if(val<m_bin_left)m_bin_low_count++;
    else if(val>m_bin_right)m_bin_high_count++;
    else m_bins[(int)((val-m_bin_left)*m_bin_scale)]++;
  }
}
double Stat::approximate_median(){
  if(!m_report_count)return 0;
  if(!m_bin_scale && m_report_count)
    rebin();
  quint64 low = m_bin_low_count;
  int     bin = 0;
  for(;bin<m_bins.size() && low+m_bins[bin]<m_report_count/2;low+=m_bins[bin],bin++){
  }
  if(bin==m_bins.size()||bin==0){
    rebin();
    low = m_bin_low_count;bin=0;
    for(;bin<m_bins.size() && low+m_bins[bin]<m_report_count/2;low+=m_bins[bin],bin++){
    }
  }
  return m_bin_left + (bin/m_bin_scale);
}
void Stat::processReport(StatReport& report) {
    m_report_count++;
    if (m_compute & (Stat::SUM | Stat::AVERAGE)) {
        m_sum += report.value;
    }
    if (m_compute & Stat::MAX && report.value > m_max) {
        m_max = report.value;
    }
    if (m_compute & Stat::MIN && report.value < m_min) {
        m_min = report.value;
    }

    // Method comes from Knuth, see:
    // http://www.johndcook.com/standard_deviation.html
    if (m_compute & Stat::SAMPLE_VARIANCE) {
        if (m_report_count == 0.0) {
            m_variance_mk = report.value;
            m_variance_sk = 0.0;
        } else {
            double variance_mk_prev = m_variance_mk;
            m_variance_mk += (report.value - m_variance_mk) / m_report_count;
            m_variance_sk += (report.value - variance_mk_prev) * (report.value - m_variance_mk);
        }
    }
    if(m_compute & Stat::HISTOGRAM || m_compute&Stat::SAMPLE_MEDIAN){
      if ( m_bin_scale ){
        double val = report.value;
        if(val<m_bin_left)m_bin_low_count++;
        else if(val>m_bin_right) m_bin_high_count++;
        else m_bins[(int)((val-m_bin_left)*m_bin_scale)]++;
      }
      if (m_compute & Stat::HISTOGRAM) {
          m_histogram[report.value] += 1.0;
      }
        m_values.push_back(report.value);
        m_approximate_median = approximate_median();
    }else if (m_compute & Stat::VALUES) {
        m_values.push_back(report.value);
    }
}
QDebug operator<<(QDebug dbg, const Stat &stat) {
    QStringList stats;
    if (stat.m_compute & Stat::COUNT) {
        stats << "count=" + QString::number(stat.m_report_count);
    }

    if (stat.m_compute & Stat::SUM) {
        stats << "sum=" + QString::number(stat.m_sum) + stat.valueUnits();
    }

    if (stat.m_compute & Stat::AVERAGE) {
        QString value = "average=";
        if (stat.m_report_count > 0) {
            value += QString::number(stat.m_sum / stat.m_report_count) + stat.valueUnits();
        } else {
            value += "XXX";
        }
        stats << value;
    }

    if (stat.m_compute & Stat::MIN) {
        QString value = "min=";
        if (stat.m_report_count > 0) {
            value += QString::number(stat.m_min) + stat.valueUnits();
        } else {
            value += "XXX";
        }
        stats << value;
    }

    if (stat.m_compute & Stat::MAX) {
        QString value = "max=";
        if (stat.m_report_count > 0) {
            value += QString::number(stat.m_max) + stat.valueUnits();
        } else {
            value += "XXX";
        }
        stats << value;
    }

    if (stat.m_compute & Stat::SAMPLE_VARIANCE) {
        double variance = stat.variance();
        stats << "variance=" + QString::number(variance) + stat.valueUnits() + "^2";
        if (variance >= 0.0) {
            stats << "stddev=" + QString::number(sqrt(variance)) + stat.valueUnits();
        }
    }

    if (stat.m_compute & Stat::SAMPLE_MEDIAN) {
      double median = stat.m_approximate_median;
        stats << "median="+QString::number(median) + stat.valueUnits();
    }

    if (stat.m_compute & Stat::HISTOGRAM) {
        QStringList histogram;
        histogram << "(-inf,"+QString::number(stat.m_bin_left)+"]:"+QString::number(stat.m_bin_low_count);
        for(int i =0;i<stat.m_bins.size();i++){
          histogram << "["+QString::number(stat.m_bin_left+i/stat.m_bin_scale)+","+QString::number(stat.m_bin_left+(i+1)/stat.m_bin_scale)+"):" +QString::number(stat.m_bins[i]);
        }
        histogram << "["+QString::number(stat.m_bin_left+(stat.m_bins.size()/stat.m_bin_scale))+",+inf):"+QString::number(stat.m_bin_high_count);
        stats << "histogram=" + histogram.join(",");
    }

    dbg.nospace() << "Stat(" << stat.m_tag << "," << stats.join(",") << ")";
    return dbg.maybeSpace();
}

// static
bool Stat::track(const QString& tag,
                 Stat::StatType type,
                 Stat::ComputeFlags compute,
                 double value) {
    if (!StatsManager::s_bStatsManagerEnabled) {
        return false;
    }
    StatReport report;
    report.tag = strdup(tag.toAscii().constData());
    report.type = type;
    report.compute = compute;
    report.time = Time::elapsed();
    report.value = value;
    StatsManager* pManager = StatsManager::instance();
    return pManager && pManager->maybeWriteReport(report);
}
