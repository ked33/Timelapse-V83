[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SourceXml,

    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Normalize-MapField {
    param([AllowNull()][string]$Value)

    if ($null -eq $Value) {
        return ''
    }

    return ([System.Text.RegularExpressions.Regex]::Replace($Value, "[`t`r`n]+", ' ')).Trim()
}

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path (Split-Path -Parent $PSScriptRoot) 'Resources\MapNames.zh-CN.tsv'
}

$sourceFullPath = [System.IO.Path]::GetFullPath($SourceXml)
$outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)

if (-not [System.IO.File]::Exists($sourceFullPath)) {
    throw "Chinese map XML was not found: $sourceFullPath"
}

$settings = New-Object System.Xml.XmlReaderSettings
$settings.IgnoreComments = $true
$settings.IgnoreWhitespace = $true
$reader = [System.Xml.XmlReader]::Create($sourceFullPath, $settings)
$maps = @{}

try {
    while ($reader.Read()) {
        if ($reader.NodeType -ne [System.Xml.XmlNodeType]::Element -or
            $reader.Name -ne 'imgdir' -or
            $reader.Depth -ne 2) {
            continue
        }

        $mapId = 0
        $mapIdText = $reader.GetAttribute('name')
        if (-not [int]::TryParse(
                $mapIdText,
                [System.Globalization.NumberStyles]::None,
                [System.Globalization.CultureInfo]::InvariantCulture,
                [ref]$mapId)) {
            continue
        }

        $streetName = ''
        $mapName = ''
        $subtree = $reader.ReadSubtree()
        try {
            while ($subtree.Read()) {
                if ($subtree.NodeType -ne [System.Xml.XmlNodeType]::Element -or
                    $subtree.Name -ne 'string') {
                    continue
                }

                $fieldName = $subtree.GetAttribute('name')
                if ($fieldName -eq 'streetName') {
                    $streetName = Normalize-MapField $subtree.GetAttribute('value')
                }
                elseif ($fieldName -eq 'mapName') {
                    $mapName = Normalize-MapField $subtree.GetAttribute('value')
                }
            }
        }
        finally {
            $subtree.Dispose()
        }

        if ([string]::IsNullOrWhiteSpace($mapName)) {
            throw "Map $mapId does not contain a Chinese mapName value."
        }

        $entry = [pscustomobject]@{
            StreetName = $streetName
            MapName = $mapName
        }

        if ($maps.ContainsKey($mapId)) {
            $existing = $maps[$mapId]
            if ($existing.StreetName -ne $entry.StreetName -or $existing.MapName -ne $entry.MapName) {
                throw "Map $mapId has conflicting Chinese names in $sourceFullPath."
            }
            continue
        }

        $maps[$mapId] = $entry
    }
}
finally {
    $reader.Dispose()
}

if ($maps.Count -eq 0) {
    throw "No Chinese map names were read from $sourceFullPath."
}

$lines = New-Object 'System.Collections.Generic.List[string]'
$lines.Add('# Generated from wz-zh-CN/String.wz/Map.img.xml by Import-ChineseMapNames.ps1')
$lines.Add("# mapID`tstreetName`tmapName")

foreach ($mapId in @($maps.Keys | ForEach-Object { [int]$_ } | Sort-Object)) {
    $entry = $maps[$mapId]
    $lines.Add("$mapId`t$($entry.StreetName)`t$($entry.MapName)")
}

$outputDirectory = [System.IO.Path]::GetDirectoryName($outputFullPath)
[System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null

$utf8NoBom = New-Object System.Text.UTF8Encoding($false, $true)
$outputText = [string]::Join("`n", [string[]]$lines) + "`n"
$existingText = if ([System.IO.File]::Exists($outputFullPath)) {
    [System.IO.File]::ReadAllText($outputFullPath, $utf8NoBom)
}
else {
    $null
}

if ($existingText -ne $outputText) {
    [System.IO.File]::WriteAllText($outputFullPath, $outputText, $utf8NoBom)
}

Write-Host "Imported $($maps.Count) unique Chinese map names to $outputFullPath"
