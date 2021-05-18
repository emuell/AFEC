#pragma once

#ifndef DataBase_h
#define DataBase_h

// =================================================================================================

#include "CoreTypes/Export/Str.h"
#include "CoreTypes/Export/Array.h"

// =================================================================================================

/*!
 *  Wrapper class for a Sqlite database
!*/

class TDatabase
{
public:
  class TStatement;

  static TString SSqliteVersionString();
  static int SSqliteVersionNumber();

  //! creates a new DB if non exists at the given location.
  //! Leave the string empty to not open anything...
  TDatabase(const TString& DbPath = TString());
  ~TDatabase();
  
  
  //@{ ... Open / Close / Create a database

  TString FileNameAndPath()const;

  bool IsOpen()const;
  
  bool Open(const TString& DbPath);
  void Close();
  //@}
  

  //@{ ... Setup custom keywords
  
  typedef bool (*TPMatchFunction)(
    void*       pContext,
    const char* pFirst, // UTF8 encoded string
    const char* pSecond); // UTF8 encoded string

  // Invoke match operator set via \function SetMatchOperator
  bool InvokeMatchOperator(
    const char* pFirst,
    const char* pSecond)const;
  //! by default unset for a database. You may want to specify your own 
  //! rule for the keyword MATCH (for example gStringsEqualIgnoreCase)...
  void SetMatchOperator(
    void*           pContext, 
    TPMatchFunction pMatchfunction);
  //@}


  //@{ ... Transactions

  /*! 
   * Undoable Transaction in a database: 
   * Construct one on the stack and to roll back all made
   * transaction as soon as its not finalized (in case of an exception).
  !*/
  class TTransaction
  {
  public:
    //! Start a new transaction
    TTransaction(TDatabase& Database);
    
    //! rolls back when not finished
    ~TTransaction();
  
    //! Commit and avoid rollback
    void Commit();
    
  private:
    TDatabase& mDatabase;
    bool mCommited;
  };
  //@}
  

  //@{ ... Statements

  int NumberOfChanges()const;
  int TotalNumberOfChanges()const;
  
  long long LastInsertedRowIndex()const;
  
  //! Executing a batch or a single statement without results
  void Execute(const TString& StatementString);
  
  //! Executes singleton SQL statement. (TStatement shortcuts)
  int ExecuteScalarInt(const TString& StatementString);
  long long ExecuteScalarInt64(const TString& StatementString);
  double ExecuteScalarDouble(const TString& StatementString);
  TString ExecuteScalarText(const TString& StatementString);

  // ===============================================================================================

  /*!
   *  Precompiled statement wrapper for a database
  !*/

  class TStatement
  {
  public:
    TStatement(TDatabase& Database, const TString& StatementString);
    TStatement(TDatabase& Database, const char* pStatementString);
    ~TStatement();

    
    //@{ ... Execution
    
    //! Execute a void statement which returns no results.
    void Execute();
    
    //! Execute a statement which produces results, you want to iterate.
    //! Step through the results in a while loop, until Step() returns false
    bool Step();
    
    //! reset the statement    
    void Reset();
    //@}
    
    
    //@{ ... Parameters
    
    int ParameterCount()const;

    TString ParameterName(int ParameterIndex)const;
    int ParameterIndex(const TString& ParameterString);
    //@}
    
    //@{ ... Write/Bind Parameters values.

    void BindNull(int ParameterIndex);
    void BindNull(const TString& ParameterString);

    void BindBool(int ParameterIndex, bool Value);
    void BindBool(const TString& ParameterString, bool Value);

    void BindInt(int ParameterIndex, int Value);
    void BindInt(const TString& ParameterString, int Value);

    void BindInt64(int ParameterIndex, long long Value);
    void BindInt64(const TString& ParameterString, long long Value);

    void BindDouble(int ParameterIndex, double Value);
    void BindDouble(const TString& ParameterString, double Value);

    void BindText(int ParameterIndex, const TString& Value);
    void BindText(const TString& ParameterString, const TString& Value);

    void BindBlob(int ParameterIndex, const void* pBlobData, int BlobSize);
    void BindBlob(const TString& ParameterString, const void* pBlobData, int BlobSize);
    //@}

    
    //@{ ... Read Column Data
    
    enum TColumnType
    {
      kInvalidColumnType = -1,
      
      kInteger,
      kDouble,
      kText,
      kBlob,
      kNull,
      
      kNumberOfColumnTypes
    };
    
    bool IsColumnNull(int ColumnIndex)const;

    int ColumnCount()const;
    TString ColumnName(int ColumnIndex);

    TString ColumnTypeString(int ColumnIndex);
    
    const void* ColumnBlob(int ColumnIndex, int& nBlobSize)const;
    double ColumnDouble(int ColumnIndex)const;

    bool ColumnBool(int ColumnIndex)const;
    int ColumnInt(int ColumnIndex)const;
    long long ColumnInt64(int ColumnIndex)const;
    TString ColumnText(int ColumnIndex)const;
    //@}
    
  private:
    // copying is not allowed
    TStatement(TStatement& Statement);
    TStatement& operator=(TStatement& Statement);

    TDatabase& mDatabase;
    struct sqlite3_stmt* mpSqliteStatement;
    
    #if defined(MDebug)
      TArray<bool> m__BoundParameters;
    #endif
  };
  //@}
  
private:
  friend class TStatement; // for mpSqliteDatabase
  
  // Copying is not allowed
  TDatabase(TDatabase& db);
  TDatabase& operator=(TDatabase& db);
  
  static void SHandleSqlError(
    const TDatabase& Database, int ErrorCode);
  static void SHandleSqlError(
    const TDatabase& Database, int ErrorCode, const TString& Message);

  int Open(const TString& DbPath, bool ThrowOnError);

  struct sqlite3* mpSqliteDatabase;

  TPMatchFunction mpMatchfunction;
  void *mpMatchfunctionContext;

  TString mFileNameAndPath;
};


#endif 

