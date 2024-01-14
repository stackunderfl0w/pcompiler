#include <stdio.h>
#include <stdlib.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ast.h"

#include "scan.h"



#include "scope.h"


typedef int (*JitFunction)();

extern int yydebug;
extern ast_node* target_program;
extern int parse_error;

scope * parse_symbols;

int main() {
    //yydebug=1;

    parse_symbols= scope_create(NULL);

    //yyin = fopen("../src/t1","r");

    struct stat statbuf;

    int t1_fd= open("../src/t2",O_RDONLY);

    fstat(t1_fd,&statbuf);

    char* src_txt= malloc(statbuf.st_size+1);
    read(t1_fd,src_txt, statbuf.st_size);
    close(t1_fd);




    yy_scan_string(src_txt);
    yylex();

    if(parse_error)
        exit(1);

    LLVMModuleRef module = generate_code(target_program,NULL);

    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();


    // Create an execution engine for the module
    char* error;
    LLVMExecutionEngineRef engine;
    if (LLVMCreateJITCompilerForModule(&engine, module, 2, &error) != 0) {
        fprintf(stderr, "Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        return 1;
    }

    // Retrieve the JIT function address
    //JitFunction jitFunctionPtr = (JitFunction)LLVMGetFunctionAddress(engine, "main");

    //int (*jitFunctionPtr)(void)=(int (*)(void)) LLVMGetFunctionAddress(engine, "main");
    //int (*jitFunctionPtr)(int)=(int (*)(int)) LLVMGetFunctionAddress(engine, "factorial");
    //int (*jitFunctionPtr)(void)=(int (*)(void)) LLVMGetFunctionAddress(engine, "arr_test");
    //int (*jitFunctionPtr)(void)=(int (*)(void)) LLVMGetFunctionAddress(engine, "inc_test");
    //int (*jitFunctionPtr)(void)=(int (*)(void)) LLVMGetFunctionAddress(engine, "break_test");
    //int (*jitFunctionPtr)(int)=(int (*)(int)) LLVMGetFunctionAddress(engine, "cond_test");
    //int (*jitFunctionPtr)(int)=(int (*)(int)) LLVMGetFunctionAddress(engine, "fib");
    //int (*jitFunctionPtr)(int,int)=(int (*)(int,int)) LLVMGetFunctionAddress(engine, "arg_test");
    //int (*jitFunctionPtr)(int)=(int (*)(int)) LLVMGetFunctionAddress(engine, "type_test");
    int (*jitFunctionPtr)(float)=(int (*)(float)) LLVMGetFunctionAddress(engine, "float_convert_test");

    printf("%p\n",jitFunctionPtr);

    //for (int i = 0; i < 11; ++i) {
    //    int x=jitFunctionPtr(i);
    //    printf("jit res:%d\n",x);
    //
    //}


    // Call the JIT function
    int x=jitFunctionPtr(7);
    printf("jit res:%d\n",x);

    // Clean up
    LLVMDisposeExecutionEngine(engine);
    //segfault when I try and dispose of both
    //LLVMDisposeModule(module);

    return 0;
}
