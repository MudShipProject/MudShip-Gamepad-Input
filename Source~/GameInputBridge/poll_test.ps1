# 背面入力の実測テスト（Unity不要・30秒間ボタンを押す）
$dll = Join-Path $PSScriptRoot "MS_GamepadBridge.dll"
if (-not (Test-Path $dll)) { $dll = Join-Path $PSScriptRoot "..\..\Runtime\Plugins\x86_64\MS_GamepadBridge.dll" }
$src = @"
using System; using System.Runtime.InteropServices;
public static class T {
  [StructLayout(LayoutKind.Sequential)] public struct F { public int connected; public uint buttons; public float lx,ly,rx,ry,lt,rt; }
  [DllImport(@"REPLACE_DLL")] public static extern int GIB_Init();
  [DllImport(@"REPLACE_DLL")] public static extern int GIB_Poll([Out] F[] f, int n);
  [DllImport(@"REPLACE_DLL")] public static extern void GIB_Shutdown();
}
"@.Replace("REPLACE_DLL",$dll)
Add-Type -TypeDefinition $src
$init = [T]::GIB_Init()
"GIB_Init=$init  (30秒間ボタンを押してください)"
$frames = New-Object 'T+F[]' 8
$seen=$false; $hits=0; $maxBtn=0; $maxTrig=0.0
$end=(Get-Date).AddSeconds(30)
while((Get-Date) -lt $end){
  [T]::GIB_Poll($frames,8) | Out-Null
  for($i=0;$i -lt 8;$i++){
    $f=$frames[$i]
    if($f.connected -ne 0){
      $active = ($f.buttons -ne 0) -or ([math]::Abs($f.lx) -gt 0.4) -or ([math]::Abs($f.ly) -gt 0.4) -or ([math]::Abs($f.rx) -gt 0.4) -or ([math]::Abs($f.ry) -gt 0.4) -or ($f.lt -gt 0.2) -or ($f.rt -gt 0.2)
      if($active){ $seen=$true; $hits++; if($f.buttons -gt $maxBtn){$maxBtn=$f.buttons}; $t=[math]::Max($f.lt,$f.rt); if($t -gt $maxTrig){$maxTrig=$t} }
    }
  }
  [System.Threading.Thread]::Sleep(40)
}
[T]::GIB_Shutdown()
"=== RESULT ==="
"seenInput = $seen"
"hits = $hits"
("maxButtons = 0x{0:X4}" -f $maxBtn)
("maxTrigger = {0:F2}" -f $maxTrig)
