#pragma once
#include <string>
#include <deque>
#include <mutex>

enum class LogType
{
  Error,
  Log,
};

struct LogItem
{
  std::string message;
  LogType LogType;
};

extern std::mutex logMutex;
extern std::deque<LogItem> logHistory;