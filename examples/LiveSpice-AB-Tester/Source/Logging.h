#pragma once

#include <juce_core/juce_core.h>

// ============================================================================
// Logging System with Levels
// ============================================================================

enum class LogLevel
{
    NONE,       // No logging
    ERROR,      // Only errors
    WARNING,    // Errors and warnings
    INFO,       // Errors, warnings, and info
    DEBUG       // All messages
};

// Global logging configuration
extern LogLevel g_currentLogLevel;
extern bool g_loggingEnabled;
extern juce::File g_logFile;

inline void logDebug(const juce::String& msg, LogLevel level = LogLevel::DEBUG)
{
    // Only log if enabled and level is high enough
    if (!g_loggingEnabled || level > g_currentLogLevel)
        return;
    
    DBG(msg);
    
    // Write to file if it exists
    if (g_logFile.exists() || g_logFile.create())
    {
        auto timestamp = juce::Time::getCurrentTime().formatted("%H:%M:%S.%ms");
        auto levelStr = [](LogLevel l) -> const char* {
            switch(l) {
                case LogLevel::ERROR:   return "[ERROR]  ";
                case LogLevel::WARNING: return "[WARN]   ";
                case LogLevel::INFO:    return "[INFO]   ";
                case LogLevel::DEBUG:   return "[DEBUG]  ";
                default:                return "[NONE]   ";
            }
        }(level);
        
        g_logFile.appendText(timestamp + " " + levelStr + msg + "\n");
    }
}
