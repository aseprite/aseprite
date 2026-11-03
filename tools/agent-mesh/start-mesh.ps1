# tools/agent-mesh/start-mesh.ps1
param(
    [string]$AgentName = $env:AGENT_NAME,
    [string]$NatsUrl = $env:NATS_URL,
    [string]$Projects = $env:PROJECTS
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (-not $AgentName) {
    $AgentName = Read-Host "Enter AGENT_NAME (e.g. arin-pc)"
}
if (-not $NatsUrl) {
    $NatsUrl = Read-Host "Enter NATS_URL (e.g. nats://<TOKEN>@<hub-name>:4222)"
}
if (-not $Projects) {
    $Projects = Read-Host "Enter PROJECTS (comma-separated, e.g. aseprite2,pitch-quest)"
}

$env:AGENT_NAME = $AgentName
$env:NATS_URL = $NatsUrl
$env:PROJECTS = $Projects

Write-Host "Starting Agent Link as [$AgentName] connected to [$NatsUrl] for projects [$Projects]..." -ForegroundColor Cyan
Set-Location $scriptDir
python agent_link.py
