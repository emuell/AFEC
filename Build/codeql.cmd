@echo off

:: create a codeql database and run "cpp-code-scanning" queries on the entire source-tree
:: requires codeql-cli from https://github.com/github/codeql-cli-binaries/releases

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
