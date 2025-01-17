/*
 * CppSQLite
 * Developed by Rob Groves <rob.groves@btinternet.com>
 * Maintained by NeoSmart Technologies <http://neosmart.net/>
 * See LICENSE file for copyright and license info
*/

#include "CppSQLite3.h"
#include <fmt/core.h>
#include <cstdlib>
#include <utility>
#include <string>

namespace {

////////////////////////////////////////////////////////////////////////////////

void defaultErrorHandler(int nErrorCode, const std::string& errorMessage, const std::string& /* context*/) {
    std::string msg = fmt::format("{:s}[{:d}]: {:s}", CppSQLite3Exception::errorCodeAsString(nErrorCode),
                                  nErrorCode, errorMessage);

    throw CppSQLite3Exception(nErrorCode, msg);
}


}

////////////////////////////////////////////////////////////////////////////////

CppSQLite3Exception::CppSQLite3Exception(const int nErrCode,
                                    const std::string& errorMessage) :
    std::runtime_error(errorMessage), mnErrCode(nErrCode)
{

}


std::string_view CppSQLite3Exception::errorCodeAsString(int nErrCode)
{
    switch (nErrCode)
    {
        case SQLITE_OK          : return "SQLITE_OK";
        case SQLITE_ERROR       : return "SQLITE_ERROR";
        case SQLITE_INTERNAL    : return "SQLITE_INTERNAL";
        case SQLITE_PERM        : return "SQLITE_PERM";
        case SQLITE_ABORT       : return "SQLITE_ABORT";
        case SQLITE_BUSY        : return "SQLITE_BUSY";
        case SQLITE_LOCKED      : return "SQLITE_LOCKED";
        case SQLITE_NOMEM       : return "SQLITE_NOMEM";
        case SQLITE_READONLY    : return "SQLITE_READONLY";
        case SQLITE_INTERRUPT   : return "SQLITE_INTERRUPT";
        case SQLITE_IOERR       : return "SQLITE_IOERR";
        case SQLITE_CORRUPT     : return "SQLITE_CORRUPT";
        case SQLITE_NOTFOUND    : return "SQLITE_NOTFOUND";
        case SQLITE_FULL        : return "SQLITE_FULL";
        case SQLITE_CANTOPEN    : return "SQLITE_CANTOPEN";
        case SQLITE_PROTOCOL    : return "SQLITE_PROTOCOL";
        case SQLITE_EMPTY       : return "SQLITE_EMPTY";
        case SQLITE_SCHEMA      : return "SQLITE_SCHEMA";
        case SQLITE_TOOBIG      : return "SQLITE_TOOBIG";
        case SQLITE_CONSTRAINT  : return "SQLITE_CONSTRAINT";
        case SQLITE_MISMATCH    : return "SQLITE_MISMATCH";
        case SQLITE_MISUSE      : return "SQLITE_MISUSE";
        case SQLITE_NOLFS       : return "SQLITE_NOLFS";
        case SQLITE_AUTH        : return "SQLITE_AUTH";
        case SQLITE_FORMAT      : return "SQLITE_FORMAT";
        case SQLITE_RANGE       : return "SQLITE_RANGE";
        case SQLITE_ROW         : return "SQLITE_ROW";
        case SQLITE_DONE        : return "SQLITE_DONE";
        case CPPSQLITE_ERROR    : return "CPPSQLITE_ERROR";
        default: return "UNKNOWN_ERROR";
    }
}

////////////////////////////////////////////////////////////////////////////////

CppSQLite3Query::CppSQLite3Query()
{
    mpVM = 0;
    mbEof = true;
    mnCols = 0;
    mbOwnVM = false;
    mfErrorHandler = defaultErrorHandler;
}


CppSQLite3Query::CppSQLite3Query(CppSQLite3Query &&rQuery)
{
    mpVM = rQuery.mpVM;
    // Only one object can own the VM
    rQuery.mpVM = 0;
    mbEof = rQuery.mbEof;
    mnCols = rQuery.mnCols;
    mbOwnVM = rQuery.mbOwnVM;
    mfErrorHandler = rQuery.mfErrorHandler;
}


CppSQLite3Query::CppSQLite3Query(sqlite3* pDB,
                            sqlite3_stmt* pVM, CppSQLite3ErrorHandler handler,
                            bool bEof,
                            bool bOwnVM/*=true*/)
{
    mpDB = pDB;
    mpVM = pVM;
    mbEof = bEof;
    mnCols = sqlite3_column_count(mpVM);
    mbOwnVM = bOwnVM;
    mfErrorHandler = handler;
}


CppSQLite3Query::~CppSQLite3Query()
{
    try
    {
        finalize();
    }
    catch (...)
    {
    }
}


CppSQLite3Query& CppSQLite3Query::operator=(CppSQLite3Query&& rQuery)
{
    try
    {
        finalize();
    }
    catch (...)
    {
    }
    mpVM = rQuery.mpVM;
    rQuery.mpVM = 0;
    mbEof = rQuery.mbEof;
    mnCols = rQuery.mnCols;
    mbOwnVM = rQuery.mbOwnVM;
    mfErrorHandler = rQuery.mfErrorHandler;
    return *this;
}


int CppSQLite3Query::numFields() const
{
    checkVM();
    return mnCols;
}


const char* CppSQLite3Query::fieldValue(int nField) const
{
    checkVM();

    if (nField < 0 || nField > mnCols-1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return (const char*)sqlite3_column_text(mpVM, nField);
}


const char* CppSQLite3Query::fieldValue(const char* szField) const
{
    int nField = fieldIndex(szField);
    return (const char*)sqlite3_column_text(mpVM, nField);
}


int CppSQLite3Query::getIntField(int nField, int nNullValue/*=0*/) const
{
    if (fieldDataType(nField) == SQLITE_NULL)
    {
        return nNullValue;
    }
    else
    {
        return sqlite3_column_int(mpVM, nField);
    }
}


int CppSQLite3Query::getIntField(const char* szField, int nNullValue/*=0*/) const
{
    int nField = fieldIndex(szField);
    return getIntField(nField, nNullValue);
}


long long CppSQLite3Query::getInt64Field(int nField, long long nNullValue/*=0*/) const
{
    if (fieldDataType(nField) == SQLITE_NULL)
    {
        return nNullValue;
    }
    else
    {
        return sqlite3_column_int64(mpVM, nField);
    }
}


long long CppSQLite3Query::getInt64Field(const char* szField, long long nNullValue/*=0*/) const
{
    int nField = fieldIndex(szField);
    return getInt64Field(nField, nNullValue);
}


double CppSQLite3Query::getFloatField(int nField, double fNullValue/*=0.0*/) const
{
    if (fieldDataType(nField) == SQLITE_NULL)
    {
        return fNullValue;
    }
    else
    {
        return sqlite3_column_double(mpVM, nField);
    }
}


double CppSQLite3Query::getFloatField(const char* szField, double fNullValue/*=0.0*/) const
{
    int nField = fieldIndex(szField);
    return getFloatField(nField, fNullValue);
}


const char* CppSQLite3Query::getStringField(int nField, const char* szNullValue/*=""*/) const
{
    if (fieldDataType(nField) == SQLITE_NULL)
    {
        return szNullValue;
    }
    else
    {
        return (const char*)sqlite3_column_text(mpVM, nField);
    }
}


const char* CppSQLite3Query::getStringField(const char* szField, const char* szNullValue/*=""*/) const
{
    int nField = fieldIndex(szField);
    return getStringField(nField, szNullValue);
}


const unsigned char* CppSQLite3Query::getBlobField(int nField, int& nLen) const
{
    checkVM();

    if (nField < 0 || nField > mnCols-1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    nLen = sqlite3_column_bytes(mpVM, nField);
    return (const unsigned char*)sqlite3_column_blob(mpVM, nField);
}


const unsigned char* CppSQLite3Query::getBlobField(const char* szField, int& nLen) const
{
    int nField = fieldIndex(szField);
    return getBlobField(nField, nLen);
}


bool CppSQLite3Query::fieldIsNull(int nField) const
{
    return (fieldDataType(nField) == SQLITE_NULL);
}


bool CppSQLite3Query::fieldIsNull(const char* szField) const
{
    int nField = fieldIndex(szField);
    return (fieldDataType(nField) == SQLITE_NULL);
}


int CppSQLite3Query::fieldIndex(const char* szField) const
{
    checkVM();

    if (szField)
    {
        for (int nField = 0; nField < mnCols; nField++)
        {
            const char* szTemp = sqlite3_column_name(mpVM, nField);

            if (strcmp(szField, szTemp) == 0)
            {
                return nField;
            }
        }
    }

    throw std::invalid_argument("Invalid field name requested");
}


const char* CppSQLite3Query::fieldName(int nCol) const
{
    checkVM();

    if (nCol < 0 || nCol > mnCols-1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return sqlite3_column_name(mpVM, nCol);
}


const char* CppSQLite3Query::fieldDeclType(int nCol) const
{
    checkVM();

    if (nCol < 0 || nCol > mnCols-1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return sqlite3_column_decltype(mpVM, nCol);
}


int CppSQLite3Query::fieldDataType(int nCol) const
{
    checkVM();

    if (nCol < 0 || nCol > mnCols-1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return sqlite3_column_type(mpVM, nCol);
}


bool CppSQLite3Query::eof() const
{
    checkVM();
    return mbEof;
}


void CppSQLite3Query::nextRow()
{
    checkVM();

    int nRet = sqlite3_step(mpVM);

    if (nRet == SQLITE_DONE)
    {
        // no rows
        mbEof = true;
    }
    else if (nRet == SQLITE_ROW)
    {
        // more rows, nothing to do
    }
    else
    {
        if(mbOwnVM) {
            nRet = sqlite3_finalize(mpVM);
        }
        else {
            // due to goofy interface of sqlite3_step
            // use sqlite3_prepare_v2 or sqlite3_prepare_v3() to avoid it
            nRet = sqlite3_reset(mpVM);
        }
        mpVM = 0;
        const char* szError = sqlite3_errmsg(mpDB);
        mfErrorHandler(nRet, szError, "when getting next row");
    }
}


void CppSQLite3Query::finalize()
{
    if (mpVM && mbOwnVM)
    {
        int nRet = sqlite3_finalize(mpVM);
        mpVM = 0;
        if (nRet != SQLITE_OK) {
            const char* szError = sqlite3_errmsg(mpDB);
            mfErrorHandler(nRet, szError, "during finalize");
        }
    }
}


void CppSQLite3Query::checkVM() const
{
    if (mpVM == 0)
    {
        throw std::logic_error( "Null Virtual Machine pointer" );
    }
}


////////////////////////////////////////////////////////////////////////////////

CppSQLite3Table::CppSQLite3Table()
{
    mpaszResults = 0;
    mnRows = 0;
    mnCols = 0;
    mnCurrentRow = 0;
}


CppSQLite3Table::CppSQLite3Table(CppSQLite3Table&& rTable)
{
    mpaszResults = rTable.mpaszResults;
    // Only one object can own the results
    rTable.mpaszResults = 0;
    mnRows = rTable.mnRows;
    mnCols = rTable.mnCols;
    mnCurrentRow = rTable.mnCurrentRow;
}


CppSQLite3Table::CppSQLite3Table(char** paszResults, int nRows, int nCols)
{
    mpaszResults = paszResults;
    mnRows = nRows;
    mnCols = nCols;
    mnCurrentRow = 0;
}


CppSQLite3Table::~CppSQLite3Table()
{
    try
    {
        finalize();
    }
    catch (...)
    {
    }
}


CppSQLite3Table& CppSQLite3Table::operator=(CppSQLite3Table&& rTable)
{
    try
    {
        finalize();
    }
    catch (...)
    {
    }
    mpaszResults = rTable.mpaszResults;
    // Only one object can own the results
    rTable.mpaszResults = 0;
    mnRows = rTable.mnRows;
    mnCols = rTable.mnCols;
    mnCurrentRow = rTable.mnCurrentRow;
    return *this;
}


void CppSQLite3Table::finalize()
{
    if (mpaszResults)
    {
        sqlite3_free_table(mpaszResults);
        mpaszResults = 0;
    }
}


int CppSQLite3Table::numFields() const
{
    checkResults();
    return mnCols;
}


int CppSQLite3Table::numRows() const
{
    checkResults();
    return mnRows;
}


const char* CppSQLite3Table::fieldValue(int nField) const
{
    checkResults();

    if (nField < 0 || nField > mnCols-1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    int nIndex = (mnCurrentRow*mnCols) + mnCols + nField;
    return mpaszResults[nIndex];
}


const char* CppSQLite3Table::fieldValue(const char* szField) const
{
    checkResults();

    if (szField)
    {
        for (int nField = 0; nField < mnCols; nField++)
        {
            if (strcmp(szField, mpaszResults[nField]) == 0)
            {
                int nIndex = (mnCurrentRow*mnCols) + mnCols + nField;
                return mpaszResults[nIndex];
            }
        }
    }
    auto msg = fmt::format("Invalid field name requested: '{:s}'", szField );
    throw std::invalid_argument(msg);
}


int CppSQLite3Table::getIntField(int nField, int nNullValue/*=0*/) const
{
    if (fieldIsNull(nField))
    {
        return nNullValue;
    }
    else
    {
        return atoi(fieldValue(nField));
    }
}


int CppSQLite3Table::getIntField(const char* szField, int nNullValue/*=0*/) const
{
    if (fieldIsNull(szField))
    {
        return nNullValue;
    }
    else
    {
        return atoi(fieldValue(szField));
    }
}


double CppSQLite3Table::getFloatField(int nField, double fNullValue/*=0.0*/) const
{
    if (fieldIsNull(nField))
    {
        return fNullValue;
    }
    else
    {
        return atof(fieldValue(nField));
    }
}


double CppSQLite3Table::getFloatField(const char* szField, double fNullValue/*=0.0*/) const
{
    if (fieldIsNull(szField))
    {
        return fNullValue;
    }
    else
    {
        return atof(fieldValue(szField));
    }
}


const char* CppSQLite3Table::getStringField(int nField, const char* szNullValue/*=""*/) const
{
    if (fieldIsNull(nField))
    {
        return szNullValue;
    }
    else
    {
        return fieldValue(nField);
    }
}


const char* CppSQLite3Table::getStringField(const char* szField, const char* szNullValue/*=""*/) const
{
    if (fieldIsNull(szField))
    {
        return szNullValue;
    }
    else
    {
        return fieldValue(szField);
    }
}


bool CppSQLite3Table::fieldIsNull(int nField) const
{
    checkResults();
    return (fieldValue(nField) == 0);
}


bool CppSQLite3Table::fieldIsNull(const char* szField) const
{
    checkResults();
    return (fieldValue(szField) == 0);
}


const char* CppSQLite3Table::fieldName(int nCol) const
{
    checkResults();

    if (nCol < 0 || nCol > mnCols-1)
    {
        throw std::invalid_argument("Invalid field index requested");
    }

    return mpaszResults[nCol];
}


void CppSQLite3Table::setRow(int nRow)
{
    checkResults();

    if (nRow < 0 || nRow > mnRows-1)
    {
        throw std::invalid_argument("Invalid row index requested");
    }

    mnCurrentRow = nRow;
}


void CppSQLite3Table::checkResults() const
{
    if (mpaszResults == 0)
    {
        throw std::logic_error("Null Results pointer");
    }
}


////////////////////////////////////////////////////////////////////////////////

CppSQLite3Statement::CppSQLite3Statement()
{
    mpDB = 0;
    mpVM = 0;
    mfErrorHandler = defaultErrorHandler;
}


CppSQLite3Statement::CppSQLite3Statement(CppSQLite3Statement &&rStatement)
{
    mpDB = rStatement.mpDB;
    mpVM = rStatement.mpVM;
    // Only one object can own VM
    rStatement.mpVM = 0;
    mfErrorHandler = rStatement.mfErrorHandler;
}


CppSQLite3Statement::CppSQLite3Statement(sqlite3* pDB, sqlite3_stmt* pVM, CppSQLite3ErrorHandler handler)
{
    mpDB = pDB;
    mpVM = pVM;
    mfErrorHandler = handler;
}


CppSQLite3Statement::~CppSQLite3Statement()
{
    try
    {
        finalize();
    }
    catch (...)
    {
    }
}


CppSQLite3Statement& CppSQLite3Statement::operator=(CppSQLite3Statement &&rStatement)
{
    mpDB = rStatement.mpDB;
    mpVM = rStatement.mpVM;
    // Only one object can own VM
    rStatement.mpVM = 0;
    mfErrorHandler = rStatement.mfErrorHandler;
    return *this;
}


int CppSQLite3Statement::execDML()
{
    checkDB();
    checkVM();

    const char* szError=0;

    int nRet = sqlite3_step(mpVM);

    if (nRet == SQLITE_DONE)
    {
        int nRowsChanged = sqlite3_changes(mpDB);

        nRet = sqlite3_reset(mpVM);

        if (nRet != SQLITE_OK)
        {
            szError = sqlite3_errmsg(mpDB);
            mfErrorHandler(nRet, szError, "when getting number of rows changed");
        }

        return nRowsChanged;
    }
    else
    {
        nRet = sqlite3_reset(mpVM);
        szError = sqlite3_errmsg(mpDB);
        mfErrorHandler(nRet, szError, "when executing DML statement");
        return 0;
    }
}


CppSQLite3Query CppSQLite3Statement::execQuery()
{
    checkDB();
    checkVM();

    int nRet = sqlite3_step(mpVM);

    if (nRet == SQLITE_DONE)
    {
        // no rows
        return CppSQLite3Query(mpDB, mpVM, mfErrorHandler, true/*eof*/, false);
    }
    else if (nRet == SQLITE_ROW)
    {
        // at least 1 row
        return CppSQLite3Query(mpDB, mpVM, mfErrorHandler, false/*eof*/, false);
    }
    else
    {
        nRet = sqlite3_reset(mpVM);
        const char* szError = sqlite3_errmsg(mpDB);
        mfErrorHandler(nRet, szError, "when evaluating query");
        return CppSQLite3Query();
    }
}


void CppSQLite3Statement::bind(int nParam, const char* szValue)
{
    checkVM();
    int nRes = sqlite3_bind_text(mpVM, nParam, szValue, -1, SQLITE_TRANSIENT);
    checkReturnCode(nRes, "when binding string param");
}


void CppSQLite3Statement::bind(int nParam, const int nValue)
{
    checkVM();
    int nRes = sqlite3_bind_int(mpVM, nParam, nValue);
    checkReturnCode(nRes,"when binding int param");
}


void CppSQLite3Statement::bind(int nParam, const long long nValue)
{
    checkVM();
    int nRes = sqlite3_bind_int64(mpVM, nParam, nValue);
    checkReturnCode(nRes, "when binding int64 param");
}


void CppSQLite3Statement::bind(int nParam, const double dValue)
{
    checkVM();
    int nRes = sqlite3_bind_double(mpVM, nParam, dValue);
    checkReturnCode(nRes, "when binding double param");
}


void CppSQLite3Statement::bind(int nParam, const unsigned char* blobValue, int nLen)
{
    checkVM();
    int nRes = sqlite3_bind_blob(mpVM, nParam,
                                (const void*)blobValue, nLen, SQLITE_TRANSIENT);

    checkReturnCode(nRes, "when binding blob param");
}


void CppSQLite3Statement::bindNull(int nParam)
{
    checkVM();
    int nRes = sqlite3_bind_null(mpVM, nParam);
    checkReturnCode(nRes, "when binding NULL param");
}


void CppSQLite3Statement::reset()
{
    if (mpVM)
    {
        int nRet = sqlite3_reset(mpVM);
        checkReturnCode(nRet, "when reseting statement");
    }
}


void CppSQLite3Statement::finalize()
{
    if (mpVM)
    {
        int nRet = sqlite3_finalize(mpVM);
        mpVM = 0;
        checkReturnCode(nRet, "when finalizing statement");
    }
}


void CppSQLite3Statement::checkDB() const
{
    if (mpDB == 0)
    {
        throw std::logic_error("Database not open");
    }
}


void CppSQLite3Statement::checkVM() const
{
    if (mpVM == 0)
    {
        throw std::logic_error("Null Virtual Machine pointer");
    }
}

void CppSQLite3Statement::checkReturnCode(int nRes, const char* context)
{
    if (nRes != SQLITE_OK)
    {
        const char* szError = sqlite3_errmsg(mpDB);
        mfErrorHandler(nRes, szError, context);
    }
}


////////////////////////////////////////////////////////////////////////////////

CppSQLite3DB::CppSQLite3DB()
{
    mpDB = nullptr;
    mnBusyTimeoutMs = 60000; // 60 seconds
    mfErrorHandler = defaultErrorHandler;
}


CppSQLite3DB::CppSQLite3DB(const CppSQLite3DB& db)
{
    mpDB = db.mpDB;
    mnBusyTimeoutMs = 60000; // 60 seconds
}


CppSQLite3DB::~CppSQLite3DB()
{
    close();
}


CppSQLite3DB& CppSQLite3DB::operator=(const CppSQLite3DB& db)
{
    mpDB = db.mpDB;
    mnBusyTimeoutMs = 60000; // 60 seconds
    return *this;
}


void CppSQLite3DB::open(const char* szFile, int flags)
{
    int nRet = sqlite3_open_v2(szFile, &mpDB, flags, nullptr);

    if (nRet != SQLITE_OK)
    {
        const char* szError = sqlite3_errmsg(mpDB);
        auto msg = fmt::format("when opening {:s}", szFile);
        mfErrorHandler(nRet, szError, msg.c_str());
    }

    setBusyTimeout(mnBusyTimeoutMs);
}


void CppSQLite3DB::close()
{
    if (mpDB)
    {
        sqlite3_close(mpDB);
        mpDB = nullptr;
    }
}

bool CppSQLite3DB::isOpened() const
{
    return mpDB != nullptr;
}


CppSQLite3Statement CppSQLite3DB::compileStatement(const char* szSQL)
{
    checkDB();

    sqlite3_stmt* pVM = compile(szSQL);
    return CppSQLite3Statement(mpDB, pVM, mfErrorHandler);
}


bool CppSQLite3DB::tableExists(const char* szTable)
{
    std::string buffer(256, '\0');
    // use sqlite3_snprintf function as it properly escapes the input string
    sqlite3_snprintf(static_cast<int>(buffer.size()), buffer.data(), "select count(*) from sqlite_master where type='table' and name=%Q", szTable );
    int nRet = execScalar(buffer.c_str());
    return (nRet > 0);
}

int CppSQLite3DB::execDML(const char* szSQL)
{
    checkDB();

    char* szError=0;

    int nRet = sqlite3_exec(mpDB, szSQL, 0, 0, &szError);

    if (nRet == SQLITE_OK)
    {
        return sqlite3_changes(mpDB);
    }
    else
    {
        std::string error = "Unknown error";
        if(szError != nullptr) {
            error = szError;
            sqlite3_free((void*)szError);
        }
        else {
            error = sqlite3_errmsg(mpDB);
        }
        mfErrorHandler(nRet, error.c_str(), "when executing DML query");
        return nRet;
    }
}


CppSQLite3Query CppSQLite3DB::execQuery(const char* szSQL)
{
    checkDB();

    sqlite3_stmt* pVM = compile(szSQL);

    int nRet = sqlite3_step(pVM);

    if (nRet == SQLITE_DONE)
    {
        // no rows
        return CppSQLite3Query(mpDB, pVM, mfErrorHandler, true/*eof*/);
    }
    else if (nRet == SQLITE_ROW)
    {
        // at least 1 row
        return CppSQLite3Query(mpDB, pVM, mfErrorHandler, false/*eof*/);
    }
    else
    {
        nRet = sqlite3_finalize(pVM);
        const char* szError= sqlite3_errmsg(mpDB);
        mfErrorHandler(nRet, szError,  "when evaluating query");
        return CppSQLite3Query();
    }
}


int CppSQLite3DB::execScalar(const char* szSQL)
{
    CppSQLite3Query q = execQuery(szSQL);

    if (q.eof() || q.numFields() < 1)
    {
        throw std::invalid_argument("Invalid scalar query");
    }

    return atoi(q.fieldValue(0));
}


CppSQLite3Table CppSQLite3DB::getTable(const char* szSQL)
{
    checkDB();

    char* szError=0;
    char** paszResults=0;
    int nRet;
    int nRows(0);
    int nCols(0);

    nRet = sqlite3_get_table(mpDB, szSQL, &paszResults, &nRows, &nCols, &szError);

    if (nRet == SQLITE_OK)
    {
        return CppSQLite3Table(paszResults, nRows, nCols);
    }
    else
    {
        std::string error = szError;
        sqlite3_free(szError);
        mfErrorHandler(nRet, error.c_str(), "when getting table");
        return CppSQLite3Table();
    }
}


sqlite_int64 CppSQLite3DB::lastRowId() const
{
    return sqlite3_last_insert_rowid(mpDB);
}


void CppSQLite3DB::setBusyTimeout(int nMillisecs)
{
    mnBusyTimeoutMs = nMillisecs;
    sqlite3_busy_timeout(mpDB, mnBusyTimeoutMs);
}


void CppSQLite3DB::checkDB() const
{
    if (!mpDB)
    {
        throw std::logic_error("Database not open");
    }
}


sqlite3_stmt* CppSQLite3DB::compile(const char* szSQL)
{
    checkDB();

    const char* szTail=0;
    sqlite3_stmt* pVM;

    int nRet = sqlite3_prepare(mpDB, szSQL, -1, &pVM, &szTail);
    const char* szError = sqlite3_errmsg(mpDB);

    if (nRet != SQLITE_OK)
    {
        mfErrorHandler(nRet, szError, "when compiling statement");
    }

    return pVM;
}
