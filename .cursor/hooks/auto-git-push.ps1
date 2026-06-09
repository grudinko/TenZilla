# Cursor hook (event: stop) - auto git commit + push after agent session ends.
# Runs from project root. Log: .cursor/hooks/auto-git-push.log
#
# Disable push only:  $env:TENZILLA_AUTO_GIT_PUSH = "0"
# Disable hook fully: remove the "stop" entry in .cursor/hooks.json

$ErrorActionPreference = "Continue"
$env:GIT_TERMINAL_PROMPT = "0"

if ([Console]::IsInputRedirected) {
    try {
        $null = [Console]::In.ReadToEnd()
    } catch {
    }
}

$LogFile = Join-Path $PSScriptRoot "auto-git-push.log"
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path

function Write-Log {
    param([string]$Message)
    $line = (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') + " " + $Message
    try {
        Add-Content -Path $LogFile -Value $line -Encoding UTF8
    } catch {
    }
}

Set-Location $ProjectRoot

if (-not (Test-Path (Join-Path $ProjectRoot ".git"))) {
    Write-Log "Skip: .git not found"
    exit 0
}

$porcelain = git status --porcelain 2>&1 | Out-String
if ($LASTEXITCODE -ne 0) {
    Write-Log ("Skip: git status failed - " + $porcelain.Trim())
    exit 0
}

if ([string]::IsNullOrWhiteSpace($porcelain)) {
    Write-Log "Skip: working tree clean"
    exit 0
}

$commitMsg = "Auto: TenZilla update " + (Get-Date -Format 'dd.MM.yyyy HH:mm')

git add -A 2>&1 | ForEach-Object { Write-Log ("add: " + $_) }
if ($LASTEXITCODE -ne 0) {
    Write-Log ("Error: git add failed (exit " + $LASTEXITCODE + ")")
    exit 0
}

$commitOut = git commit -m $commitMsg 2>&1
$commitOut | ForEach-Object { Write-Log ("commit: " + $_) }
if ($LASTEXITCODE -ne 0) {
    Write-Log ("Error: git commit failed (exit " + $LASTEXITCODE + ")")
    exit 0
}

$pushDisabled = $env:TENZILLA_AUTO_GIT_PUSH
if ($pushDisabled -eq "0" -or $pushDisabled -eq "false" -or $pushDisabled -eq "off") {
    Write-Log ("OK: committed, push disabled (TENZILLA_AUTO_GIT_PUSH=" + $pushDisabled + ")")
    exit 0
}

$branch = (git rev-parse --abbrev-ref HEAD 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0) {
    Write-Log "Error: cannot detect branch"
    exit 0
}

$pushOut = git push origin $branch 2>&1
$pushOut | ForEach-Object { Write-Log ("push: " + $_) }
if ($LASTEXITCODE -ne 0) {
    Write-Log ("Error: git push failed (exit " + $LASTEXITCODE + ")")
    exit 0
}

Write-Log ("OK: committed and pushed to origin/" + $branch)
exit 0
