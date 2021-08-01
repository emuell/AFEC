@echo off

:: Create a codeql database and run "cpp-code-scanning" queries on the entire source-tree.
:: Requires a codeql repository clone and codeql-cli binaries, see 
:: https://codeql.github.com/docs/codeql-cli/getting-started-with-the-codeql-cli/ for more info.

SET CODEQL=%HOME%/bin/codeql/codeql.exe
IF NOT EXIST "%CODEQL%" GOTO :CODEQL_Error

:: ############################################################################

PUSHD %~dp0..
%CODEQL% database create ./Build/Out/CodeQL --overwrite --language="cpp" --command="call Build/build.cmd /release /c"
if %ERRORLEVEL% GTR 0 GOTO :Script_Error
%CODEQL% database analyze ./Build/Out/CodeQL cpp-code-scanning.qls --sarif-category=cpp --format=sarif-latest --output=Build/codeql-results.sarif
if %ERRORLEVEL% GTR 0 GOTO :Script_Error
POPD

GOTO :end

:: ############################################################################

:CODEQL_Error
ECHO. 
ECHO CODEQL (%CODEQL%) could not be found
GOTO :Script_Error

:Script_Error
ECHO. 
ECHO *** CodeQL script failed ***

:end
