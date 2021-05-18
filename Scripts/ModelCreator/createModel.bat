:: runs the model creator for the specified classification model train data folder, 
:: e.g "OneShot-vs-Loops" in the afec/classification-packs repository
:: the resulting model is saved to the crawlers resource dirs, so the crawler will use
:: it the next time it's creating afec.db high level databases. 

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
:: run crawler to create descriptors, if needed
if EXIST "%CLASSIFICATION_FOLDER%\afec-ll.db" goto Train
%BIN_DIR%\Crawler.exe -l low -o "%CLASSIFICATION_FOLDER%" "%CLASSIFICATION_FOLDER%"
if %ERRORLEVEL% GTR 0 goto :Crawler_Error

:: ------------------------------------------------------------------------------------------------
:: train and write the model
:Train
%BIN_DIR%\ModelCreator.exe "%CLASSIFICATION_FOLDER%\afec-ll.db"
if %ERRORLEVEL% GTR 0 goto :ModelCreator_Error

goto :end

:: ------------------------------------------------------------------------------------------------
:CLASSIFICATION_FOLDER_Error
echo. 
echo *** Missing or invalid argument: Can't find classification data set folder at 
echo *** `%CLASSIFICATION_FOLDER%`
echo *** You can download the classification packs with:
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
:ModelCreator_Error
echo. 
echo *** ModelCreator failed to run
GOTO :end

:: ------------------------------------------------------------------------------------------------
:NORMALIZEPATH
  SET RETVAL=%~dpfn1
  EXIT /B

:: ------------------------------------------------------------------------------------------------
:end
PAUSE
