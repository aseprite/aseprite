# tools/agent-mesh/manage-tokens.ps1
param(
    [Parameter(Position=0, Mandatory=$true)]
    [ValidateSet("list", "create", "revoke")]
    [string]$Command,

    [string]$Agent,
    [string]$Projects,
    [string]$Token,
    [string]$Description,
    [switch]$ActiveOnly,
    [switch]$Purge,
    [string]$HubUrl = $env:HUB_URL,
    [string]$AdminToken = $env:ADMIN_TOKEN
)

$ErrorActionPreference = "Stop"

if (-not $HubUrl) {
    $HubUrl = "https://agent-mesh-hub.slack-clone-arin.workers.dev"
}
$HubUrl = $HubUrl.TrimEnd('/')

if (-not $AdminToken) {
    if ($env:ADMIN_TOKEN) {
        $AdminToken = $env:ADMIN_TOKEN
    } elseif ($env:HUB_TOKEN -and -not $env:HUB_TOKEN.StartsWith("amt_")) {
        $AdminToken = $env:HUB_TOKEN
    } else {
        $AdminToken = "cVc8xLDpHfMujg1GEkimVaDdSdhdR70k"
    }
}

$headers = @{
    "Authorization" = "Bearer $AdminToken"
    "Content-Type"  = "application/json"
    "User-Agent"    = "AgentMesh/ManageTokensPS"
}

switch ($Command) {
    "list" {
        $queryParams = @()
        if ($Agent) { $queryParams += "agent=$([System.Web.HttpUtility]::UrlEncode($Agent))" }
        if ($ActiveOnly) { $queryParams += "active=true" }
        $qs = ""
        if ($queryParams.Count -gt 0) { $qs = "?" + ($queryParams -join "&") }

        $url = "$HubUrl/tokens$qs"
        $resp = Invoke-RestMethod -Uri $url -Method Get -Headers $headers
        if ($resp.tokens.Count -eq 0) {
            Write-Host "No tokens found." -ForegroundColor Yellow
            return
        }
        Write-Host "Total Tokens: $($resp.tokens.Count)`n" -ForegroundColor Cyan
        $resp.tokens | ForEach-Object {
            [PSCustomObject]@{
                Token       = if ($_.token_hash_prefix) { $_.token_hash_prefix } elseif ($_.token_hash) { $_.token_hash.Substring(0, 12) + "..." } else { $_.token }
                Agent       = $_.agent
                Projects    = ($_.projects -join ", ")
                Status      = if ($_.revoked_at) { "REVOKED" } else { "ACTIVE" }
                CreatedAt   = $_.created_at
                Description = $_.description
            }
        } | Format-Table -AutoSize
    }

    "create" {
        if (-not $Agent) {
            $Agent = Read-Host "Enter Agent Name (e.g. arin-pc)"
        }
        if (-not $Projects) {
            $Projects = Read-Host "Enter allowed projects (comma-separated, e.g. aseprite2,pitch-quest)"
        }

        $projectList = $Projects.Split(',') | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }
        $body = @{
            agent    = $Agent
            projects = $projectList
        }
        if ($Token) { $body.token = $Token }
        if ($Description) { $body.description = $Description }

        $jsonBody = $body | ConvertTo-Json
        $url = "$HubUrl/tokens"
        $resp = Invoke-RestMethod -Uri $url -Method Post -Headers $headers -Body $jsonBody

        $created = $resp.token
        Write-Host ""
        Write-Host "============================================================" -ForegroundColor Green
        Write-Host " Agent Token Created Successfully" -ForegroundColor Green
        Write-Host "============================================================" -ForegroundColor Green
        Write-Host " Agent:       $($created.agent)"
        Write-Host " Token:       $($created.token)" -ForegroundColor Yellow
        Write-Host " Projects:    $(($created.projects -join ', '))"
        Write-Host " Created At:  $($created.created_at)"
        if ($created.description) {
            Write-Host " Description: $($created.description)"
        }
        Write-Host "------------------------------------------------------------"
        Write-Host " Usage in current PowerShell session:" -ForegroundColor Cyan
        Write-Host "   `$env:AGENT_NAME=`"$($created.agent)`"; `$env:AGENT_TOKEN=`"$($created.token)`""
        Write-Host "============================================================`n"
    }

    "revoke" {
        if (-not $Token -and -not $Agent) {
            Write-Error "Must provide either -Token or -Agent to revoke."
            return
        }
        $body = @{
            purge = [bool]$Purge
        }
        if ($Token) { $body.token = $Token }
        if ($Agent) { $body.agent = $Agent }

        $jsonBody = $body | ConvertTo-Json
        $url = "$HubUrl/tokens/revoke"
        $resp = Invoke-RestMethod -Uri $url -Method Post -Headers $headers -Body $jsonBody

        $action = if ($Purge) { "purged" } else { "revoked" }
        Write-Host "Successfully $action $($resp.revoked) token(s)." -ForegroundColor Green
        if ($resp.disconnected_sessions -gt 0) {
            Write-Host "Disconnected $($resp.disconnected_sessions) active WebSocket session(s)." -ForegroundColor Yellow
        }
    }
}
