param(
    [string]$ProbePath = (Join-Path $PSScriptRoot 'mingw_runtime_probe.exe'),
    [string]$ReportPath = (Join-Path $PSScriptRoot '..\..\outputs\Coneic-VLC-validation\mingw-runtime-results.txt')
)
$ErrorActionPreference = 'Stop'
$probeAbsolute = (Resolve-Path -LiteralPath $ProbePath).Path
$scratchPath = Join-Path $PSScriptRoot ('compat-scratch-' + [guid]::NewGuid().ToString('N') + '.tmp')
$probeOutput = & $probeAbsolute $scratchPath 2>&1
$probeExit = $LASTEXITCODE
$resultLines = [Collections.Generic.List[string]]::new()
$resultLines.Add('Coneic MinGW compatibility runtime probe')
$resultLines.Add('UTC: ' + [DateTime]::UtcNow.ToString('o'))
$resultLines.Add('Executable: ' + $probeAbsolute)
$resultLines.Add('SHA256: ' + (Get-FileHash -LiteralPath $probeAbsolute -Algorithm SHA256).Hash)
$resultLines.Add('Exit code: ' + $probeExit)
foreach ($resultLine in $probeOutput) { $resultLines.Add([string]$resultLine) }
$resultLines.Add('Scope: adapter behavior only; not full VLC or plugin runtime parity.')
$resultText = $resultLines -join [Environment]::NewLine
$reportAbsolute = [IO.Path]::GetFullPath($ReportPath)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($reportAbsolute)) | Out-Null
[IO.File]::WriteAllText($reportAbsolute, $resultText + [Environment]::NewLine)
Write-Output $resultText
if ($probeExit -ne 0) { exit $probeExit }
