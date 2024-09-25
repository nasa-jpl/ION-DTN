# Due to the differences in resource management within windows, the pipe that ION creates 
# has to be killed via the Win32 API. MSYS/MSYS2 isn't able to handle that natively, so it calls
# this powershell script to handle the cleanup of the pipe.

$pipeName = "\\.\pipe\ion.pipe"
$msg = [byte[]](0, 0, 0, 0, 0)

try {
    $pipe = [System.IO.Pipes.NamedPipeClientStream]::new(".", "ion.pipe", [System.IO.Pipes.PipeDirection]::Out)
    $pipe.Connect(100)
    $pipe.Write($msg, 0, $msg.Length)
    $pipe.Close()
    Write-Host "Pipe closed"
} catch {
    Write-Host "Pipe DNE"
}