$ErrorActionPreference = 'Stop'
$tk5 = Join-Path $PSScriptRoot 'mvs-tk5'
Set-Location $tk5
$env:HERCULES_RC = 'scripts/ipl.rc'
$env:TK5CRLF = 'CRLF'
$env:MAINSIZE = '16'
Remove-Item -Force -ErrorAction SilentlyContinue log\3033.log
$exe = Join-Path $tk5 'hercules\windows\64\hercules.exe'
$p = Start-Process -FilePath $exe -ArgumentList '-d','-f','conf\tk5.cnf' `
     -WorkingDirectory $tk5 -WindowStyle Hidden -PassThru `
     -RedirectStandardOutput (Join-Path $tk5 'log\3033.log') `
     -RedirectStandardError  (Join-Path $tk5 'log\3033.err')
"hercules pid $($p.Id)"
