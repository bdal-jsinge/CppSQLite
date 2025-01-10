/*
 * CppSQLite
 * Developed by Rob Groves <rob.groves@btinternet.com>
 * Maintained by NeoSmart Technologies <http://neosmart.net/>
 * See LICENSE file for copyright and license info
 */

#ifndef CppSQLite3_H
#define CppSQLite3_H

#include <cstdio>
#include <cstring>
#include <sqlite3.h>

#include <stdexcept>

#define CPPSQLITE_ERROR 1000

struct CppSQLite3LogLevel
{

    enum Level
    {
        VERBOSE,
        INFO,
        WARNING,
        ERROR
    } code;
    std::string_view name;

    explicit CppSQLite3LogLevel(Level level);
};

using CppSQLite3ErrorHandler = void (*)(int /*sqlite3_error_code*/, const std::string& /* message */,
                                        const std::string& /* context */);
using CppSQLite3LogHandler = void (*)(CppSQLite3LogLevel /*level*/, const std::string& /*message */);


class CppSQLite3Exception : public std::runtime_error
{
public:
    CppSQLite3Exception(const int nErrCode, const std::string& errorMessage);

    const int errorCode() const
    {
        return mnErrCode;
    }

    static std::string_view errorCodeAsString(int nErrCode);

private:
    int mnErrCode;
};

struct CppSQLite3Config
{
    CppSQLite3Config();
    sqlite3* db;
    CppSQLite3ErrorHandler errorHandler;
    CppSQLite3LogHandler logHandler;
    bool enableVerboseLogging = false;
    void log(CppSQLite3LogLevel::Level level, const char* message);
    void log(CppSQLite3LogLevel::Level level, const std::string& message);
};

class CppSQLite3Query
{
public:
    CppSQLite3Query();

    CppSQLite3Query(CppSQLite3Query&& rQuery);

    CppSQLite3Query(const CppSQLite3Config& config, sqlite3_stmt* pVM, bool bEof, bool bOwnVM = true);

    CppSQLite3Query& operator=(CppSQLite3Query&& rQuery);

    virtual ~CppSQLite3Query();

    int numFields() const;

    int fieldIndex(const char* szField) const;
    const char* fieldName(int nCol) const;

    const char* fieldDeclType(int nCol) const;
    int fieldDataType(int nCol) const;

    const char* fieldValue(int nField) const;
    const char* fieldValue(const char* szField) const;

    int getIntField(int nField, int nNullValue = 0) const;
    int getIntField(const char* szField, int nNullValue = 0) const;

    long long getInt64Field(int nField, long long nNullValue = 0) const;
    long long getInt64Field(const char* szField, long long nNullValue = 0) const;

    double getFloatField(int nField, double fNullValue = 0.0) const;
    double getFloatField(const char* szField, double fNullValue = 0.0) const;

    const char* getStringField(int nField, const char* szNullValue = "") const;
    const char* getStringField(const char* szField, const char* szNullValue = "") const;

    const unsigned char* getBlobField(int nField, int& nLen) const;
    const unsigned char* getBlobField(const char* szField, int& nLen) const;

    bool fieldIsNull(int nField) const;
    bool fieldIsNull(const char* szField) const;

    bool eof() const;

    void nextRow();

    void finalize();

private:
    void checkVM() const;

    CppSQLite3Config mConfig;
    sqlite3_stmt* mpVM;
    bool mbEof;
    int mnCols;
    bool mbOwnVM;
};

class CppSQLite3Statement
{
public:
    CppSQLite3Statement();

    CppSQLite3Statement(CppSQLite3Statement&& rStatement);

    CppSQLite3Statement(const CppSQLite3Config& config, sqlite3_stmt* pVM);

    virtual ~CppSQLite3Statement();

    CppSQLite3Statement& operator=(CppSQLite3Statement&& rStatement);

    int execDML();

    CppSQLite3Query execQuery();

    void bind(int nParam, const char* szValue);
    void bind(int nParam, const int nValue);
    void bind(int nParam, const long long nValue);
    void bind(int nParam, const double dwValue);
    void bind(int nParam, const unsigned char* blobValue, int nLen);
    void bindNull(int nParam);

    void reset();

    void finalize();

private:
    void checkDB() const;
    void checkVM() const;
    void checkReturnCode(int returnCode, const char* context);

    CppSQLite3Config mConfig;
    sqlite3_stmt* mpVM;
};


class CppSQLite3DB
{
public:
    CppSQLite3DB();

    CppSQLite3DB(const CppSQLite3DB& db) = delete;
    CppSQLite3DB& operator=(const CppSQLite3DB& db) = delete;

    virtual ~CppSQLite3DB();

    /**
     * @brief open opens a database with the given filename
     * @param szFile the filename of the database
     * @param flags the SQLITE_OPEN_* flags that are passed on to the sqlite3_open_v2 call
     */
    void open(const char* szFile, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    void close();

    /**
     * @brief enableVerboseLogging enables logging of all SQL statements & queries
     */
    void enableVerboseLogging(bool enable);

    bool isOpened() const;

    bool tableExists(const char* szTable);

    int execDML(const char* szSQL);

    CppSQLite3Query execQuery(const char* szSQL);

    int execScalar(const char* szSQL);

    CppSQLite3Statement compileStatement(const char* szSQL);

    sqlite_int64 lastRowId() const;

    void interrupt()
    {
        sqlite3_interrupt(mConfig.db);
    }

    void setBusyTimeout(int nMillisecs);

    void setErrorHandler(CppSQLite3ErrorHandler h);

    void setLogHandler(CppSQLite3LogHandler h);

    static const char* SQLiteVersion()
    {
        return SQLITE_VERSION;
    }

    /**
     * @brief performCheckpoint wraps sqlite3_wal_checkpoint_v2
     * @param dbName name of the attached database (or empty)
     * @param mode SQLITE_CHECKPOINT_* value
     */
    void performCheckpoint(const std::string& dbName = "", int mode = SQLITE_CHECKPOINT_PASSIVE);

private:
    sqlite3_stmt* compile(const char* szSQL);

    void checkDB() const;
    CppSQLite3Config mConfig;
    int mnBusyTimeoutMs;
};

#endif
