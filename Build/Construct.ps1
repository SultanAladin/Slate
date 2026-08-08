# Construct.ps1 — builds every Slate unit with cl.exe, lib.exe and link.exe directly.
#
# 🔴 /MD in every configuration, including Debug. SLATE_DEBUG selects the debug path; _DEBUG is never
#    defined, because it selects the debug CRT and mixing that with /MD is a link failure at best.
#
#     powershell -File Build\Construct.ps1
#     powershell -File Build\Construct.ps1 -Configuration Debug
#     powershell -File Build\Construct.ps1 -Unit SlateMath -Rebuild

[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')] [string] $Configuration = 'Release',
    [string]                                   $Unit          = '',
    [switch]                                   $Rebuild
)

$ErrorActionPreference = 'Stop'

$RepositoryRoot = Split-Path -Parent $PSScriptRoot
$EngineRoot     = Join-Path $RepositoryRoot 'Engine'
$PackageRoot    = Join-Path $RepositoryRoot 'ExternalPackages'
$OutputRoot     = Join-Path $RepositoryRoot "_AgentScratch\build\$Configuration"

#---
#                                          THE UNIT ORDER
#---

# 📝 The order below IS the dependency DAG. A unit is compiled only after every unit it requires, and the
#    Requires list is what the linker is handed — reversed, since a static library only satisfies references
#    the linker has already seen.
$UnitOrder = @(
    @{ Name = 'SlateMath';     Product = 'StaticLibrary'; Requires = @() }
    @{ Name = 'SlateDocument'; Product = 'StaticLibrary'; Requires = @('SlateMath') }
    @{ Name = 'SlateVulkan';   Product = 'StaticLibrary'; Requires = @('SlateMath') }
    @{ Name = 'SlateCompute';  Product = 'StaticLibrary'; Requires = @('SlateVulkan', 'SlateDocument', 'SlateMath') }
    @{ Name = 'SlateUI';       Product = 'StaticLibrary'; Requires = @('SlateCompute', 'SlateVulkan', 'SlateDocument', 'SlateMath') }
    @{ Name = 'Application';   Product = 'Executable';    Requires = @('SlateUI', 'SlateCompute', 'SlateVulkan', 'SlateDocument', 'SlateMath') }
)

#---
#                                       TOOLCHAIN ACQUISITION
#---

# 📝 cl.exe is not on PATH in this environment, so the Visual Studio environment is imported here rather
#    than assumed. vcvarsall.bat runs once and the environment it produced is read back into this session.
function Import-ToolchainEnvironment
{
    if (Get-Command cl.exe -ErrorAction SilentlyContinue)
    {
        Write-Host '  toolchain  already on PATH'
        return
    }

    $Candidates = @(
        'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat'
        'C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvarsall.bat'
        'C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvarsall.bat'
        'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat'
        'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat'
    )

    $Selected = $Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1

    if ($null -eq $Selected)
    {
        throw 'no vcvarsall.bat was found; the C++ toolchain is not installed where this script looks'
    }

    Write-Host "  toolchain  $Selected"

    $Captured = cmd.exe /c "`"$Selected`" x64 > nul & set"

    foreach ($Line in $Captured)
    {
        if ($Line -match '^([^=]+)=(.*)$')
        {
            Set-Item -Path "env:$($Matches[1])" -Value $Matches[2] -ErrorAction SilentlyContinue
        }
    }

    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue))
    {
        throw 'vcvarsall.bat ran but cl.exe is still absent from PATH'
    }
}

function Resolve-VulkanRoot
{
    if ($env:VULKAN_SDK -and (Test-Path $env:VULKAN_SDK))
    {
        return $env:VULKAN_SDK
    }

    $Installed = Get-ChildItem 'C:\VulkanSDK' -Directory -ErrorAction SilentlyContinue |
                 Sort-Object Name -Descending |
                 Select-Object -First 1

    if ($null -eq $Installed)
    {
        throw 'no Vulkan SDK was found; VULKAN_SDK is unset and C:\VulkanSDK holds nothing'
    }

    return $Installed.FullName
}

#---
#                                         COMPILATION FLAGS
#---

# 🔴 /fp:precise is not decoration. The exact orientation predicate relies on round-to-nearest and on the
#    absence of contraction; /fp:fast reassociates the filtered determinant and its sign stops being exact.
function Get-CompilationFlags([string] $Selection)
{
    $Common = @(
        '/nologo'
        '/c'
        '/EHsc'
        '/MD'
        '/std:c++20'
        '/permissive-'
        '/fp:precise'
        '/W4'
        '/utf-8'
        '/Zc:__cplusplus'
        '/DWIN32_LEAN_AND_MEAN'
        '/DNOMINMAX'
        '/DGLFW_DLL'
    )

    if ($Selection -eq 'Debug')
    {
        # 📝 🔴 SLATE_DEBUG selects every debug path in the engine. _DEBUG is never defined — it selects the
        #    debug CRT, and /MD is declared for every configuration, so the two cannot both be honoured.
        return $Common + @('/Od', '/Zi', '/DSLATE_DEBUG=1')
    }

    return $Common + @('/O2', '/Zi', '/DNDEBUG')
}

#---
#                                           PATH ASSEMBLY
#---

function Get-IncludePath([hashtable] $UnitEntry, [string] $VulkanRoot)
{
    # 📝 Contract/ and Shared/ are reachable from every unit through the engine root, and so is every other
    #    unit's Api/ folder. The partition is not enforced by hiding headers — it is enforced by the link:
    #    SlateDocument is never handed SlateVulkan.lib, so a device reference fails to resolve.
    $Paths  = @($EngineRoot)
    $Paths += (Join-Path $PackageRoot 'glfw\include')
    $Paths += (Join-Path $VulkanRoot  'Include')

    if ($UnitEntry.Name -eq 'SlateUI')
    {
        $Paths += (Join-Path $PackageRoot 'imgui')
    }

    return @($Paths | ForEach-Object { "/I$_" })
}

function Get-UnitSource([hashtable] $UnitEntry)
{
    $UnitRoot = Join-Path $EngineRoot $UnitEntry.Name
    $Sources  = @(Get-ChildItem $UnitRoot -Recurse -Filter '*.cpp' -File | ForEach-Object { $_.FullName })

    # 📝 🔴 `00` §2.2: exactly one copy of ImGui exists in the process and it is compiled into SlateUI. The
    #    vendored translation units are appended here rather than built into a library of their own, so a
    #    second copy cannot enter the link.
    if ($UnitEntry.Name -eq 'SlateUI')
    {
        $Sources += @(
            (Join-Path $PackageRoot 'imgui\imgui.cpp')
            (Join-Path $PackageRoot 'imgui\imgui_draw.cpp')
            (Join-Path $PackageRoot 'imgui\imgui_tables.cpp')
            (Join-Path $PackageRoot 'imgui\imgui_widgets.cpp')
            (Join-Path $PackageRoot 'imgui\backends\imgui_impl_glfw.cpp')
            (Join-Path $PackageRoot 'imgui\backends\imgui_impl_vulkan.cpp')
        )
    }

    return $Sources
}

#---
#                                           TRANSLATION
#---

# 📝 MSVC writes its diagnostics to stdout, so nothing here redirects stderr. Redirecting a native
#    executable's stderr in Windows PowerShell wraps each line in an ErrorRecord and, under an Stop
#    preference, turns a plain warning into a thrown build failure.
function Invoke-Translation([hashtable] $UnitEntry, [string] $Selection, [string] $VulkanRoot)
{
    $UnitName    = $UnitEntry.Name
    $ObjectRoot  = Join-Path $OutputRoot "Object\$UnitName"
    $Sources     = Get-UnitSource $UnitEntry
    $IncludePath = Get-IncludePath $UnitEntry $VulkanRoot
    $Flags       = Get-CompilationFlags $Selection

    if ($Sources.Count -eq 0)
    {
        throw "$UnitName declares no translation unit"
    }

    if (-not (Test-Path $ObjectRoot))
    {
        New-Item -ItemType Directory -Force -Path $ObjectRoot | Out-Null
    }

    Write-Host "  $UnitName — $($Sources.Count) translation units"

    $Produced  = New-Object System.Collections.Generic.List[string]
    $Retranslated = 0

    foreach ($Source in $Sources)
    {
        $Stem       = [System.IO.Path]::GetFileNameWithoutExtension($Source)
        $ObjectPath = Join-Path $ObjectRoot "$Stem.obj"
        $Produced.Add($ObjectPath)

        if (-not $Rebuild -and (Test-Path $ObjectPath) -and
            (Get-Item $ObjectPath).LastWriteTimeUtc -gt (Get-Item $Source).LastWriteTimeUtc)
        {
            continue
        }

        # 📝 /Fd names a per-unit database. Sharing one across units serialises the compiler on it, and
        #    concurrent invocations corrupt it outright.
        $Arguments = $Flags + $IncludePath + @(
            "/Fo$ObjectPath"
            "/Fd$(Join-Path $ObjectRoot "$UnitName.pdb")"
            $Source
        )

        $Diagnostics = & cl.exe @Arguments
        $Refused     = $LASTEXITCODE -ne 0
        ++$Retranslated

        $Notable = $Diagnostics | Where-Object { $_ -match ': (warning|error) ' }

        if ($Notable)
        {
            $Notable | ForEach-Object { Write-Host "    $_" }
        }

        if ($Refused)
        {
            throw "$UnitName — cl.exe refused $([System.IO.Path]::GetFileName($Source))"
        }
    }

    if ($Retranslated -eq 0)
    {
        Write-Host '    unchanged'
    }

    return $Produced.ToArray()
}

#---
#                                           ARCHIVING
#---

function Invoke-Archive([hashtable] $UnitEntry, [string[]] $ObjectPath)
{
    $LibraryRoot = Join-Path $OutputRoot 'Library'

    if (-not (Test-Path $LibraryRoot))
    {
        New-Item -ItemType Directory -Force -Path $LibraryRoot | Out-Null
    }

    $LibraryPath = Join-Path $LibraryRoot "$($UnitEntry.Name).lib"
    $Diagnostics = & lib.exe /nologo "/OUT:$LibraryPath" @ObjectPath

    if ($LASTEXITCODE -ne 0)
    {
        $Diagnostics | ForEach-Object { Write-Host "    $_" }
        throw "$($UnitEntry.Name) — lib.exe refused the archive"
    }

    Write-Host "    -> $LibraryPath"
}

#---
#                                          HOST LINKING
#---

function Invoke-HostLink([hashtable] $UnitEntry, [string[]] $ObjectPath, [string] $VulkanRoot)
{
    $BinaryRoot  = Join-Path $OutputRoot 'Binary'
    $LibraryRoot = Join-Path $OutputRoot 'Library'

    if (-not (Test-Path $BinaryRoot))
    {
        New-Item -ItemType Directory -Force -Path $BinaryRoot | Out-Null
    }

    # 📝 Requires is already declared most-dependent first, which is the order the linker resolves against.
    $Linked = @($UnitEntry.Requires | ForEach-Object { Join-Path $LibraryRoot "$_.lib" })

    $Linked += (Join-Path $VulkanRoot  'Lib\vulkan-1.lib')
    $Linked += (Join-Path $PackageRoot 'glfw\lib-vc2022\glfw3dll.lib')

    $ExecutablePath = Join-Path $BinaryRoot 'ConsoleHost.exe'

    $Arguments = @(
        '/nologo'
        '/DEBUG'
        '/SUBSYSTEM:CONSOLE'
        "/OUT:$ExecutablePath"
        "/PDB:$(Join-Path $BinaryRoot 'ConsoleHost.pdb')"
    ) + $ObjectPath + $Linked

    $Diagnostics = & link.exe @Arguments

    if ($LASTEXITCODE -ne 0)
    {
        $Diagnostics | ForEach-Object { Write-Host "    $_" }
        throw 'link.exe refused the host'
    }

    # 📝 🔴 glfw3dll.lib is an import library. Without glfw3.dll beside the executable the process fails to
    #    start, and the operating system reports a missing dependency rather than anything a reader can act
    #    on. Copying it here is what keeps that failure out of the run.
    Copy-Item (Join-Path $PackageRoot 'glfw\lib-vc2022\glfw3.dll') $BinaryRoot -Force

    Write-Host "    -> $ExecutablePath"
}

#---
#                                             THE RUN
#---

Write-Host "Slate — $Configuration"

if ($Rebuild -and (Test-Path (Join-Path $OutputRoot 'Object')))
{
    Remove-Item (Join-Path $OutputRoot 'Object') -Recurse -Force
}

if (-not (Test-Path $OutputRoot))
{
    New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
}

Import-ToolchainEnvironment

$VulkanRoot = Resolve-VulkanRoot
Write-Host "  vulkan     $VulkanRoot"
Write-Host ''

$Selected = if ($Unit) { @($UnitOrder | Where-Object { $_.Name -eq $Unit }) } else { $UnitOrder }

if ($Selected.Count -eq 0)
{
    throw "no unit is named $Unit"
}

foreach ($UnitEntry in $Selected)
{
    $Produced = Invoke-Translation $UnitEntry $Configuration $VulkanRoot

    if ($UnitEntry.Product -eq 'StaticLibrary')
    {
        Invoke-Archive $UnitEntry $Produced
    }
    else
    {
        Invoke-HostLink $UnitEntry $Produced $VulkanRoot
    }
}

Write-Host ''
Write-Host "constructed into $OutputRoot"
