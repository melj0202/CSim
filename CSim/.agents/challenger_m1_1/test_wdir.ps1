# Check working directory of CSim.exe
$process = New-Object System.Diagnostics.Process
$process.StartInfo.FileName = "c:\Users\gravi\Source\Projects\CSim - Copy\CSim\build\Debug\CSim.exe"
$process.StartInfo.UseShellExecute = $false
$process.Start() | Out-Null
Start-Sleep -Seconds 1
Get-CimInstance Win32_Process -Filter "ProcessId = $($process.Id)" | Format-List Name, WorkingDirectory, CommandLine, ExecutablePath
$process.CloseMainWindow() | Out-Null
$process.WaitForExit(5000) | Out-Null
