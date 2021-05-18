#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/Log.h"

#include "../../3rdParty/Sqlite/Export/Sqlite.h"

// =================================================================================================

// -------------------------------------------------------------------------------------------------

static void SInvokeMatchOperator(
  sqlite3_context*  pCtx, 
  int               NumValues, 
  sqlite3_value**   ppValues)
{
  MAssert(NumValues == 2, "Unexpected number of args");

  TDatabase* pDatabase = reinterpret_cast<TDatabase*>(
    ::sqlite3_user_data(pCtx)); //lint !e611 (Suspicious cast)

  const char* pFirst = (char*)::sqlite3_value_text(ppValues[0]);
  const char* pSecond = (char*)::sqlite3_value_text(ppValues[1]);
  ::sqlite3_result_int(pCtx, pDatabase->InvokeMatchOperator(pFirst, pSecond));
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TString TDatabase::SSqliteVersionString()
{
  return TString(::sqlite3_libversion(), TString::kUtf8);
}

// -------------------------------------------------------------------------------------------------

int TDatabase::SSqliteVersionNumber()
{
  return ::sqlite3_libversion_number();
}

// -------------------------------------------------------------------------------------------------

void TDatabase::SHandleSqlError(const TDatabase& Database, int ErrorCode)
{
  if (ErrorCode != SQLITE_OK)
  {
    throw TReadableException(
      MText("Database Error (File: '%s'): '%s' (Code:%s)", 
      Database.FileNameAndPath(),
      TString((const TUnicodeChar*)::sqlite3_errmsg16(Database.mpSqliteDatabase)), 
      ToString(ErrorCode)));
  }
}

void TDatabase::SHandleSqlError(const TDatabase& Database, int ErrorCode, const TString& Message)
{
  if (ErrorCode != SQLITE_OK)
  {
    throw TReadableException(
      MText("Database Error (File: '%s'): '%s' (Code:%s)", 
      Database.FileNameAndPath(),
      Message, 
      ToString(ErrorCode)));
  }
}

// -------------------------------------------------------------------------------------------------

TDatabase::TDatabase(const TString& DbPath)
  : mpSqliteDatabase(NULL),
    mpMatchfunction(NULL),
    mpMatchfunctionContext(NULL)
{
  if (!DbPath.IsEmpty())
  {
    const bool ThrowErrors = true;
    Open( DbPath, ThrowErrors);
  }
}

// -------------------------------------------------------------------------------------------------

TDatabase::~TDatabase()
{
  if (mpSqliteDatabase != NULL)
  {
    int ErrorCode = ::sqlite3_close(mpSqliteDatabase);
    MAssert(ErrorCode == SQLITE_OK, "");

    if (ErrorCode != SQLITE_OK)
    {
      ::sqlite3_interrupt(mpSqliteDatabase);
      
      ErrorCode = ::sqlite3_close(mpSqliteDatabase);
      MAssert(ErrorCode == SQLITE_OK, "");
      mpSqliteDatabase = NULL;
    }
  }
}

// -------------------------------------------------------------------------------------------------

TString TDatabase::FileNameAndPath()const
{
  return mFileNameAndPath;
}

// -------------------------------------------------------------------------------------------------

bool TDatabase::Open(const TString& sDbPath)
{
  mFileNameAndPath = sDbPath;

  MAssert(mpSqliteDatabase == NULL, "");
  return (Open(sDbPath, false) == SQLITE_OK);
}

// -------------------------------------------------------------------------------------------------

bool TDatabase::IsOpen()const
{
  return (mpSqliteDatabase != NULL);
}

// -------------------------------------------------------------------------------------------------

void TDatabase::Close()
{
  if (mpSqliteDatabase != NULL)
  {
    struct sqlite3* pSqliteDatabase = mpSqliteDatabase;
    
    mpSqliteDatabase = NULL;
    mFileNameAndPath = "";

    SHandleSqlError(*this, ::sqlite3_close(pSqliteDatabase));
  }
}

// -------------------------------------------------------------------------------------------------

bool TDatabase::InvokeMatchOperator(
  const char* pFirst,
  const char* pSecond)const
{
  if (mpMatchfunction)
  {
    return mpMatchfunction(mpMatchfunctionContext, pFirst, pSecond);
  }
  else
  {
    MInvalid("No match function set");
    return false;
  }
}

// -------------------------------------------------------------------------------------------------

void TDatabase::SetMatchOperator(
  void*           pContext,
  TPMatchFunction pMatchfunction)
{
  MAssert(IsOpen(), "Must be open!");

  mpMatchfunctionContext = pContext;
  mpMatchfunction = pMatchfunction;
  
  ::sqlite3_create_function(mpSqliteDatabase, "match", 2, 
    SQLITE_UTF8, (void*)this, //lint !e611 (Suspicious cast)
    SInvokeMatchOperator, NULL, NULL);
}

// -------------------------------------------------------------------------------------------------

long long TDatabase::LastInsertedRowIndex()const
{
  MAssert(mpSqliteDatabase != NULL, "");
  return ::sqlite3_last_insert_rowid(const_cast<sqlite3*>(mpSqliteDatabase));
}

// -------------------------------------------------------------------------------------------------

int TDatabase::NumberOfChanges()const
{
  MAssert(mpSqliteDatabase != NULL, "");
  return ::sqlite3_changes(const_cast<sqlite3*>(mpSqliteDatabase));
}

// -------------------------------------------------------------------------------------------------

int TDatabase::TotalNumberOfChanges()const
{
  MAssert(mpSqliteDatabase != NULL, "");
  return ::sqlite3_total_changes(const_cast<sqlite3*>(mpSqliteDatabase));
}

// -------------------------------------------------------------------------------------------------

void TDatabase::Execute(const TString& StatementString)
{
  MAssert(mpSqliteDatabase != NULL, "");
  
  char* pErrorMessage = NULL;
  
  const int Result = ::sqlite3_exec(mpSqliteDatabase, 
    StatementString.CString(TString::kUtf8), NULL, NULL, &pErrorMessage);

  TString ErrorMessage;
  
  if (pErrorMessage != NULL)
  {
    ErrorMessage = TString(pErrorMessage, TString::kUtf8);
    ::sqlite3_free(pErrorMessage);
  }
  
  SHandleSqlError(*this, Result, ErrorMessage);
}

// -------------------------------------------------------------------------------------------------

int TDatabase::ExecuteScalarInt(const TString& StatementString)
{
  TDatabase::TStatement Statement(*this, StatementString);
  
  if (Statement.Step())
  {
    MAssert(Statement.ColumnCount() == 1, 
      "More than one column in this scalar statement!");
  
    const int Int = Statement.ColumnInt(0);
    
    Statement.Reset();
    return Int;
  }
  else
  {
    throw TReadableException(
      MText("Database Error: ExecuteScalarInt - Query didn't return any results."));
  }
}

// -------------------------------------------------------------------------------------------------

long long TDatabase::ExecuteScalarInt64(const TString& StatementString)
{
  TDatabase::TStatement Statement(*this, StatementString);
  
  if (Statement.Step())
  {
    MAssert(Statement.ColumnCount() == 1, 
      "More than one column in this scalar statement!");
  
    long long Int64 = Statement.ColumnInt64(0);
    
    Statement.Reset();
    return Int64;
  }
  else
  {
    throw TReadableException(
      MText("Database Error: ExecuteScalarInt64 - Query didn't return any results."));
  }
}

// -------------------------------------------------------------------------------------------------

double TDatabase::ExecuteScalarDouble(const TString& StatementString)
{
  TDatabase::TStatement Statement(*this, StatementString);
  
  if (Statement.Step())
  {
    MAssert(Statement.ColumnCount() == 1, 
      "More than one column in this scalar statement!");
    
    const double Double = Statement.ColumnDouble(0);
    
    Statement.Reset();
    return Double;
  }
  else
  {
    throw TReadableException(
      MText("Database Error: ExecuteScalarDouble - Query didn't return any results."));
  }
}

// -------------------------------------------------------------------------------------------------

TString TDatabase::ExecuteScalarText(const TString& StatementString)
{
  TDatabase::TStatement Statement(*this, StatementString);
  
  if (Statement.Step())
  {
    MAssert(Statement.ColumnCount() == 1, 
      "More than one column in this scalar statement!");

    const TString Text = Statement.ColumnText(0);

    Statement.Reset();
    return Text;
  }
  else
  {
    throw TReadableException(
      MText("Database Error: ExecuteScalarText - Query didn't return any results."));
  }
}

// -------------------------------------------------------------------------------------------------

int TDatabase::Open(const TString& DbPath, bool ThrowOnError)
{
  MAssert(mpSqliteDatabase == NULL, "");
  
  // open database
  const int Result = ::sqlite3_open16(
    static_cast<const void*>(DbPath.Chars()), &mpSqliteDatabase);

  if (Result != SQLITE_OK)
  {
    mpSqliteDatabase = NULL;

    if (ThrowOnError)
    {
      SHandleSqlError(*this, Result);
    }
  }
  else // Result == SQLITE_OK
  {
    // set encoding
    Execute( "PRAGMA encoding = utf8;" );

    // set WAL mode
    const TString JournalMode = ExecuteScalarText( "PRAGMA journal_mode = WAL;" );
    
    if ( JournalMode != "wal" )
    {
      TLog::SLog()->AddLine( "Database", "FAILED to set journal_mode to WAL: "
        "using journal_mode '%s' instead", JournalMode.CString());

      MInvalid( "Failed to set database journal_mode" );
    }

    // set synchronous mode
    Execute( "PRAGMA synchronous = NORMAL;" );

    // set temp store
    try
    {
      if (gTempDir().Path().Find("'") == -1) // use ' to backslash, if possible...
      {
        Execute("PRAGMA temp_store_directory=" + 
          TString("'") + gTempDir().Path() + TString("'"));
      }
      else
      {
        Execute("PRAGMA temp_store_directory=" + 
          TString("\"") + gTempDir().Path() + TString("\""));
      }
    }
    catch (const TReadableException&)
    {
      // ignore PRAGMA temp_store_directory errors
    }
  }
  
  return Result;
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

TDatabase::TTransaction::TTransaction(TDatabase& Database)
  : mDatabase(Database), 
    mCommited(false)
{
  mDatabase.Execute("BEGIN TRANSACTION");
}

// -------------------------------------------------------------------------------------------------

TDatabase::TTransaction::~TTransaction()
{
  if (!mCommited)
  {
    mDatabase.Execute("ROLLBACK TRANSACTION");
  }
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TTransaction::Commit()
{
  MAssert(!mCommited, "Already committed!");
  mDatabase.Execute("COMMIT TRANSACTION");
  
  mCommited = true;
}
    
// =================================================================================================

// -------------------------------------------------------------------------------------------------

TDatabase::TStatement::TStatement(
  TDatabase&      Database, 
  const TString&  StatementString)
  : mDatabase(Database),
    mpSqliteStatement(NULL)
{
  SHandleSqlError(mDatabase,
    ::sqlite3_prepare16(mDatabase.mpSqliteDatabase,
      static_cast<const void*>(StatementString.Chars()),
      -1,
      &mpSqliteStatement, NULL));
  
  #if defined(MDebug)
    m__BoundParameters.SetSize(ParameterCount());
    m__BoundParameters.Init(false);
  #endif
}

TDatabase::TStatement::TStatement(
  TDatabase&  Database, 
  const char* pStatementString)
  : mDatabase(Database),
    mpSqliteStatement(NULL)
{
  SHandleSqlError(mDatabase, ::sqlite3_prepare(mDatabase.mpSqliteDatabase,
      pStatementString, -1, &mpSqliteStatement, NULL));

  #if defined(MDebug)
    m__BoundParameters.SetSize(ParameterCount());
    m__BoundParameters.Init(false);
  #endif
}

// -------------------------------------------------------------------------------------------------

TDatabase::TStatement::~TStatement()
{
  if (mpSqliteStatement)
  {
    ::sqlite3_finalize(mpSqliteStatement);
    mpSqliteStatement = NULL;
  }
}

// -------------------------------------------------------------------------------------------------

int TDatabase::TStatement::ParameterCount()const
{
  MAssert(mpSqliteStatement != NULL, "");
  return ::sqlite3_bind_parameter_count(const_cast<sqlite3_stmt*>(mpSqliteStatement));
}

// -------------------------------------------------------------------------------------------------

int TDatabase::TStatement::ParameterIndex(const TString& ParameterString)
{
  return ::sqlite3_bind_parameter_index(
    mpSqliteStatement, ParameterString.CString(TString::kUtf8));
}

// -------------------------------------------------------------------------------------------------

TString TDatabase::TStatement::ParameterName(int ParameterIndex)const
{
  MAssert(mpSqliteStatement != NULL, "");
  
  return TString(::sqlite3_bind_parameter_name(
    mpSqliteStatement, ParameterIndex), TString::kUtf8);
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::BindBlob(
  int         ParameterIndex, 
  const void* pBlobData, 
  int         BlobSize)
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    MAssert(ParameterIndex != 0, "Invalid ParameterIndex");
    MAssert(ParameterIndex - 1 < m__BoundParameters.Size(), "Invalid ParameterIndex");
    m__BoundParameters[ParameterIndex - 1] = true;
  #endif

  SHandleSqlError(mDatabase, ::sqlite3_bind_blob(mpSqliteStatement, 
    ParameterIndex, pBlobData, BlobSize, SQLITE_TRANSIENT));
}

void TDatabase::TStatement::BindBlob(
  const TString&  ParameterString, 
  const void*     pBlobData, 
  int             BlobSize)
{
  BindBlob(ParameterIndex(ParameterString), pBlobData, BlobSize);
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::BindDouble(int ParameterIndex, double Value)
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    MAssert(ParameterIndex != 0, "Invalid ParameterIndex");
    MAssert(ParameterIndex - 1 < m__BoundParameters.Size(), "Invalid ParameterIndex");
    m__BoundParameters[ParameterIndex - 1] = true;
  #endif

  SHandleSqlError(mDatabase, ::sqlite3_bind_double(
    mpSqliteStatement, ParameterIndex, Value));
}

void TDatabase::TStatement::BindDouble(const TString& ParameterString, double Value)
{
  BindDouble(ParameterIndex(ParameterString), Value);
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::BindBool(int ParameterIndex, bool Value)
{
  MAssert(mpSqliteStatement != NULL, "");

  #if defined(MDebug)
    MAssert(ParameterIndex != 0, "Invalid ParameterIndex");
    MAssert(ParameterIndex - 1 < m__BoundParameters.Size(), "Invalid ParameterIndex");
    m__BoundParameters[ParameterIndex - 1] = true;
  #endif

  SHandleSqlError(mDatabase, ::sqlite3_bind_int(mpSqliteStatement, 
    ParameterIndex, (int)Value));
}

void TDatabase::TStatement::BindBool(const TString& ParameterString, bool Value)
{
  BindBool(ParameterIndex(ParameterString), Value);
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::BindInt(int ParameterIndex, int Value)
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    MAssert(ParameterIndex != 0, "Invalid ParameterIndex");
    MAssert(ParameterIndex - 1 < m__BoundParameters.Size(), "Invalid ParameterIndex");
    m__BoundParameters[ParameterIndex - 1] = true;
  #endif

  SHandleSqlError(mDatabase, ::sqlite3_bind_int(mpSqliteStatement, 
    ParameterIndex, Value));
}

void TDatabase::TStatement::BindInt(const TString& ParameterString, int Value)
{
  BindInt(ParameterIndex(ParameterString), Value);
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::BindInt64(int ParameterIndex, long long Value)
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    MAssert(ParameterIndex != 0, "Invalid ParameterIndex");
    MAssert(ParameterIndex - 1 < m__BoundParameters.Size(), "Invalid ParameterIndex");
    m__BoundParameters[ParameterIndex - 1] = true;
  #endif

  SHandleSqlError(mDatabase, ::sqlite3_bind_int64(mpSqliteStatement, 
    ParameterIndex, Value));
}

void TDatabase::TStatement::BindInt64(const TString& ParameterString, long long Value)
{
  BindInt64(ParameterIndex(ParameterString), Value);
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::BindText(int ParameterIndex, const TString& Value)
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    MAssert(ParameterIndex != 0, "Invalid ParameterIndex");
    MAssert(ParameterIndex - 1 < m__BoundParameters.Size(), "Invalid ParameterIndex");
    m__BoundParameters[ParameterIndex - 1] = true;
  #endif

  SHandleSqlError(mDatabase, ::sqlite3_bind_text16(mpSqliteStatement, 
    ParameterIndex, static_cast<const void*>(Value.Chars()), -1, SQLITE_TRANSIENT));
}

void TDatabase::TStatement::BindText(const TString& ParameterString, const TString& Value)
{
  BindText(ParameterIndex(ParameterString), Value);
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::BindNull(int ParameterIndex)
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    MAssert(ParameterIndex != 0, "Invalid ParameterIndex");
    MAssert(ParameterIndex - 1 < m__BoundParameters.Size(), "Invalid ParameterIndex");
    m__BoundParameters[ParameterIndex - 1] = true;
  #endif
  
  SHandleSqlError(mDatabase, ::sqlite3_bind_null(mpSqliteStatement, ParameterIndex));
}

void TDatabase::TStatement::BindNull(const TString& ParameterString)
{
  BindNull(ParameterIndex(ParameterString));
}

// -------------------------------------------------------------------------------------------------

bool TDatabase::TStatement::IsColumnNull(int ColumnIndex)const
{
  MAssert(mpSqliteStatement != NULL, "");

  return (SQLITE_NULL == ::sqlite3_column_type(
    const_cast<sqlite3_stmt*>(mpSqliteStatement), ColumnIndex));
}

// -------------------------------------------------------------------------------------------------

int TDatabase::TStatement::ColumnCount()const
{
  MAssert(mpSqliteStatement != NULL, "");
  
  return ::sqlite3_column_count(
    const_cast<sqlite3_stmt*>(mpSqliteStatement));
}

// -------------------------------------------------------------------------------------------------

TString TDatabase::TStatement::ColumnName(int ColumnIndex)
{
  MAssert(mpSqliteStatement != NULL, "");
  
  return static_cast<const TUnicodeChar*>(
    ::sqlite3_column_name16(mpSqliteStatement, ColumnIndex));
}

// -------------------------------------------------------------------------------------------------

bool TDatabase::TStatement::Step()
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    for (int i = 0; i < m__BoundParameters.Size(); ++i)
    {
      MAssert(m__BoundParameters[i], "Parameter was not bound. "
        "Set it to NULL if you want NULL");
    }
  #endif
  
  const int Result = ::sqlite3_step(mpSqliteStatement);
  
  switch (Result)
  {
  case SQLITE_DONE:
    SHandleSqlError(mDatabase, ::sqlite3_reset(mpSqliteStatement));
    return false;

  case SQLITE_ROW:
    return true;

  default:
    ::sqlite3_reset(mpSqliteStatement);
    SHandleSqlError(mDatabase, Result);
    return false; // never reached
  }
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::Execute()
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    for (int i = 0; i < m__BoundParameters.Size(); ++i)
    {
      MAssert(m__BoundParameters[i], "Parameter was not bound. "
        "Set it to NULL if you want NULL");
    }
  #endif
  
  const int Result = ::sqlite3_step(mpSqliteStatement);
  
  switch (Result)
  {
  case SQLITE_DONE:
    SHandleSqlError(mDatabase, ::sqlite3_reset(mpSqliteStatement));
    break;

  case SQLITE_ROW:
    throw TReadableException(MText("Database Error (File: %s): 'Expected no result entries'",
      mDatabase.FileNameAndPath()));
    
  default:
    ::sqlite3_reset(mpSqliteStatement);
    SHandleSqlError(mDatabase, Result);
  }
}

// -------------------------------------------------------------------------------------------------

void TDatabase::TStatement::Reset()
{
  MAssert(mpSqliteStatement != NULL, "");
  
  #if defined(MDebug)
    m__BoundParameters.Init(false);
  #endif
  
  SHandleSqlError(mDatabase, ::sqlite3_reset(mpSqliteStatement));
}

// -------------------------------------------------------------------------------------------------

// always returns 0 as type! 
/*TDatabase::TStatement::TColumnType TDatabase::TStatement::ColumnType(int ColumnIndex)const
{
  MAssert(mpSqliteStatement != NULL, "");
  
  const int Res = ::sqlite3_column_type(
    const_cast<sqlite3_stmt*>(mpSqliteStatement), ColumnIndex);
    
  switch (Res)
  {
  case 0: MInvalid("ColumnType is not supported on empty tables.");
    return kInvalidColumnType;
  
  case SQLITE_INTEGER:
    return kInteger;
  case SQLITE_FLOAT:
    return kDouble;
  case SQLITE_TEXT:
    return kText;
  case SQLITE_BLOB:
    return kBlob;
  case SQLITE_NULL:
    return kNull;
  
  default: MInvalid("");
    return kInvalidColumnType;
  }
}*/

// -------------------------------------------------------------------------------------------------

TString TDatabase::TStatement::ColumnTypeString(int ColumnIndex)
{
  MAssert(mpSqliteStatement != NULL, "");
  
  return static_cast<const TUnicodeChar*>(
    ::sqlite3_column_decltype16(mpSqliteStatement, ColumnIndex));
}

// -------------------------------------------------------------------------------------------------

const void* TDatabase::TStatement::ColumnBlob(
  int   ColumnIndex, 
  int& BlobSize)const
{
  MAssert(mpSqliteStatement != NULL, "");
  
  BlobSize = ::sqlite3_column_bytes(
    const_cast<sqlite3_stmt*>(mpSqliteStatement), ColumnIndex);
  
  return ::sqlite3_column_blob(
    const_cast<sqlite3_stmt*>(mpSqliteStatement), ColumnIndex);
}

// -------------------------------------------------------------------------------------------------

double TDatabase::TStatement::ColumnDouble(int ColumnIndex)const
{
  MAssert(mpSqliteStatement != NULL, "");
  
  return ::sqlite3_column_double(
    const_cast<sqlite3_stmt*>(mpSqliteStatement), ColumnIndex);
}

// -------------------------------------------------------------------------------------------------

bool TDatabase::TStatement::ColumnBool(int ColumnIndex)const
{
  MAssert(mpSqliteStatement != NULL, "");

  const int IntValue = ::sqlite3_column_int(
    const_cast<sqlite3_stmt*>(mpSqliteStatement), ColumnIndex);

  MAssert(IntValue == 1 || IntValue == 0, "Not a boolean!");
  return !!IntValue;
}

// -------------------------------------------------------------------------------------------------

int TDatabase::TStatement::ColumnInt(int ColumnIndex)const
{
  MAssert(mpSqliteStatement != NULL, "");
  
  return ::sqlite3_column_int(
    const_cast<sqlite3_stmt*>(mpSqliteStatement), ColumnIndex);
}

// -------------------------------------------------------------------------------------------------

long long TDatabase::TStatement::ColumnInt64(int ColumnIndex)const
{
  MAssert(mpSqliteStatement != NULL, "");
  
  return ::sqlite3_column_int64(
    const_cast<sqlite3_stmt*>(mpSqliteStatement), ColumnIndex);
}

// -------------------------------------------------------------------------------------------------

TString TDatabase::TStatement::ColumnText(int ColumnIndex)const
{
  return static_cast<const TUnicodeChar*>(
    ::sqlite3_column_text16(mpSqliteStatement, ColumnIndex));
}

