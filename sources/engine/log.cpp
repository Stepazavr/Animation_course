
#include "engine/log_history.h"
#include "engine/api.h"
#include <stdarg.h>

std::mutex logMutex;
std::deque<LogItem> logHistory;

void debug_common(const char *fmt, LogType message_type, va_list args)
{
  constexpr int messageLen = 1024, timeLen = 20;
  char messageBuf[messageLen], timeBuf[timeLen];
  vsnprintf(messageBuf, messageLen, fmt, args);
  snprintf(timeBuf, timeLen, "[%.2f] ", engine::get_time());

  std::string logConcat = std::string(timeBuf) + std::string(messageBuf);

  if (message_type == LogType::Error)
  {
    fprintf(stdout, "\033[31m%s%s\033[39m\n", logConcat.c_str(), messageBuf);
  }
  else
  {
    fprintf(stdout, "%s%s\n", logConcat.c_str(), messageBuf);
  }

  {
    std::unique_lock read_write_lock(logMutex);
    if (logHistory.size() >= engine::MAX_LOG_HISTORY)
    {
      logHistory.pop_front();
    }
    logHistory.push_back({std::move(logConcat), message_type});
  }
}

void engine::error(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  debug_common(fmt, LogType::Error, args);
  va_end(args);
}

void engine::log(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  debug_common(fmt, LogType::Log, args);
  va_end(args);
}
