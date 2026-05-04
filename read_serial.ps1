param(
    [string]$portName = "COM3",
    [int]$baud = 115200,
    [string]$logFile = "serial_log.txt"
)

$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

while ($true) {
    $serial = $null
    try {
        $serial = New-Object System.IO.Ports.SerialPort $portName, $baud, None, 8, One
        $serial.ReadTimeout = 2000
        $serial.Open()

        $msg = "[$timestamp] Connected to $portName at ${baud} baud"
        Write-Host $msg
        Add-Content -Path $logFile -Value $msg

        while ($true) {
            try {
                $line = $serial.ReadLine()
                $line = $line.TrimEnd("`r", "`n")
                if ($line -ne "") {
                    $timeStr = Get-Date -Format "HH:mm:ss.fff"
                    $output = "[$timeStr] $line"
                    Write-Host $output
                    Add-Content -Path $logFile -Value $output
                }
            } catch [TimeoutException] {
                # normal timeout, continue
            }
        }
    }
    catch {
        $errMsg = "[$timestamp] Error: $_"
        Write-Host $errMsg
        Add-Content -Path $logFile -Value $errMsg
        
        if ($serial -and $serial.IsOpen) {
            try { $serial.Close() } catch {}
        }
        
        Write-Host "Retrying in 3 seconds..."
        Start-Sleep -Seconds 3
    }
    finally {
        if ($serial -and $serial.IsOpen) {
            try { $serial.Close() } catch {}
        }
    }
}
