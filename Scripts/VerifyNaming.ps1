# Verifies that retired Slate vocabulary does not return to first-party Engine names.

$ErrorActionPreference = 'Stop'
$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$EngineRoot = Join-Path $RepositoryRoot 'Engine'
$Extensions = @('.h', '.hpp', '.cpp', '.c', '.slang')
$Retired = @('Ceiling', 'Ordinal', 'Choice', 'Boundary', 'Region',
             ('Con' + 'tract'), ('con' + 'tract'), ('Led' + 'ger'), ('led' + 'ger'))
$VendorBindingTokens = @('VkDescriptorSetLayoutBinding', 'pBindings', 'dstBinding')
$Failures = [System.Collections.Generic.List[string]]::new()

Get-ChildItem $EngineRoot -File -Recurse |
    Where-Object { $Extensions -contains $_.Extension } |
    ForEach-Object {
        $Path = $_.FullName
        $Relative = [System.IO.Path]::GetRelativePath($RepositoryRoot, $Path).Replace('\', '/')
        $LineNumber = 0
        Get-Content $Path | ForEach-Object {
            $LineNumber++
            $Line = $_
            $SlateText = $Line.Replace('GetContentRegionAvail', '')
            foreach ($Word in $Retired) {
                if ($SlateText.Contains($Word)) { $Failures.Add("${Relative}:${LineNumber}: $Word") }
            }
            if ($Line -match '\bConstruct\s*\(') {
                $Failures.Add("${Relative}:${LineNumber}: plain Construct method")
            }
            foreach ($Match in [regex]::Matches($Line, '[A-Za-z_][A-Za-z0-9_]*Binding[A-Za-z0-9_]*')) {
                if ($VendorBindingTokens -notcontains $Match.Value) {
                    $Failures.Add("${Relative}:${LineNumber}: $($Match.Value)")
                }
            }
        }
    }

Get-ChildItem $EngineRoot -Recurse | ForEach-Object {
    foreach ($Word in $Retired) {
        if ($_.Name.Contains($Word)) {
            $Relative = [System.IO.Path]::GetRelativePath($RepositoryRoot, $_.FullName).Replace('\', '/')
            $Failures.Add("${Relative}: $Word in path")
        }
    }
}

if ($Failures.Count -ne 0) {
    $Failures | ForEach-Object { Write-Host "[Naming] $_" -ForegroundColor Red }
    throw "$($Failures.Count) naming violation(s)"
}

Write-Host '[Naming] first-party Engine vocabulary holds' -ForegroundColor Green
