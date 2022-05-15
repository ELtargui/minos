.global _start

_start:
  pop %eax
  push $main
  call init_libc
