# Test run script for CSim.exe with WorkingDirectory set to build\Debug
Set-Location "c:\Users\gravi\Source\Projects\CSim - Copy\CSim\build\Debug"

# Clear old log file in build\Debug
if (Test-Path "log.txt") {
    Remove-Item "log.txt" -Force
}

Write-Output "Starting CSim.exe with WorkingDirectory=build\Debug..."
$process = New-Object System.Diagnostics.Process
$process.StartInfo.FileName = "c:\Users\gravi\Source\Projects\CSim - Copy\CSim\build\Debug\CSim.exe"
$process.StartInfo.WorkingDirectory = "c:\Users\gravi\Source\Projects\CSim - Copy\CSim\build\Debug"
$process.StartInfo.UseShellExecute = $false
$process.StartInfo.RedirectStandardOutput = $false
$process.StartInfo.RedirectStandardError = $false

$started = $process.Start()
if (-not $started) {
    Write-Output "Error: Failed to start CSim.exe"
    exit 1
}

Write-Output "Started CSim.exe with Process ID: $($process.Id)"

# Loop to wait for window handle
$hasWindow = $false
$windowTitle = ""
$windowHandle = 0
$threadCount = 0

for ($i = 0; $i -lt 10; $i++) {
    Start-Sleep -Milliseconds 500
    $process.Refresh()
    $windowHandle = $process.MainWindowHandle
    $windowTitle = $process.MainWindowTitle
    if ($windowHandle -ne 0) {
        $hasWindow = $true
        $threadCount = $process.Threads.Count
        break
    }
}

if ($hasWindow) {
    Write-Output "Success: Window detected!"
    Write-Output "Window Handle: $windowHandle"
    Write-Output "Window Title:  $windowTitle"
    Write-Output "Thread Count:  $threadCount"
} else {
    Write-Output "Warning: No window handle detected within timeout."
}

# Wait 2 more seconds
Start-Sleep -Seconds 2

# Check if process is still running
$process.Refresh()
if ($process.HasExited) {
    Write-Output "Process exited prematurely with code: $($process.ExitCode)"
} else {
    # Verify closing the window cleanly
    Write-Output "Sending close message (WM_CLOSE) to main window..."
    $closedCleanly = $process.CloseMainWindow()
    Write-Output "CloseMainWindow returned: $closedCleanly"
    
    # Wait for the process to exit
    Write-Output "Waiting for process to exit..."
    $exited = $process.WaitForExit(5000) # wait up to 5 seconds
    
    $process.Refresh()
    if ($process.HasExited) {
        Write-Output "Process exited cleanly."
        Write-Output "Exit Code: $($process.ExitCode)"
        Write-Output "Exit Time: $($process.ExitTime)"
    } else {
        Write-Output "Process did not exit within timeout. Killing..."
        $process.Kill()
    }
}

# Read log.txt in build\Debug if it exists
if (Test-Path "log.txt") {
    Write-Output "--- Content of log.txt in build\Debug ---"
    Get-Content "log.txt"
}
