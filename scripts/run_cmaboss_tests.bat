@echo off
rem Windows version of run_cmaboss_tests.sh: runs the cMaBoSS tests against the
rem installed cMaBoSS. PYTHON selects the interpreter (default: python).

setlocal
if "%PYTHON%"=="" set PYTHON=python
set return_code=0

pushd engine\tests\cmaboss

%PYTHON% -m unittest test
if errorlevel 1 (echo tests on classic ERR& set return_code=1) else (echo tests on classic OK)
%PYTHON% -m unittest test_128n
if errorlevel 1 (echo tests on bitsets ERR& set return_code=1) else (echo tests on bitsets OK)

popd

exit /b %return_code%
