source scripts/wmo/gdb/dagger.py

define dagger-attach
  target remote bdi-dagger:2001
  file barebox
end

define dagger-map-pp
  # map the packet processor at 0x80000000 in the CPU's memory map
  set {u32}0xd0020050 = 0x03ff0031
  set {u32}0xd0020054 = 0x80000000

  # disable the undocumented legacy addressing mode
  set {u32}0x80000140 = 0x00008102
end

define dagger-ramload
  restore images/start_dagger.pblx binary 0
  file barebox
  set $pc=0
  c
end
