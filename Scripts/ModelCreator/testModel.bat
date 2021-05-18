:: runs the modeltester for the specified classification model train data folder, 
:: e.g "OneShot-vs-Loops" in the afec/classification-packs repository

@echo off

:: ------------------------------------------------------------------------------------------------
:: get and verify paths
set CLASSIFICATION_FOLDER=%~1
CALL :NORMALIZEPATH "%CLASSIFICATION_FOLDER%"
set CLASSIFICATION_FOLDER=%RETVAL%
if NOT EXIST "%CLASSIFICATION_FOLDER%" goto :CLASSIFICATION_FOLDER_Error

set BIN_DIR=%~dp0..\..\Dist\Release
if NOT EXIST "%BIN_DIR%" goto BIN_DIR_Error

:: ------------------------------------------------------------------------------------------------
:: run crawler to create descriptors
if EXIST "%CLASSIFICATION_FOLDER%\afec-ll.db" goto Test
%BIN_DIR%\Crawler.exe -l low -o "%CLASSIFICATION_FOLDER%" "%CLASSIFICATION_FOLDER%"
if %ERRORLEVEL% GTR 0 goto :Crawler_Error

:: run model tester
:Test
%BIN_DIR%\ModelTester.exe --repeat=5 --seed=300 --all=true --bagging=false "%CLASSIFICATION_FOLDER%\afec-ll.db"
if %ERRORLEVEL% GTR 0 goto :ModelTester_Error

:: copy results: NB: assumes GBDT is the default model
xcopy /s /y "%~dp0..\..\Projects\Crawler\XModelTester\Resources\Results\GBDT\*" "%CLASSIFICATION_FOLDER%"

goto :end

:: ------------------------------------------------------------------------------------------------
:CLASSIFICATION_FOLDER_Error
echo. 
echo *** Missing or invalid argument: Can't find classification data set folder at
echo *** %CLASSIFICATION_FOLDER%
echo *** You can download the classification packs via:
echo *** `git clone --depth 1 https://git.renoise.com/afec/classification-packs.git "AFEC-Classifiers"`
goto :end

:: ------------------------------------------------------------------------------------------------
:BIN_DIR_Error
echo. 
echo *** Can't find binaries at %BIN_DIR%. Run crawler build scripts first.
goto :end

:: ------------------------------------------------------------------------------------------------
:Crawler_Error
echo. 
echo *** Crawler failed to run
goto :end

:: ------------------------------------------------------------------------------------------------
:ModelTester_Error
echo. 
echo *** ModelTester failed to run
GOTO :end

:: ------------------------------------------------------------------------------------------------
:NORMALIZEPATH
  SET RETVAL=%~dpfn1
  EXIT /B

:: ------------------------------------------------------------------------------------------------

:end
PAUSE
