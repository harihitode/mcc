; ModuleID = 'lib.c'
source_filename = "lib.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.13.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%f\00", align 1

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_atan(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = call float @atanf(float %3) #3
  ret float %4
}

; Function Attrs: nounwind readnone
declare float @atanf(float) #1

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_sin(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = call float @sinf(float %3) #3
  ret float %4
}

; Function Attrs: nounwind readnone
declare float @sinf(float) #1

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_cos(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = call float @cosf(float %3) #3
  ret float %4
}

; Function Attrs: nounwind readnone
declare float @cosf(float) #1

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_sqrt(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = call float @sqrtf(float %3) #3
  ret float %4
}

; Function Attrs: nounwind readnone
declare float @sqrtf(float) #1

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_read_int() #0 {
  %1 = alloca i32, align 4
  %2 = call i32 (i8*, ...) @scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i32 0, i32 0), i32* %1)
  %3 = load i32, i32* %1, align 4
  ret i32 %3
}

declare i32 @scanf(i8*, ...) #2

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_read_float() #0 {
  %1 = alloca float, align 4
  %2 = call i32 (i8*, ...) @scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.1, i32 0, i32 0), float* %1)
  %3 = load float, float* %1, align 4
  ret float %3
}

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_fabs(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = fpext float %3 to double
  %5 = call double @fabs(double %4) #3
  %6 = fptrunc double %5 to float
  ret float %6
}

; Function Attrs: nounwind readnone
declare double @fabs(double) #1

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_fhalf(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = fpext float %3 to double
  %5 = fmul double %4, 5.000000e-01
  %6 = fptrunc double %5 to float
  ret float %6
}

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_fisneg(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = fpext float %3 to double
  %5 = fcmp olt double %4, 0.000000e+00
  %6 = zext i1 %5 to i64
  %7 = select i1 %5, i32 1, i32 0
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_fispos(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = fpext float %3 to double
  %5 = fcmp ogt double %4, 0.000000e+00
  %6 = zext i1 %5 to i64
  %7 = select i1 %5, i32 1, i32 0
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_fiszero(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = fpext float %3 to double
  %5 = fcmp oeq double %4, 0.000000e+00
  %6 = zext i1 %5 to i64
  %7 = select i1 %5, i32 1, i32 0
  ret i32 %7
}

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_fless(float, float) #0 {
  %3 = alloca float, align 4
  %4 = alloca float, align 4
  store float %0, float* %3, align 4
  store float %1, float* %4, align 4
  %5 = load float, float* %3, align 4
  %6 = load float, float* %4, align 4
  %7 = fcmp olt float %5, %6
  %8 = zext i1 %7 to i64
  %9 = select i1 %7, i32 1, i32 0
  ret i32 %9
}

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_fneg(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = fsub float -0.000000e+00, %3
  ret float %4
}

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_fsqr(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = load float, float* %2, align 4
  %5 = fmul float %3, %4
  ret float %5
}

; Function Attrs: noinline nounwind ssp uwtable
define void @min_caml_print_char(i32) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i8, align 1
  store i32 %0, i32* %2, align 4
  %4 = load i32, i32* %2, align 4
  %5 = trunc i32 %4 to i8
  store i8 %5, i8* %3, align 1
  %6 = load i8, i8* %3, align 1
  %7 = sext i8 %6 to i32
  %8 = call i32 @putchar(i32 %7)
  ret void
}

declare i32 @putchar(i32) #2

; Function Attrs: noinline nounwind ssp uwtable
define void @min_caml_print_int(i32) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i32 0, i32 0), i32 %3)
  ret void
}

declare i32 @printf(i8*, ...) #2

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_truncate(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = fcmp olt float %3, 0.000000e+00
  br i1 %4, label %5, label %9

; <label>:5:                                      ; preds = %1
  %6 = load float, float* %2, align 4
  %7 = fpext float %6 to double
  %8 = call double @ceil(double %7) #3
  br label %13

; <label>:9:                                      ; preds = %1
  %10 = load float, float* %2, align 4
  %11 = fpext float %10 to double
  %12 = call double @floor(double %11) #3
  br label %13

; <label>:13:                                     ; preds = %9, %5
  %14 = phi double [ %8, %5 ], [ %12, %9 ]
  %15 = fptosi double %14 to i32
  ret i32 %15
}

; Function Attrs: nounwind readnone
declare double @ceil(double) #1

; Function Attrs: nounwind readnone
declare double @floor(double) #1

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_ceil(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = call float @ceilf(float %3) #3
  %5 = fptosi float %4 to i32
  ret i32 %5
}

; Function Attrs: nounwind readnone
declare float @ceilf(float) #1

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_floor(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = call float @floorf(float %3) #3
  %5 = fptosi float %4 to i32
  ret i32 %5
}

; Function Attrs: nounwind readnone
declare float @floorf(float) #1

; Function Attrs: noinline nounwind ssp uwtable
define float @min_caml_float_of_int(i32) #0 {
  %2 = alloca i32, align 4
  %3 = alloca float, align 4
  store i32 %0, i32* %2, align 4
  %4 = load i32, i32* %2, align 4
  %5 = sitofp i32 %4 to float
  store float %5, float* %3, align 4
  %6 = load float, float* %3, align 4
  ret float %6
}

; Function Attrs: noinline nounwind ssp uwtable
define i32 @min_caml_int_of_float(float) #0 {
  %2 = alloca float, align 4
  %3 = alloca i32, align 4
  store float %0, float* %2, align 4
  %4 = load float, float* %2, align 4
  %5 = fptosi float %4 to i32
  store i32 %5, i32* %3, align 4
  %6 = load i32, i32* %3, align 4
  ret i32 %6
}

; Function Attrs: noinline nounwind ssp uwtable
define void @min_caml_print_float(float) #0 {
  %2 = alloca float, align 4
  store float %0, float* %2, align 4
  %3 = load float, float* %2, align 4
  %4 = fpext float %3 to double
  %5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.1, i32 0, i32 0), double %4)
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable
define void @min_caml_print_newline() #0 {
  %1 = call i32 @putchar(i32 10)
  ret void
}

attributes #0 = { noinline nounwind ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind readnone }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"Apple LLVM version 9.0.0 (clang-900.0.39.2)"}
