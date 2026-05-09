$fqbn = "esp32:esp32:uPesy_wroom:UploadSpeed=921600,CPUFreq=240,FlashFreq=80,FlashMode=qio,PartitionScheme=min_spiffs,DebugLevel=none,EraseFlash=none"
$ip   = "10.0.0.69"
$pass = "harv123"
$out  = "$PSScriptRoot\build"

Write-Host "==> Compiling..."
arduino-cli compile --fqbn $fqbn --output-dir $out harv_firmware
if ($LASTEXITCODE -ne 0) { Write-Host "Compile failed"; exit 1 }

Write-Host "==> Uploading via OTA..."
arduino-cli upload --fqbn $fqbn --port $ip --protocol network `
  --upload-field password=$pass --discovery-timeout 15s harv_firmware
if ($LASTEXITCODE -eq 0) { Write-Host "Done."; exit 0 }

Write-Host "==> arduino-cli upload failed, trying espota.py directly..."
$espota = "$env:LOCALAPPDATA\Arduino15\packages\esp32\hardware\esp32\3.3.8\tools\espota.py"
$bin    = "$out\harv_firmware.ino.bin"
python $espota -i $ip -p 3232 --auth=$pass -f $bin
