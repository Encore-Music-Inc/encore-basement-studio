# ENCORE. Basement Studio
# Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).

# Does a plugin checker hide a prompt that a CHILD process puts up, and clean it up when it gives up?
# Runs Studio with --windowtest and a child PowerShell whose window is see-through and takes no focus.
param([string]$Exe = "$PSScriptRoot\..\build\EncoreBasementStudio_artefacts\Release\ENCORE Basement Studio.exe")
$marker = "EncoreWindowTest" + (Get-Random)
$dir = Join-Path $env:TEMP $marker
New-Item -ItemType Directory $dir | Out-Null
$child = Join-Path $dir "prompt.ps1"
@"
Add-Type -AssemblyName System.Windows.Forms
Add-Type -ReferencedAssemblies System.Windows.Forms -TypeDefinition 'public class F : System.Windows.Forms.Form { protected override bool ShowWithoutActivation { get { return true; } } }'
`$f = New-Object F; `$f.Text = '$marker'; `$f.Opacity = 0; `$f.ShowInTaskbar = `$false
`$f.Show(); [System.Windows.Forms.Application]::Run()
"@ | Set-Content $child
$p = Start-Process -FilePath $Exe -ArgumentList "--windowtest", "`"$child`"" -PassThru

Add-Type @"
using System; using System.Runtime.InteropServices; using System.Text;
public class Win {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  public static int Count(string title, bool visibleOnly) { int n = 0;
    EnumWindows((h, l) => { var s = new StringBuilder(256); GetWindowText(h, s, 256);
      if (s.ToString() == title && (!visibleOnly || IsWindowVisible(h))) n++; return true; }, IntPtr.Zero); return n; }
}
"@
$seen = $false; $visible = 0; $checks = 0
$end = (Get-Date).AddSeconds(25)
while (-not $p.HasExited -and (Get-Date) -lt $end) {
  if ([Win]::Count($marker, $false) -gt 0) { $seen = $true; $checks++; if ([Win]::Count($marker, $true) -gt 0) { $visible++ } }
  Start-Sleep -Milliseconds 20
}
if (-not $p.HasExited) { "FAIL the checker didn't give up"; Stop-Process $p -Force }
Start-Sleep -Milliseconds 500
$left = @(Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" | Where-Object { $_.CommandLine -like "*$marker*" })
if (-not $seen) { "the prompt never opened: this run proves nothing" }
"prompt seen: $seen, $visible of $checks checks visible, exit code $($p.ExitCode), left running: $($left.Count)"
$left | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }
Remove-Item $dir -Recurse -Force
if ($seen -and $visible -le 3 -and $p.ExitCode -eq 3 -and $left.Count -eq 0) { "PASS"; exit 0 } else { "FAIL"; exit 1 }
