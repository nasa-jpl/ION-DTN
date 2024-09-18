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