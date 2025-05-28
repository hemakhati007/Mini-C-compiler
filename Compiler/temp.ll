; Optimized IR
define i32 @main() {
  %ch = alloca i8
  %a = alloca i32
  store i32 10, i32* %a
  %x = alloca float
  store float 1.000000e+01, float* %x
  %b = alloca i32
  store i32 10, i32* %b
  ret i32 0
}
