/***************************************************************************
 *  foxxll/io/iostats.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008-2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009, 2010 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2016 Alex Schickedanz <alex@ae.cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <iomanip>
#include <mutex>
#include <numeric>
#include <sstream>
#include <string>

#include <foxxll/common/log.hpp>
#include <foxxll/common/timer.hpp>
#include <foxxll/common/types.hpp>
#include <foxxll/io/iostats.hpp>
#include <foxxll/verbose.hpp>

namespace foxxll {

/******************************************************************************/
// file_stats

file_stats::file_stats(unsigned int device_id)
    : device_id_(device_id),
      read_count_(0), write_count_(0),
      read_bytes_(0), write_bytes_(0),
      read_time_(0.0), write_time_(0.0),
      p_begin_read_(0.0), p_begin_write_(0.0),
      acc_reads_(0), acc_writes_(0)
{ }

void file_stats::write_started(const size_t size, double now)
{
    if (now == 0.0)
        now = timestamp();

    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        ++write_count_;
        write_bytes_ += size;
        const double diff = now - p_begin_write_;
        write_time_ += (acc_writes_++) * diff;
        p_begin_write_ = now;
    }

    stats::get_instance()->p_write_started(now);
}

void file_stats::write_canceled(const size_t size)
{
    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        --write_count_;
        write_bytes_ -= size;
    }
    write_finished();
}

void file_stats::write_finished()
{
    double now = timestamp();

    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        const double diff = now - p_begin_write_;
        write_time_ += (acc_writes_--) * diff;
        p_begin_write_ = now;
    }

    stats::get_instance()->p_write_finished(now);
}

void file_stats::read_started(const size_t size, double now)
{
    if (now == 0.0)
        now = timestamp();

    {
        std::unique_lock<std::mutex> read_lock(read_mutex_);

        ++read_count_;
        read_bytes_ += size;
        const double diff = now - p_begin_read_;
        read_time_ += (acc_reads_++) * diff;
        p_begin_read_ = now;
    }

    stats::get_instance()->p_read_started(now);
}

void file_stats::read_canceled(const size_t size)
{
    {
        std::unique_lock<std::mutex> read_lock(read_mutex_);

        --read_count_;
        read_bytes_ -= size;
    }
    read_finished();
}

void file_stats::read_finished()
{
    double now = timestamp();

    {
        std::unique_lock<std::mutex> read_lock(read_mutex_);

        const double diff = now - p_begin_read_;
        read_time_ += (acc_reads_--) * diff;
        p_begin_read_ = now;
    }

    stats::get_instance()->p_read_finished(now);
}

/******************************************************************************/
// file_stats_data

file_stats_data file_stats_data::operator + (const file_stats_data& a) const
{
    STXXL_THROW_IF(device_id_ != a.device_id_, std::runtime_error,
                   "foxxll::file_stats_data objects do not belong to the same file/disk");

    file_stats_data fsd;
    fsd.device_id_ = device_id_;

    fsd.read_count_ = read_count_ + a.read_count_;
    fsd.write_count_ = write_count_ + a.write_count_;
    fsd.read_bytes_ = read_bytes_ + a.read_bytes_;
    fsd.write_bytes_ = write_bytes_ + a.write_bytes_;
    fsd.read_time_ = read_time_ + a.read_time_;
    fsd.write_time_ = write_time_ + a.write_time_;

    return fsd;
}

file_stats_data file_stats_data::operator - (const file_stats_data& a) const
{
    STXXL_THROW_IF(device_id_ != a.device_id_, std::runtime_error,
                   "foxxll::file_stats_data objects do not belong to the same file/disk");

    file_stats_data fsd;
    fsd.device_id_ = device_id_;

    fsd.read_count_ = read_count_ - a.read_count_;
    fsd.write_count_ = write_count_ - a.write_count_;
    fsd.read_bytes_ = read_bytes_ - a.read_bytes_;
    fsd.write_bytes_ = write_bytes_ - a.write_bytes_;
    fsd.read_time_ = read_time_ - a.read_time_;
    fsd.write_time_ = write_time_ - a.write_time_;

    return fsd;
}

/******************************************************************************/
// stats

stats::stats()
    : creation_time_(timestamp()),
      p_reads_(0.0), p_writes_(0.0),
      p_begin_read_(0.0), p_begin_write_(0.0),
      p_ios_(0.0),
      p_begin_io_(0.0),

      acc_reads_(0.0), acc_writes_(0.0),
      acc_ios_(0.0),

      t_waits_(0.0), p_waits_(0.0),
      p_begin_wait_(0.0),
      t_wait_read_(0.0), p_wait_read_(0.0),
      p_begin_wait_read_(0.0),
      t_wait_write_(0.0), p_wait_write_(0.0),
      p_begin_wait_write_(0.0),
      acc_waits_(0.0),
      acc_wait_read_(0.0), acc_wait_write_(0.0)
{ }

#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
void stats::wait_started(wait_op_type wait_op)
{
    const double now = timestamp();
    {
        std::unique_lock<std::mutex> wait_lock(wait_mutex_);

        double diff = now - p_begin_wait_;
        t_waits_ += acc_waits_ * diff;
        p_begin_wait_ = now;
        p_waits_ += (acc_waits_++) ? diff : 0.0;

        if (wait_op == WAIT_OP_READ) {
            diff = now - p_begin_wait_read_;
            t_wait_read_ += acc_wait_read_ * diff;
            p_begin_wait_read_ = now;
            p_wait_read_ += (acc_wait_read_++) ? diff : 0.0;
        }
        else /* if (wait_op == WAIT_OP_WRITE) */ {
            // wait_any() is only used from write_pool and buffered_writer, so account WAIT_OP_ANY for WAIT_OP_WRITE, too
            diff = now - p_begin_wait_write_;
            t_wait_write_ += acc_wait_write_ * diff;
            p_begin_wait_write_ = now;
            p_wait_write_ += (acc_wait_write_++) ? diff : 0.0;
        }
    }
}

void stats::wait_finished(const wait_op_type wait_op)
{
    const double now = timestamp();
    {
        std::unique_lock<std::mutex> wait_lock(wait_mutex_);

        double diff = now - p_begin_wait_;
        t_waits_ += acc_waits_ * diff;
        p_begin_wait_ = now;
        p_waits_ += (acc_waits_--) ? diff : 0.0;

        if (wait_op == WAIT_OP_READ) {
            double diff2 = now - p_begin_wait_read_;
            t_wait_read_ += acc_wait_read_ * diff2;
            p_begin_wait_read_ = now;
            p_wait_read_ += (acc_wait_read_--) ? diff2 : 0.0;
        }
        else /* if (wait_op == WAIT_OP_WRITE) */ {
            double diff2 = now - p_begin_wait_write_;
            t_wait_write_ += acc_wait_write_ * diff2;
            p_begin_wait_write_ = now;
            p_wait_write_ += (acc_wait_write_--) ? diff2 : 0.0;
        }
#ifdef STXXL_WAIT_LOG_ENABLED
        std::ofstream* waitlog = foxxll::logger::get_instance()->waitlog_stream();
        if (waitlog)
            *waitlog << (now - last_reset) << "\t"
                     << ((wait_op == WAIT_OP_READ) ? diff : 0.0) << "\t"
                     << ((wait_op != WAIT_OP_READ) ? diff : 0.0) << "\t"
                     << t_wait_read_ << "\t" << t_wait_write_ << "\n" << line_prefix << std::flush;
#endif
    }
}
#endif

void stats::p_write_started(const double now)
{
    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        const double diff = now - p_begin_write_;
        p_begin_write_ = now;
        p_writes_ += (acc_writes_++) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex_);

        const double diff = now - p_begin_io_;
        p_ios_ += (acc_ios_++) ? diff : 0.0;
        p_begin_io_ = now;
    }
}

void stats::p_write_finished(const double now)
{
    {
        std::unique_lock<std::mutex> write_lock(write_mutex_);

        const double diff = now - p_begin_write_;
        p_begin_write_ = now;
        p_writes_ += (acc_writes_--) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex_);

        const double diff = now - p_begin_io_;
        p_ios_ += (acc_ios_--) ? diff : 0.0;
        p_begin_io_ = now;
    }
}

void stats::p_read_started(const double now)
{
    {
        std::unique_lock<std::mutex> read_lock(read_mutex_);

        const double diff = now - p_begin_read_;
        p_begin_read_ = now;
        p_reads_ += (acc_reads_++) ? diff : 0.0;
    }
    {
        std::unique_lock<std::mutex> io_lock(io_mutex_);

        const double diff = now - p_begin_io_;
        p_ios_ += (acc_ios_++) ? diff : 0.0;
        p_begin_io_ = now;
    }
}

void stats::p_read_finished(const double now)
{
    {
        const double diff = now - p_begin_read_;
        p_begin_read_ = now;
        p_reads_ += (acc_reads_--) ? diff : 0.0;
    }
    {
        const double diff = now - p_begin_io_;
        p_ios_ += (acc_ios_--) ? diff : 0.0;
        p_begin_io_ = now;
    }
}

file_stats* stats::create_file_stats(unsigned int device_id)
{
    file_stats_list_.emplace_back(device_id);
    return &file_stats_list_.back();
}

std::vector<file_stats_data> stats::deepcopy_file_stats_data_list() const
{
    return {
               file_stats_list_.cbegin(), file_stats_list_.cend()
    };
}

std::ostream& operator << (std::ostream& o, const stats& s)
{
    o << stats_data(s);
    return o;
}


/******************************************************************************/
// stats_data

template <typename T, typename Functor>
T stats_data::fetch_sum(const Functor& get_value) const
{
    return std::accumulate(file_stats_data_list_.cbegin(), file_stats_data_list_.cend(), T(0),
                           [get_value](T sum, const auto& x) { return sum + get_value(x); });
}

template <typename T>
template <typename Functor>
stats_data::summary<T>::summary(
    const std::vector<file_stats_data>& fs, const Functor& get_value)
{
    values_per_device.reserve(fs.size());
    for (const auto& f : fs) {
        values_per_device.emplace_back(get_value(f), f.get_device_id());
    }

    std::sort(values_per_device.begin(), values_per_device.end(),
              [](std::pair<T, unsigned> a, std::pair<T, unsigned> b) {
                  return a.first < b.first;
              });

    if (values_per_device.size() != 0)
    {
        min = values_per_device.front().first;
        max = values_per_device.back().first;

        const size_t mid = values_per_device.size() / 2;
        median =
            (values_per_device.size() % 2)
            ? values_per_device[mid].first
            : (values_per_device[mid - 1].first + values_per_device[mid].first) / 2.0;

        total = std::accumulate(values_per_device.cbegin(), values_per_device.cend(), T(0),
                                [](T sum, const auto& x) { return sum + x.first; });

        average = static_cast<double>(total) / values_per_device.size();
    }
    else
    {
        min = std::numeric_limits<T>::quiet_NaN();
        max = std::numeric_limits<T>::quiet_NaN();
        median = std::numeric_limits<T>::quiet_NaN();
        total = std::numeric_limits<T>::quiet_NaN();
        average = std::numeric_limits<T>::quiet_NaN();
    }
}

stats_data stats_data::operator + (const stats_data& a) const
{
    stats_data s;

    if (a.file_stats_data_list_.size() == 0)
    {
        s.file_stats_data_list_ = file_stats_data_list_;
    }
    else if (file_stats_data_list_.size() == 0)
    {
        s.file_stats_data_list_ = a.file_stats_data_list_;
    }
    else if (file_stats_data_list_.size() == a.file_stats_data_list_.size())
    {
        for (auto it1 = file_stats_data_list_.cbegin(),
             it2 = a.file_stats_data_list_.cbegin();
             it1 != file_stats_data_list_.cend(); it1++, it2++)
        {
            s.file_stats_data_list_.push_back((*it1) + (*it2));
        }
    }
    else
    {
        STXXL_THROW(std::runtime_error,
                    "The number of files has changed between the snapshots.");
    }

    s.p_reads_ = p_reads_ + a.p_reads_;
    s.p_writes_ = p_writes_ + a.p_writes_;
    s.p_ios_ = p_ios_ + a.p_ios_;
    s.t_wait = t_wait + a.t_wait;
    s.t_wait_read_ = t_wait_read_ + a.t_wait_read_;
    s.t_wait_write_ = t_wait_write_ + a.t_wait_write_;
    s.elapsed_ = elapsed_ + a.elapsed_;
    return s;
}

stats_data stats_data::operator - (const stats_data& a) const
{
    stats_data s;

    if (a.file_stats_data_list_.size() == 0)
    {
        s.file_stats_data_list_ = file_stats_data_list_;
    }
    else if (file_stats_data_list_.size() == a.file_stats_data_list_.size())
    {
        for (auto it1 = file_stats_data_list_.cbegin(),
             it2 = a.file_stats_data_list_.cbegin();
             it1 != file_stats_data_list_.cend(); it1++, it2++)
        {
            s.file_stats_data_list_.push_back((*it1) - (*it2));
        }
    }
    else
    {
        STXXL_THROW(std::runtime_error,
                    "The number of files has changed between the snapshots.");
    }

    s.p_reads_ = p_reads_ - a.p_reads_;
    s.p_writes_ = p_writes_ - a.p_writes_;
    s.p_ios_ = p_ios_ - a.p_ios_;
    s.t_wait = t_wait - a.t_wait;
    s.t_wait_read_ = t_wait_read_ - a.t_wait_read_;
    s.t_wait_write_ = t_wait_write_ - a.t_wait_write_;
    s.elapsed_ = elapsed_ - a.elapsed_;
    return s;
}

size_t stats_data::num_files() const
{
    return file_stats_data_list_.size();
}

unsigned stats_data::get_read_count() const
{
    return fetch_sum<unsigned>(
        [](const file_stats_data& fsd) { return fsd.get_read_count(); });
}

stats_data::summary<unsigned> stats_data::get_read_count_summary() const
{
    return {
               file_stats_data_list_, [](const file_stats_data& fsd) {
                   return fsd.get_read_count();
               }
    };
}

unsigned stats_data::get_write_count() const
{
    return fetch_sum<unsigned>(
        [](const file_stats_data& fsd) { return fsd.get_write_count(); });
}

stats_data::summary<unsigned> stats_data::get_write_count_summary() const
{
    return {
               file_stats_data_list_, [](const file_stats_data& fsd) {
                   return fsd.get_write_count();
               }
    };
}

external_size_type stats_data::get_read_bytes() const
{
    return fetch_sum<external_size_type>(
        [](const file_stats_data& fsd) { return fsd.get_read_bytes(); });
}

stats_data::summary<external_size_type>
stats_data::get_read_bytes_summary() const
{
    return {
               file_stats_data_list_, [](const file_stats_data& fsd) {
                   return fsd.get_read_bytes();
               }
    };
}

external_size_type stats_data::get_write_bytes() const
{
    return fetch_sum<external_size_type>(
        [](const file_stats_data& fsd) { return fsd.get_write_bytes(); });
}

stats_data::summary<external_size_type>
stats_data::get_write_bytes_summary() const
{
    return {
               file_stats_data_list_, [](const file_stats_data& fsd) {
                   return fsd.get_write_bytes();
               }
    };
}

double stats_data::get_read_time() const
{
    return fetch_sum<double>(
        [](const file_stats_data& fsd) { return fsd.get_read_time(); });
}

stats_data::summary<double> stats_data::get_read_time_summary() const
{
    return {
               file_stats_data_list_, [](const file_stats_data& fsd) {
                   return fsd.get_read_time();
               }
    };
}

double stats_data::get_write_time() const
{
    return fetch_sum<double>(
        [](const file_stats_data& fsd) { return fsd.get_write_time(); });
}

stats_data::summary<double> stats_data::get_write_time_summary() const
{
    return {
               file_stats_data_list_, [](const file_stats_data& fsd) {
                   return fsd.get_write_time();
               }
    };
}

double stats_data::get_pread_time() const
{
    return p_reads_;
}

double stats_data::get_pwrite_time() const
{
    return p_writes_;
}

double stats_data::get_pio_time() const
{
    return p_ios_;
}

stats_data::summary<double> stats_data::get_read_speed_summary() const
{
    return {
               file_stats_data_list_, [](const file_stats_data& fsd) {
                   return static_cast<double>(fsd.get_read_bytes()) / fsd.get_read_time();
               }
    };
}

stats_data::summary<double> stats_data::get_pread_speed_summary() const
{
    return {
               file_stats_data_list_, [this](const file_stats_data& fsd) {
                   return static_cast<double>(fsd.get_read_bytes()) / p_reads_;
               }
    };
}

stats_data::summary<double> stats_data::get_write_speed_summary() const
{
    return {
               file_stats_data_list_, [](const file_stats_data& fsd) {
                   return static_cast<double>(fsd.get_write_bytes()) / fsd.get_write_time();
               }
    };
}

stats_data::summary<double> stats_data::get_pwrite_speed_summary() const
{
    return {
               file_stats_data_list_, [this](const file_stats_data& fsd) {
                   return static_cast<double>(fsd.get_write_bytes()) / p_writes_;
               }
    };
}

stats_data::summary<double> stats_data::get_pio_speed_summary() const
{
    return {
               file_stats_data_list_, [this](const file_stats_data& fsd) {
                   return static_cast<double>(fsd.get_read_bytes() + fsd.get_write_bytes()) / p_ios_;
               }
    };
}

double stats_data::get_io_wait_time() const
{
    return t_wait;
}

double stats_data::get_wait_read_time() const
{
    return t_wait_read_;
}

double stats_data::get_wait_write_time() const
{
    return t_wait_write_;
}

std::string format_with_SI_IEC_unit_multiplier(
    const uint64_t number, const std::string& unit, int multiplier)
{
    // may not overflow, std::numeric_limits<uint64_t>::max() == 16 EB
    static const std::array<std::string, 7> endings {
        "", "k", "M", "G", "T", "P", "E"
    };
    static const std::array<std::string, 7> binary_endings {
        "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei"
    };

    std::ostringstream out;
    out << number << ' ';

    size_t scale = 0;
    auto number_d = static_cast<double>(number);
    double multiplier_d = multiplier;
    while (number_d >= multiplier_d)
    {
        number_d /= multiplier_d;
        ++scale;
    }

    if (scale > 0) {
        out << '(' << std::fixed << std::setprecision(3) << number_d << ' '
            << (multiplier == 1024 ? binary_endings[scale] : endings[scale])
            << unit << ") ";
    }
    else {
        out << unit;
    }

    return out.str();
}

void stats_data::to_ostream(std::ostream& o, const std::string line_prefix) const
{
    constexpr double one_mib = 1024.0 * 1024;

    o << "STXXL I/O statistics\n" << line_prefix;

    size_t nf = num_files();
    if (nf != 1) {
        o << " number of disks/files                      : "
          << nf << "\n" << line_prefix;
    }

    const auto read_bytes_summary = get_read_bytes_summary();
    const auto write_bytes_summary = get_write_bytes_summary();

    const auto read_speed_summary = get_read_speed_summary();
    const auto pread_speed_summary = get_pread_speed_summary();
    const auto write_speed_summary = get_write_speed_summary();
    const auto pwrite_speed_summary = get_pwrite_speed_summary();
    const auto pio_speed_summary = get_pio_speed_summary();

    o << " total number of reads                      : "
      << add_IEC_binary_multiplier(get_read_count()) << "\n" << line_prefix;
    o << " average block size (read)                  : "
      << add_IEC_binary_multiplier(get_read_count() ? get_read_bytes() / get_read_count() : 0, "B")
      << "\n" << line_prefix;
    o << " number of bytes read from disks            : "
      << add_IEC_binary_multiplier(get_read_bytes(), "B") << "\n" << line_prefix;
    o << " time spent in serving all read requests    : "
      << get_read_time() << " s"
      << " @ " << (static_cast<double>(get_read_bytes()) / one_mib / get_read_time()) << " MiB/s";
    if (nf > 1) {
        o << " (min: " << read_speed_summary.min / one_mib << " MiB/s, "
          << "max: " << read_speed_summary.max / one_mib << " MiB/s)";
    }
    o << "\n" << line_prefix;
    o << " time spent in reading (parallel read time) : "
      << get_pread_time() << " s"
      << " @ " << (static_cast<double>(get_read_bytes()) / one_mib / get_pread_time()) << " MiB/s"
      << "\n" << line_prefix;
    if (nf > 1) {
        o << "  reading speed per file                    : "
          << "min: " << pread_speed_summary.min / one_mib << " MiB/s, "
          << "median: " << pread_speed_summary.median / one_mib << " MiB/s, "
          << "max: " << pread_speed_summary.max / one_mib << " MiB/s"
          << "\n" << line_prefix;
    }

    o << " total number of writes                     : "
      << add_IEC_binary_multiplier(get_write_count()) << "\n" << line_prefix
      << " average block size (write)                 : "
      << add_IEC_binary_multiplier(get_write_count() ? get_write_bytes() / get_write_count() : 0, "B") << "\n" << line_prefix
      << " number of bytes written to disks           : "
      << add_IEC_binary_multiplier(get_write_bytes(), "B") << "\n" << line_prefix
      << " time spent in serving all write requests   : "
      << get_write_time() << " s"
      << " @ " << (static_cast<double>(get_write_bytes()) / one_mib / get_write_time()) << " MiB/s";

    if (nf > 1) {
        o << " (min: " << write_speed_summary.min / one_mib << " MiB/s, "
          << "max: " << write_speed_summary.max / one_mib << " MiB/s)";
    }
    o << "\n" << line_prefix;

    o << " time spent in writing (parallel write time): " << get_pwrite_time() << " s"
      << " @ " << (static_cast<double>(get_write_bytes()) / one_mib / get_pwrite_time()) << " MiB/s"
      << "\n" << line_prefix;
    if (nf > 1) {
        o << "   parallel write speed per file            : "
          << "min: " << pwrite_speed_summary.min / one_mib << " MiB/s, "
          << "median: " << pwrite_speed_summary.median / one_mib << " MiB/s, "
          << "max: " << pwrite_speed_summary.max / one_mib << " MiB/s"
          << "\n" << line_prefix;
    }

    o << " time spent in I/O (parallel I/O time)      : " << get_pio_time() << " s"
      << " @ " << (static_cast<double>((get_read_bytes()) + get_write_bytes()) / one_mib / get_pio_time()) << " MiB/s"
      << "\n" << line_prefix;
    if (nf > 1) {
        o << "   parallel I/O speed per file              : "
          << "min: " << pio_speed_summary.min / one_mib << " MiB/s, "
          << "median: " << pio_speed_summary.median / one_mib << " MiB/s, "
          << "max: " << pio_speed_summary.max / one_mib << " MiB/s"
          << "\n" << line_prefix;
    }
#ifndef STXXL_DO_NOT_COUNT_WAIT_TIME
    o << " I/O wait time                              : "
      << get_io_wait_time() << " s\n" << line_prefix;
    if (get_wait_read_time() != 0.0)
        o << " I/O wait4read time                         : "
          << get_wait_read_time() << " s\n" << line_prefix;
    if (get_wait_write_time() != 0.0)
        o << " I/O wait4write time                        : "
          << get_wait_write_time() << " s\n" << line_prefix;
#endif
    o << " Time since the last reset                  : "
      << get_elapsed_time() << " s";

    if (pio_speed_summary.min / pio_speed_summary.max < 0.5 ||
        pread_speed_summary.min / pread_speed_summary.max < 0.5 ||
        pwrite_speed_summary.min / pwrite_speed_summary.max < 0.5)
    {
        o << "\n" << line_prefix
          << "WARNING: Slow disk(s) detected.\n" << line_prefix
          << " Reading: ";
        o << pread_speed_summary.values_per_device.front().second
          << "@ " << pread_speed_summary.values_per_device.front().first / one_mib << " MiB/s";
        for (int i = 1; pread_speed_summary.values_per_device[i].first / pread_speed_summary.values_per_device.back().first < 0.5; ++i)
        {
            o << ", " << pread_speed_summary.values_per_device[i].second
              << "@ " << pread_speed_summary.values_per_device[i].first / one_mib << " MiB/s";
        }
        o << "\n" << line_prefix
          << " Writing: "
          << pwrite_speed_summary.values_per_device.front().second
          << "@ " << pwrite_speed_summary.values_per_device.front().first / one_mib << " MiB/s";
        for (int i = 1; pwrite_speed_summary.values_per_device[i].first / pwrite_speed_summary.values_per_device.back().first < 0.5; ++i)
        {
            o << ", " << pwrite_speed_summary.values_per_device[i].second
              << "@ " << pwrite_speed_summary.values_per_device[i].first / one_mib << " MiB/s";
        }
    }

    if (static_cast<double>(read_bytes_summary.min) / read_bytes_summary.max < 0.5 ||
        static_cast<double>(write_bytes_summary.min) / write_bytes_summary.max < 0.5)
    {
        o << "\n" << line_prefix <<
            "WARNING: Bad load balancing.\n" << line_prefix
          << " Smallest read load on disk  "
          << read_bytes_summary.values_per_device.front().second
          << " @ " << add_IEC_binary_multiplier(read_bytes_summary.values_per_device.front().first, "B")
          << "\n" << line_prefix
          << " Biggest read load on disk   "
          << read_bytes_summary.values_per_device.back().second
          << " @ " << add_IEC_binary_multiplier(read_bytes_summary.values_per_device.back().first, "B")
          << "\n" << line_prefix
          << " Smallest write load on disk "
          << write_bytes_summary.values_per_device.front().second
          << " @ " << add_IEC_binary_multiplier(write_bytes_summary.values_per_device.front().first, "B")
          << "\n" << line_prefix
          << " Biggest write load on disk  "
          << write_bytes_summary.values_per_device.back().second
          << " @ " << add_IEC_binary_multiplier(write_bytes_summary.values_per_device.back().first, "B")
          << "\n" << line_prefix;
    }

    o << std::endl;
}

void scoped_print_iostats::report() const
{
    const auto result = foxxll::stats_data(*foxxll::stats::get_instance()) - m_begin;

    std::ostringstream ss;

    ss << (m_message.empty() ? "" : "Finished ") << m_message << ". ";

    if (m_bytes) {
        const auto bps = static_cast<double>(m_bytes) / result.get_elapsed_time();
        ss << "Processed " << tlx::format_iec_units(m_bytes) << "B"
           << " @ " << tlx::format_iec_units((uint64_t)bps) << "B/s. ";
    }

    result.to_ostream(ss, m_key);

    STXXL_MSG(ss.str());
}

} // namespace foxxll
// vim: et:ts=4:sw=4
