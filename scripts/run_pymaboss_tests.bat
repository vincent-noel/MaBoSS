@echo off
rem Windows version of run_pymaboss_tests.sh, which documents what it does and why.
rem
rem usage: scripts\run_pymaboss_tests.bat [CMABOSS]
rem   CMABOSS  what to install as cMaBoSS: a wheel, or a source directory
rem            (default: engine\python, built by pip)
rem
rem environment: PYTHON (default: python), PIP_FLAGS,
rem   PYMABOSS_REPO (default: vincent-noel/pyMaBoSS), PYMABOSS_REF (default: develop)

setlocal
if "%PYTHON%"=="" set PYTHON=python
if "%PYMABOSS_REPO%"=="" set PYMABOSS_REPO=https://github.com/vincent-noel/pyMaBoSS.git
if "%PYMABOSS_REF%"=="" set PYMABOSS_REF=develop
if "%~1"=="" (set CMABOSS=%CD%\engine\python) else (set CMABOSS=%~f1)

set WORK_DIR=%TEMP%\pymaboss-tests-%RANDOM%%RANDOM%
set PYMABOSS_DIR=%WORK_DIR%\pyMaBoSS
mkdir "%WORK_DIR%" || exit /b 1

rem Build a source directory once: pip would rebuild it for each install below.
if exist "%CMABOSS%\pyproject.toml" (
    %PYTHON% -m pip wheel --no-deps -w "%WORK_DIR%\wheel" "%CMABOSS%" || (rmdir /s /q "%WORK_DIR%" & exit /b 1)
    for %%w in ("%WORK_DIR%\wheel\cmaboss-*.whl") do set CMABOSS=%%~fw
)

git clone --quiet --depth 1 --branch %PYMABOSS_REF% %PYMABOSS_REPO% "%PYMABOSS_DIR%" || (rmdir /s /q "%WORK_DIR%" & exit /b 1)
for /f %%h in ('git -C "%PYMABOSS_DIR%" rev-parse --short HEAD') do echo pyMaBoSS %PYMABOSS_REF% at %%h

%PYTHON% -m pip install %PIP_FLAGS% --force-reinstall --no-deps "%CMABOSS%" || (rmdir /s /q "%WORK_DIR%" & exit /b 1)
%PYTHON% -m pip install %PIP_FLAGS% "%CMABOSS%" "%PYMABOSS_DIR%" || (rmdir /s /q "%WORK_DIR%" & exit /b 1)
%PYTHON% -m maboss_setup_experimental || (rmdir /s /q "%WORK_DIR%" & exit /b 1)

set return_code=0
pushd "%PYMABOSS_DIR%"

%PYTHON% -m unittest test.test_probtrajs_cmaboss
if errorlevel 1 (echo probtrajs_cmaboss ERR& set return_code=1) else (echo probtrajs_cmaboss OK)
%PYTHON% -m unittest test.test_observed_graph_cmaboss
if errorlevel 1 (echo observed_graph_cmaboss ERR& set return_code=1) else (echo observed_graph_cmaboss OK)
%PYTHON% -m unittest test.test_uppmaboss_cmaboss
if errorlevel 1 (echo uppmaboss_cmaboss ERR& set return_code=1) else (echo uppmaboss_cmaboss OK)
rem The other test_loadsbml cases run the MaBoSS executables, not cMaBoSS.
%PYTHON% -m unittest test.test_loadsbml.TestLoadSBML.test_load_sbml_cmaboss
if errorlevel 1 (echo load_sbml_cmaboss ERR& set return_code=1) else (echo load_sbml_cmaboss OK)

popd
rmdir /s /q "%WORK_DIR%"

exit /b %return_code%
