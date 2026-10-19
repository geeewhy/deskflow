/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include "base/Log.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>

// Opt-in, process-local work timing. This does not measure network transit or
// when the compositor actually presents an injected event. No pointer/key data
// is recorded. One summary per active stage per five seconds avoids per-event IO.
class InputTiming
{
public:
  using Clock = std::chrono::steady_clock;
  explicit InputTiming(const char *stage) : m_stage(stage)
  {
    const char *value = std::getenv("DESKFLOW_INPUT_TIMING");
    m_enabled = value && std::strcmp(value, "1") == 0;
  }

  class Scope
  {
  public:
    explicit Scope(InputTiming &owner, uint32_t queuedBytes = 0)
        : m_owner(owner), m_queuedBytes(queuedBytes),
          m_start(owner.m_enabled ? Clock::now() : Clock::time_point{}) {}
    ~Scope()
    {
      if (m_owner.m_enabled)
        m_owner.record(m_start, m_queuedBytes);
    }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
  private:
    InputTiming &m_owner;
    uint32_t m_queuedBytes;
    Clock::time_point m_start;
  };

private:
  void record(Clock::time_point start, uint32_t queuedBytes)
  {
    const auto now = Clock::now();
    const auto us = std::chrono::duration<double, std::micro>(now - start).count();
    if (m_previousStart != Clock::time_point{}) {
      const auto gapMs = std::chrono::duration<double, std::milli>(start - m_previousStart).count();
      m_maxGapMs = (std::max)(m_maxGapMs, gapMs);
    }
    m_previousStart = start;
    ++m_count;
    m_totalUs += us;
    m_maxUs = (std::max)(m_maxUs, us);
    m_maxQueue = (std::max)(m_maxQueue, queuedBytes);
    if (now - m_reported < std::chrono::seconds(5))
      return;
    LOG_INFO("input timing stage=%s samples=%llu mean_work_us=%.1f max_work_us=%.1f max_queue_bytes=%u max_start_gap_ms=%.1f",
             m_stage, static_cast<unsigned long long>(m_count), m_totalUs / m_count, m_maxUs, m_maxQueue, m_maxGapMs);
    m_count = 0;
    m_totalUs = m_maxUs = 0;
    m_maxQueue = 0;
    m_maxGapMs = 0;
    m_reported = now;
  }
  const char *m_stage;
  bool m_enabled = false;
  uint64_t m_count = 0;
  double m_totalUs = 0, m_maxUs = 0;
  double m_maxGapMs = 0;
  uint32_t m_maxQueue = 0;
  Clock::time_point m_reported = Clock::now();
  Clock::time_point m_previousStart{};
};
