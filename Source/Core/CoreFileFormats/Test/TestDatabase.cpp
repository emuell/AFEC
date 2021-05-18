#include "CoreFileFormatsPrecompiledHeader.h"

#include "CoreTypes/Export/TestHelpers.h"
#include "CoreFileFormats/Test/TestDatabase.h"
                  
// =================================================================================================

int sNumberOfMatchOperatorCalls = 0;

// -------------------------------------------------------------------------------------------------

static bool STestMatchOperator(
  void*       pContext,
  const char* pFirst,
  const char* pSecond)
{
  ++sNumberOfMatchOperatorCalls;
  return gStringsEqualIgnoreCase(
    TString(pFirst, TString::kUtf8), TString(pSecond, TString::kUtf8));
}

// =================================================================================================

// -------------------------------------------------------------------------------------------------

void TCoreFileFormatsTest::Database()
{
  // wrong linked version?
  BOOST_CHECK_EQUAL(TDatabase::SSqliteVersionNumber(), 3019003);
  
  const int sTableSize = 25;
  const int sBlobBlockSize = 128;
  
  TDatabase Db(gTempDir().Path() + "TestDatabase.db");
  
  
  // ... Table Creation
  
  Db.Execute(
    "CREATE TABLE Test( a INTEGER PRIMARY KEY, b TEXT, c INTEGER, d DOUBLE, "
      "e LONG INTEGER, f BLOB )");
      
  Db.Execute(
    "CREATE INDEX d ON Test( d DESC ); "
    "CREATE INDEX e ON Test( e )"
   );
   
  
  // ... Insert (and binding)
  {
    TDatabase::TTransaction Transaction(Db);
    
    TDatabase::TStatement InsertStatement(Db, 
      "INSERT INTO Test(b,c,d,e,f) VALUES(?,?,?,:long_param,:blob_param)");

    for (int i = 1; i <= sTableSize; ++i)
    {
      InsertStatement.BindText(1, "Text Content");
      InsertStatement.BindInt(2, TRandom::Integer(2<<29));
      
      if (i%2)
      {
        InsertStatement.BindDouble(3, TRandom::Integer(2<<29) / 2.7896);
      }
      else
      {
        InsertStatement.BindNull(3);
      }
      
      InsertStatement.BindInt64(":long_param", TRandom::Integer(2<<29));
      
      const int BlockSize = TRandom::Integer(2<<29) % sBlobBlockSize;
      TArray<char> BlobData(BlockSize);
      
      for (int j = 0; j < BlockSize; ++j)
      {
        BlobData[j] = (char)TRandom::Integer(2<<8);
      }
      
      InsertStatement.BindBlob(":blob_param", BlobData.FirstRead(), BlobData.Size());
      
      InsertStatement.Execute();
    }
      
    
    BOOST_CHECK_EQUAL(Db.LastInsertedRowIndex(), sTableSize);
    //BOOST_CHECK_EQUAL(Db.TotalNumberOfChanges(), sTableSize * 5);
    
    Transaction.Commit();
  }
  
  
  // ... Select
  {  
    TDatabase::TStatement SelectStatement(Db,
      "SELECT a as 'Hello from a',b as 'Hello from b',c as 'Hello from c',"
        "d as 'Hello from d',e,f FROM Test ORDER BY e DESC");
    
    BOOST_CHECK_EQUAL(SelectStatement.ColumnCount(), 6);
    
    BOOST_CHECK_EQUAL(SelectStatement.ColumnName(0), "Hello from a");
    BOOST_CHECK_EQUAL(SelectStatement.ColumnTypeString(0), "INTEGER");
    
    BOOST_CHECK_EQUAL(SelectStatement.ColumnName(1), "Hello from b");
    BOOST_CHECK_EQUAL(SelectStatement.ColumnTypeString(1), "TEXT");
    
    BOOST_CHECK_EQUAL(SelectStatement.ColumnName(2), "Hello from c");
    BOOST_CHECK_EQUAL(SelectStatement.ColumnTypeString(2), "INTEGER");
    
    BOOST_CHECK_EQUAL(SelectStatement.ColumnName(3), "Hello from d");
    BOOST_CHECK_EQUAL(SelectStatement.ColumnTypeString(3), "DOUBLE");
    
    BOOST_CHECK_EQUAL(SelectStatement.ColumnName(4), "e");
    BOOST_CHECK_EQUAL(SelectStatement.ColumnTypeString(4), "LONG INTEGER");
    
    BOOST_CHECK_EQUAL(SelectStatement.ColumnName(5), "f");
    BOOST_CHECK_EQUAL(SelectStatement.ColumnTypeString(5), "BLOB");
    
    int NumRows = 0;
    
    while (SelectStatement.Step())
    {
      ++NumRows;
    }
    
    BOOST_CHECK_EQUAL(NumRows, sTableSize);
  }  
  
  
  // ... Select with custom MATCH operator
  {
    Db.SetMatchOperator(NULL, STestMatchOperator);

    TDatabase::TStatement SelectStatement(Db,
      "SELECT * FROM Test WHERE b MATCH 'text content'");

    int NumRows = 0;

    while (SelectStatement.Step())
    {
      ++NumRows;
    }

    BOOST_CHECK(sNumberOfMatchOperatorCalls > 0);
    BOOST_CHECK(NumRows > 0);
  }


  // ... Rolled back Transaction
  {
    {
      TDatabase::TTransaction Transaction(Db);
       
      TDatabase::TStatement InsertStatement(Db, 
        "INSERT INTO Test(b,c,d,e,f) VALUES(?,?,?,:long_param,:blob_param)");

      for (int i = 1; i <= sTableSize; ++i)
      {
        InsertStatement.BindText(1, "Text Content");
        InsertStatement.BindInt(2, TRandom::Integer(2<<29));
        InsertStatement.BindNull(3);
        InsertStatement.BindNull(4);
        InsertStatement.BindNull(5);
        
        InsertStatement.Execute();
      }
      
      // NOT commited
    }  
  
    TDatabase::TStatement SelectStatement(Db, "SELECT a,b,c,d,e,f FROM Test");
    
    int NumRows = 0;
    
    while (SelectStatement.Step())
    {
      ++NumRows;
    }
    
    BOOST_CHECK_EQUAL(NumRows, sTableSize);
  }
  
   
  // ... Random scalar access
  
  for (int i = 1; i < sTableSize; i++)
  {
    int Index = 1 + (rand() % sTableSize);
    
    const TString b = Db.ExecuteScalarText(
      "SELECT b FROM TEST WHERE a=" + ToString(Index));
    MUnused(b);  
    
    const double d = Db.ExecuteScalarDouble(
      "SELECT d FROM TEST WHERE a=" + ToString(Index));
    MUnused(d);  
    
    const long long e = Db.ExecuteScalarInt64(
      "SELECT e FROM TEST WHERE a=" + ToString(Index));
    MUnused(e);  
  }
}

