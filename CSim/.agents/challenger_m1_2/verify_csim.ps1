# verification script for CSim
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# Add MSVC toolchain path to environment PATH so ASan DLL can be found
$msvcPath = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64"
if (Test-Path $msvcPath) {
    $env:PATH = "$msvcPath;" + $env:PATH
    Write-Output "Added MSVC path to PATH: $msvcPath"
} else {
    Write-Output "WARNING: MSVC path not found at $msvcPath"
}

$workingDir = "c:\Users\gravi\Source\Projects\CSim - Copy\CSim"
$outputDir = "c:\Users\gravi\Source\Projects\CSim - Copy\CSim\.agents\challenger_m1_2"
$exePath = Join-Path $workingDir "build\Debug\CSim.exe"

Write-Output "Cleaning up old stdout.log, stderr.log, and screenshot.png..."
Remove-Item (Join-Path $workingDir "stdout.log") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $workingDir "stderr.log") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $outputDir "screenshot.png") -ErrorAction SilentlyContinue

# Record the starting size of log.txt to see what gets appended
$logPath = Join-Path $workingDir "log.txt"
$logStartSize = 0
if (Test-Path $logPath) {
    $logStartSize = (Get-Item $logPath).Length
}
Write-Output "log.txt start size: $logStartSize bytes"

Write-Output "Launching CSim.exe in background using native Start-Process..."
try {
    $csimProcess = Start-Process -FilePath $exePath -WorkingDirectory $workingDir -RedirectStandardOutput (Join-Path $workingDir "stdout.log") -RedirectStandardError (Join-Path $workingDir "stderr.log") -PassThru
} catch {
    Write-Output "ERROR failed to start process: $_"
    Exit 1
}

# Give it 2 seconds to launch the window
Start-Sleep -Seconds 2

# Check if process is still running
if ($csimProcess.HasExited) {
    Write-Output "ERROR: CSim.exe exited immediately! ExitCode: $($csimProcess.ExitCode)"
    if (Test-Path (Join-Path $workingDir "stderr.log")) {
        Write-Output "--- stderr.log content ---"
        Get-Content (Join-Path $workingDir "stderr.log")
    }
    Exit 1
}

Write-Output "CSim process running (PID: $($csimProcess.Id)). Waiting 3 seconds for rendering..."
Start-Sleep -Seconds 3

# Take screenshot
Write-Output "Capturing screenshot..."
try {
    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    $graphics.Dispose()
    $screenshotPath = Join-Path $outputDir "screenshot.png"
    $bitmap.Save($screenshotPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
    Write-Output "Screenshot saved successfully to $screenshotPath"
} catch {
    Write-Output "Warning: Failed to capture screenshot: $_"
}

# Close window
Write-Output "Sending close signal to CSim window..."
$closeResult = $csimProcess.CloseMainWindow()
Write-Output "CloseMainWindow returned: $closeResult"

# Wait for exit
Write-Output "Waiting for process to exit cleanly..."
$hasExited = $csimProcess.WaitForExit(5000)

if ($hasExited) {
    Write-Output "Process exited cleanly. ExitCode: $($csimProcess.ExitCode)"
} else {
    Write-Output "WARNING: Process hung! Force terminating..."
    $csimProcess.Kill()
    Write-Output "Process force-terminated."
}

# Wait for file handles to release
Start-Sleep -Seconds 1

# Display logs
Write-Output "--- stdout.log ---"
if (Test-Path (Join-Path $workingDir "stdout.log")) {
    Get-Content (Join-Path $workingDir "stdout.log")
} else {
    Write-Output "(stdout.log not found)"
}

Write-Output "--- stderr.log ---"
if (Test-Path (Join-Path $workingDir "stderr.log")) {
    Get-Content (Join-Path $workingDir "stderr.log")
} else {
    Write-Output "(stderr.log not found)"
}

Write-Output "--- New content in log.txt ---"
if (Test-Path $logPath) {
    $logEndSize = (Get-Item $logPath).Length
    if ($logEndSize -gt $logStartSize) {
        $logFile = [System.IO.File]::OpenRead($logPath)
        $logFile.Seek($logStartSize, [System.IO.SeekOrigin]::Begin) | Out-Null
        $reader = New-Object System.IO.StreamReader($logFile)
        $newLogContent = $reader.ReadToEnd()
        $reader.Close()
        $logFile.Close()
        Write-Output $newLogContent
    } else {
        Write-Output "(No new content in log.txt)"
    }
} else {
    Write-Output "(log.txt not found)"
}
