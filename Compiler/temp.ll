; Optimized IR
define i32 @main() {
  %a = alloca i32
  store i32 10, i32* %a
  %b = alloca i32
  store i32 10, i32* %b
  %ch = alloca i32
  ret i32 0
}
